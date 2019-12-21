/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: lightduer_bind_device.c
 * Auth: Chen Xihao (chenxihao@baidu.com)
 * Desc: Support WeChat Subscription to bind device
 */
#include "lightduer_bind_device.h"
#ifdef __linux__
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include <unistd.h>
#else
#include <sockets.h>
#endif
#include <stdbool.h>
#include "baidu_json.h"
#include "mbedtls/base64.h"
#include "lightduer_events.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_timers.h"
#include "lightduer_aes.h"
#include "lightduer_sleep.h"
#include "lightduer_ds_log_bind.h"

/*
 * airkiss package format
 * |-------------------------Header-----------------------|
 * |   4 bytes    | 4 bytes |     4 bytes    |   4 bytes  |
 * | magic number | version | package length | command id |
 * |-------------------------Body-------------------------|
 */
#ifndef DUER_AIRKISS_DEVICE_TYPE
#define DUER_AIRKISS_DEVICE_TYPE "gh_e2314fcc26d5" // wechat public number
// #define DUER_AIRKISS_DEVICE_TYPE "gh_41bc99e17d4b" // wechat public number for test
#endif
static const int AIRKISS_DEFAULT_LAN_PORT = 12476;
static const uint8_t AIRKISS_MAGIC_NUMBER[4] = { 0xfd, 0x01, 0xfe, 0xfc};
static const uint8_t AIRKISS_VERSION[4] = { 0x00, 0x20, 0x00, 0x02};
static const uint8_t AIRKISS_SEND_VERSION[4] = { 0x00, 0x10, 0x00, 0x01};

static const size_t STACK_SIZE = 3 * 1024;
static duer_events_handler s_events = NULL;
static duer_timer_handler s_stop_timer = NULL;
static volatile bool s_stop_send = false;

static duer_aes_context s_aes_ctx;
static const char *AES_KEY = "e1c361cc29e43fb8"; // 16bytes
static const char *DEVICE_ID_PREFIX = "duer_";
static char *s_device_uuid = NULL;
static char *s_bind_token = NULL;
static duer_ds_log_bind_code_t s_log_code;

typedef enum {
    /* error message */
    DUER_AIRKISS_ERR = -1,

    /* correct message, but do not need to handle */
    DUER_AIRKISS_CONTINUE = 0,

    /* received discovery request */
    DUER_AIRKISS_SSDP_REQ = 1,

    /* message package ready */
    DUER_AIRKISS_PAKE_READY = 2
} duer_airkiss_ret_t;

typedef enum {
    DUER_AIRKISS_SSDP_REQ_CMD = 0x1,
    DUER_AIRKISS_SSDP_RESP_CMD = 0x1001,
    DUER_AIRKISS_SSDP_NOTIFY_CMD = 0x1002
} airkiss_lan_cmdid_t;

static uint8_t *duer_aes_cbc_encrypte_bind_token(const char *bind_token)
{
    const size_t BIND_TOKEN_LEN = 32;
    uint8_t *enc_data = NULL;
    uint8_t *base64_data = NULL;
    size_t olen = 0;
    int ret = 0;

    if (bind_token == NULL || DUER_STRLEN(bind_token) != BIND_TOKEN_LEN) {
        DUER_LOGE("invalid bind token!");
        return NULL;
    }

    enc_data = (uint8_t *)DUER_MALLOC(BIND_TOKEN_LEN);
    if (enc_data == NULL) {
        DUER_LOGE("no free memory");
        return NULL;
    }

    do {
        ret = duer_aes_cbc_encrypt(s_aes_ctx, BIND_TOKEN_LEN, (uint8_t *)bind_token, enc_data);
        if (ret != DUER_OK) {
            DUER_LOGE("encrypt failed");
            break;
        }

        olen = BIND_TOKEN_LEN * 4 / 3 + 3;
        base64_data = (uint8_t *)DUER_MALLOC(olen);
        if (base64_data == NULL) {
            DUER_LOGE("no free memory");
            break;
        }

        ret = mbedtls_base64_encode(base64_data, olen, &olen, enc_data, BIND_TOKEN_LEN);
        base64_data[olen] = 0;
        if (ret < 0) {
            DUER_LOGE("base64 encode failed");
            DUER_FREE(base64_data);
            base64_data = NULL;
        }
    } while (0);

    DUER_FREE(enc_data);
    enc_data = NULL;

    return base64_data;
}

// only use to test duer_aes_cbc_encrypte_bind_token
#if 0
static char *duer_aes_cbc_decrypte_bind_token(const uint8_t *base64_data)
{
    const size_t BIND_TOKEN_LEN = 32;
    uint8_t *enc_data = NULL;
    uint8_t *bind_token = NULL;
    size_t olen = 0;
    int ret = 0;

    if (base64_data == NULL) {
        DUER_LOGE("invalid base64_data!");
        return NULL;
    }

    enc_data = (uint8_t *)DUER_MALLOC(BIND_TOKEN_LEN);
    if (enc_data == NULL) {
        DUER_LOGE("no free memory");
        return NULL;
    }

    do {
        ret = mbedtls_base64_decode(enc_data, BIND_TOKEN_LEN, &olen,
                   base64_data, DUER_STRLEN((char *)base64_data));
        if (ret < 0 || olen != BIND_TOKEN_LEN) {
            DUER_LOGE("base64 decode failed, olen = %d", olen);
            break;
        }

        bind_token = (uint8_t *)DUER_MALLOC(BIND_TOKEN_LEN + 1);
        if (bind_token == NULL) {
            DUER_LOGE("no free memory");
            break;
        }
        bind_token[BIND_TOKEN_LEN] = 0;

        ret = duer_aes_cbc_decrypt(s_aes_ctx, BIND_TOKEN_LEN, enc_data, bind_token);
        if (ret != DUER_OK) {
            DUER_LOGE("decrypt failed");
            DUER_FREE(bind_token);
            bind_token = NULL;
        }
    } while (0);

    DUER_FREE(enc_data);
    enc_data = NULL;

    return (char *)bind_token;
}
#endif

static int duer_get_socket_errno(int fd)
{
    int sock_errno = 0;
    uint32_t optlen = sizeof(sock_errno);

    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &sock_errno, &optlen) < 0) {
        DUER_LOGE("getsockopt failed");
    }

    return sock_errno;
}

static duer_airkiss_ret_t duer_airkiss_lan_recv(const uint8_t *msg, uint16_t len)
{
    // msg must contain at least 16 bytes
    if (msg == NULL || len < 16) {
        DUER_LOGI("duer_airkiss_lan_recv error msg");
        return DUER_AIRKISS_ERR;
    }

    if (DUER_MEMCMP(msg, AIRKISS_MAGIC_NUMBER, sizeof(AIRKISS_MAGIC_NUMBER)) != 0) {
        DUER_LOGI("not airkiss packet");
        return DUER_AIRKISS_ERR;
    }
    msg += sizeof(AIRKISS_MAGIC_NUMBER);

    // skip version code
    DUER_LOGD("version code:0x%x%x%x%x", msg[0], msg[1], msg[2], msg[3]);
    msg += 4;

    // skip msg length
    msg += 4;

    uint32_t cmd = msg[0] << 24 | msg[1] << 16 | msg[2] << 8 | msg[3];
    DUER_LOGD("airkiss cmd: 0x%x", cmd);
    if (cmd == DUER_AIRKISS_SSDP_REQ_CMD) {
        return DUER_AIRKISS_SSDP_REQ;
    }

    return DUER_AIRKISS_CONTINUE;
}

static int duer_airkiss_pack_device_info(uint32_t cmdid, uint8_t **outbuf, uint16_t *len)
{
    baidu_json *msg = NULL;
    baidu_json *dev_info = NULL;
    uint8_t *buf = NULL;
    char *body = NULL;
    char *device_id = NULL;
    char *enc_token = NULL;
    int id_len = 0;
    int body_len = 0;
    int ret = -1;

    if (outbuf == NULL || *outbuf != NULL || len == NULL) {
        DUER_LOGE("invalid arguments");
        return -1;
    }

    do {
        msg = baidu_json_CreateObject();
        if (msg == NULL) {
            DUER_LOGE("create msg failed");
            s_log_code = DUER_DS_LOG_BIND_NO_MEMORY;
            break;
        }

        dev_info = baidu_json_CreateObject();
        if (dev_info == NULL) {
            DUER_LOGE("create dev_info failed");
            s_log_code = DUER_DS_LOG_BIND_NO_MEMORY;
            break;
        }
        baidu_json_AddItemToObjectCS(msg, "deviceInfo", dev_info);

        id_len = DUER_STRLEN(DEVICE_ID_PREFIX) + DUER_STRLEN(s_device_uuid) + 1;
        device_id = (char *)DUER_MALLOC(id_len);
        if (device_id == NULL) {
            s_log_code = DUER_DS_LOG_BIND_NO_MEMORY;
            DUER_LOGE("no free memory");
            break;
        }
        DUER_SNPRINTF(device_id, id_len, "%s%s", DEVICE_ID_PREFIX, s_device_uuid);

        baidu_json_AddStringToObjectCS(dev_info, "deviceType", DUER_AIRKISS_DEVICE_TYPE);
        baidu_json_AddStringToObjectCS(dev_info, "deviceId", device_id);
        DUER_FREE(device_id);
        device_id = NULL;

        enc_token = (char *)duer_aes_cbc_encrypte_bind_token(s_bind_token);
        if (enc_token == NULL) {
            s_log_code = DUER_DS_LOG_BIND_ENCRYPT_FAILED;
            break;
        }
        baidu_json_AddStringToObjectCS(msg, "manufacturerData", enc_token);
        DUER_FREE(enc_token);
        enc_token = NULL;


        body = baidu_json_PrintUnformatted(msg);
        if (body == NULL) {
            DUER_LOGE("get string of msg failed");
            s_log_code = DUER_DS_LOG_BIND_NO_MEMORY;
            break;
        }
        DUER_LOGI("body = %s", body);

        body_len = DUER_STRLEN(body);

        // the length of header is 16 bytes
        *len = body_len + 16;
        buf = (uint8_t *)DUER_MALLOC(*len);
        if (buf == NULL) {
            DUER_LOGE("No free memory");
            s_log_code = DUER_DS_LOG_BIND_NO_MEMORY;
            break;
        }
        *outbuf = buf;

        // magic number
        DUER_MEMCPY(buf, AIRKISS_MAGIC_NUMBER, sizeof(AIRKISS_MAGIC_NUMBER));
        buf += 4;

        // version code
        DUER_MEMCPY(buf, AIRKISS_SEND_VERSION, sizeof(AIRKISS_SEND_VERSION));
        buf += 4;

        // the length of message
        buf[0] = buf[1] = 0;
        buf[2] = (uint8_t)((*len) >> 8);
        buf[3] = (uint8_t)(*len);
        buf += 4;

        // command id
        buf[0] = (uint8_t)(cmdid >> 24);
        buf[1] = (uint8_t)(cmdid >> 16);
        buf[2] = (uint8_t)(cmdid >> 8);
        buf[3] = (uint8_t)cmdid;
        buf += 4;

        DUER_MEMCPY(buf, body, body_len);
        ret = 0;
    } while (0);

    if (body) {
        baidu_json_release(body);
        body = NULL;
    }

    if (msg) {
        baidu_json_Delete(msg);
        msg = NULL;
    }

    return ret;
}

static void duer_bind_device(int what, void *object)
{
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    const uint16_t BUF_LEN = 200;
    uint16_t resp_len = 0;
    uint16_t recv_len = 0;
    uint8_t *buf = NULL;
    uint8_t *resp_buf = NULL;
    int ret = 0;
    int err = 0;
    int fd = -1;
    int optval = 1;
    int req_port = 0;
    int resp_success_count = 0;
    int resp_fail_count = 0;

    duer_ds_log_bind(DUER_DS_LOG_BIND_TASK_START);
    s_log_code = DUER_DS_LOG_BIND_TASK_END;

    do {
        buf = (uint8_t *)DUER_MALLOC(BUF_LEN);
        if (buf == NULL) {
            DUER_LOGE("buf allocate fail");
            s_log_code = DUER_DS_LOG_BIND_NO_MEMORY;
            break;
        }

        s_aes_ctx = duer_aes_context_init();
        if (s_aes_ctx == NULL) {
            DUER_LOGE("init aes context failed");
            s_log_code = DUER_DS_LOG_BIND_INIT_AES_FAILED;
            break;
        }
        duer_aes_setkey(s_aes_ctx, (unsigned char *)AES_KEY, 128);

        fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); //IPPROTO_UDP
        if (fd < 0) {
            DUER_LOGE("failed to create socket!");
            s_log_code = DUER_DS_LOG_BIND_INIT_SOCKET_FAILED;
            break;
        }

        ret = setsockopt(fd, SOL_SOCKET, SO_BROADCAST, (void*)&optval, sizeof(optval));
        if (ret < 0) {
            DUER_LOGE("setsockopt failed");
            s_log_code = DUER_DS_LOG_BIND_INIT_SOCKET_FAILED;
            break;
        }

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        local_addr.sin_port = htons(AIRKISS_DEFAULT_LAN_PORT);

        ret = bind(fd, (const struct sockaddr *)&local_addr, sizeof(local_addr));
        if (ret < 0) {
            err = duer_get_socket_errno(fd);
            DUER_LOGE("airkiss bind local port ERROR! errno %d", err);
            s_log_code = DUER_DS_LOG_BIND_BIND_SOCKET_FAILED;
            break;
        }

        ret = duer_airkiss_pack_device_info(DUER_AIRKISS_SSDP_RESP_CMD, &resp_buf, &resp_len);
        if (ret < 0) {
            DUER_LOGE("Pack device info packet error!");
            break;
        }

        while (!s_stop_send) {
            struct timeval tv;
            fd_set rfds;

            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);

            tv.tv_sec = 1;
            tv.tv_usec = 0;

            ret = select(fd + 1, &rfds, NULL, NULL, &tv);
            if (ret > 0) {
                recv_len = recvfrom(fd, buf, BUF_LEN, 0, (struct sockaddr *)&remote_addr,
                                        (socklen_t *)&addr_len);
                if (recv_len <= 0) {
                    continue;
                }

                duer_airkiss_ret_t lan_ret = duer_airkiss_lan_recv(buf, recv_len);
                switch (lan_ret) {
                case DUER_AIRKISS_SSDP_REQ:
                    DUER_LOGD("DUER_AIRKISS_SSDP_REQ");
                    req_port = ntohs(remote_addr.sin_port);
                    ret = sendto(fd, resp_buf, resp_len, 0,
                            (struct sockaddr *)&remote_addr, addr_len);
                    if (ret < 0) {
                        err = duer_get_socket_errno(fd);
                        DUER_LOGE("send notify msg ERROR! errno %d", err);
                        s_log_code = DUER_DS_LOG_BIND_SOCKET_ERROR;
                        resp_fail_count++;
                    } else {
                        DUER_LOGI("send notify msg OK!");
                        if (resp_success_count++ == 0) {
                            duer_ds_log_bind(DUER_DS_LOG_BIND_SEND_RESP);
                        }
                    }
                    break;
                default:
                    DUER_LOGD("Pack is not ssdp req!");
                    break;
                }
            } else {
                DUER_LOGD("recv nothing!");
            }
        }
    } while(0);

    duer_ds_log_report_bind_result(req_port, resp_success_count, resp_fail_count);

    if (fd > -1) {
        close(fd);
        fd = -1;
    }

    if (buf) {
        DUER_FREE(buf);
        buf = NULL;
    }

    if (resp_buf) {
        DUER_FREE(resp_buf);
        resp_buf = NULL;
    }

    if (s_aes_ctx) {
        duer_aes_context_destroy(s_aes_ctx);
        s_aes_ctx = NULL;
    }

    if (s_device_uuid != NULL) {
        DUER_FREE(s_device_uuid);
        s_device_uuid = NULL;
    }

    if (s_bind_token != NULL) {
        DUER_FREE(s_bind_token);
        s_bind_token = NULL;
    }

    duer_ds_log_bind(s_log_code);
}

static void duer_stop_timer_callback(void *param)
{
    duer_stop_bind_device_task();
}

duer_status_t duer_start_bind_device_task(const char *uuid,
        const char *bind_token, size_t lifecycle)
{
    int ret = DUER_ERR_FAILED;
    int len = 0;
    if (uuid == NULL || uuid[0] == 0 || bind_token == NULL || bind_token[0] == 0) {
        DUER_LOGE("invalid uuid or bind_token");
        duer_ds_log_bind(DUER_DS_LOG_BIND_INVALID_PARAMS);
        return DUER_ERR_FAILED;
    }

    if (lifecycle < 60) {
        DUER_LOGE("the lifecycle is too short");
        duer_ds_log_bind(DUER_DS_LOG_BIND_INVALID_PARAMS);
        return DUER_ERR_FAILED;
    }

    if (s_events != NULL) {
        DUER_LOGE("a task has been started!");
        duer_ds_log_bind(DUER_DS_LOG_BIND_START_TASK_FAILED);
        return DUER_ERR_FAILED;
    }

    do {
        len = DUER_STRLEN(uuid) + 1;
        s_device_uuid = (char *)DUER_MALLOC(len);
        if (s_device_uuid == NULL) {
            DUER_LOGE("no free memory");
            s_log_code = DUER_DS_LOG_BIND_NO_MEMORY;
            break;
        }
        DUER_MEMCPY(s_device_uuid, uuid, len);

        len = DUER_STRLEN(bind_token) + 1;
        s_bind_token = (char *)DUER_MALLOC(len);
        if (s_bind_token == NULL) {
            DUER_LOGE("no free memory");
            s_log_code = DUER_DS_LOG_BIND_NO_MEMORY;
            break;
        }
        DUER_MEMCPY(s_bind_token, bind_token, len);

        s_stop_timer = duer_timer_acquire(duer_stop_timer_callback, NULL, DUER_TIMER_ONCE);
        if (s_stop_timer == NULL) {
            DUER_LOGE("create timer failed");
            s_log_code = DUER_DS_LOG_BIND_START_TASK_FAILED;
            break;
        }

        s_events = duer_events_create("bind_device", STACK_SIZE, 1);
        if (s_events == NULL) {
            DUER_LOGE("create events failed");
            s_log_code = DUER_DS_LOG_BIND_START_TASK_FAILED;
            break;
        }

        s_stop_send = false;
        duer_events_call(s_events, duer_bind_device, 0, NULL);

        duer_timer_start(s_stop_timer, lifecycle * 1000);

        ret = DUER_OK;
    } while(0);

    if (ret != DUER_OK) {
        if (s_device_uuid != NULL) {
            DUER_FREE(s_device_uuid);
            s_device_uuid = NULL;
        }

        if (s_bind_token != NULL) {
            DUER_FREE(s_bind_token);
            s_bind_token = NULL;
        }

        if (s_stop_timer) {
            duer_timer_release(s_stop_timer);
            s_stop_timer = NULL;
        }

        duer_ds_log_bind(s_log_code);
    }

    return DUER_OK;
}

duer_status_t duer_stop_bind_device_task(void)
{
    if (s_events != NULL) {
        s_stop_send = true;

        duer_timer_stop(s_stop_timer);
        duer_timer_release(s_stop_timer);
        s_stop_timer = NULL;

        duer_events_destroy(s_events);
        s_events = NULL;
    }

    return DUER_OK;
}
