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
/*
 * File: lightduer_ota_updater.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA Updater
 */

#include "lightduer_ota_updater.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "baidu_json.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_sleep.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "lightduer_events.h"
#include "lightduer_connagent.h"
#include "lightduer_http_client.h"
#include "lightduer_ota_notifier.h"
#include "lightduer_ota_downloader.h"
#include "mbedtls/md5.h"

#define STACK_SIZE        (10 * 1024)
#define QUEUE_LENGTH      (1)
#define REBOOT_DELAY_TIME (10 * 1000)

static duer_ota_init_ops_t const *s_ota_init_ops = NULL;

volatile static duer_ota_switch s_ota_switch = ENABLE_OTA;
volatile static duer_ota_reboot s_ota_reboot = ENABLE_REBOOT;
volatile static duer_mutex_t s_lock_ota_updater = NULL;
volatile static duer_events_handler s_ota_updater_handler = NULL;
static duer_ota_update_command_t const *s_ota_update_cmd = NULL;

static int notify_package_info(duer_ota_updater_t const *updater)
{
    int ret = DUER_OK;
    duer_package_info_t package_info;
    baidu_json *data_json = NULL;
    baidu_json *os_info_json = NULL;
    baidu_json *package_info_json = NULL;

    DUER_MEMSET(&package_info, 0, sizeof(package_info));

    ret = duer_ota_get_package_info(&package_info);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Notifier: Get package info failed");

        goto out;
    }

    package_info_json = baidu_json_CreateObject();
    if (package_info_json == NULL) {
        DUER_LOGE("OTA Notifier: Create json object failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto out;
    }

    os_info_json = baidu_json_CreateObject();
    if (os_info_json == NULL) {
        DUER_LOGE("OTA Notifier: Create json object failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto delete_package_json;
    }

    data_json = baidu_json_CreateObject();
    if (data_json == NULL) {
        DUER_LOGE("OTA Notifier: Create json object failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto delete_osinfo_json;
    }

    baidu_json_AddStringToObject(os_info_json, "name", (char *)&package_info.os_info.os_name);
    baidu_json_AddStringToObject(
            os_info_json,
            "developer",
            (char *)&package_info.os_info.developer);
    baidu_json_AddStringToObject(
            os_info_json,
            "staged_version",
            (char *)&package_info.os_info.staged_version);
    baidu_json_AddStringToObject(os_info_json, "version", (char *)&updater->update_cmd->version);
    baidu_json_AddItemToObject(package_info_json, "os", os_info_json);

    baidu_json_AddStringToObject(package_info_json, "version", OTA_PROTOCOL_VERSION);
    baidu_json_AddStringToObject(package_info_json, "product", (char *)&package_info.product);
    baidu_json_AddStringToObject(package_info_json, "batch", (char *)&package_info.batch);
    baidu_json_AddItemToObject(data_json, "package_info", package_info_json);

    ret = duer_data_report(data_json);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Notifier: Report package info failed ret: %d", ret);
    }

    /*
     * Json object use linked list to link related json objects
     * and, it will delete all json objects which linked automatic
     * So, We do not need to delete them(os_info_json, package_info_json, data_json) one by one
     */
    baidu_json_Delete(data_json);

    return ret;
delete_osinfo_json:
    baidu_json_Delete(os_info_json);
delete_package_json:
    baidu_json_Delete(package_info_json);
out:
    return ret;
}

static int package_handler(void *private, char const *buf, size_t len)
{
    int ret = DUER_OK;
    int err = DUER_OK;
    double progress = 0.0;
    double total_data_size = 0.0;
    double received_data_size = 0.0;
    duer_ota_updater_t *updater = NULL;

    if (private == NULL) {
        DUER_LOGE("OTA Updater: Argument Error");

        return DUER_ERR_FAILED;
    }
    updater = (duer_ota_updater_t *)private;

    updater->received_data_size += len;
    received_data_size = updater->received_data_size;
    total_data_size = updater->update_cmd->size;
    progress = received_data_size / total_data_size;

    mbedtls_md5_update(&updater->md5_ctx, buf, len);

    do {
        if (buf != NULL && len > 0) {
            ret = duer_ota_unpacker_unpack_package(updater->unpacker, (uint8_t const *)buf, len);
            if (ret != DUER_OK) {
                DUER_LOGE("OTA Updater: Unpack OTA package failed ret: %d", ret);

                break;
            }

            if (((int)(progress * 10) % 10) >= updater->progress) {
                updater->progress++;

                ret = duer_ota_notifier_event(updater, OTA_EVENT_DOWNLOADING, progress, NULL);
                if (ret != DUER_OK) {
                    DUER_LOGE("OTA Updater: Notifier OTA download progress failed ret: %d", ret);

                    break;
                }
            }
        } else {
            /*
             * Some Downloader will send a NULL pointer or 0 to indicate the
             * end of the transmission
             */
            DUER_LOGW("OTA Updater: HTTP Data length: %d, buf: %p", len, buf);
        }

        return ret;
    } while(0);

    err = duer_ota_notifier_event(updater, OTA_EVENT_UNPACK_FAIL, 0, NULL);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Updater: Notifier OTA unpack failed ret: %d", ret);
    }

    return ret;
}

int duer_ota_destroy_updater(duer_ota_updater_t *updater)
{
    int ret = DUER_OK;

    if (updater == NULL) {
        DUER_LOGE("OTA Updater: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (updater->unpacker != NULL) {
        DUER_LOGI("OTA Updater: Destroy unpacker");

        ret = duer_ota_unpacker_destroy_unpacker(updater->unpacker);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Destroy unpacker failed ret: %d", ret);

            goto out;
        }
        updater->unpacker = NULL;
    }

    if (updater->downloader != NULL) {
        ret = duer_ota_downloader_destroy_downloader(updater->downloader);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Destroy downloader failed ret: %d", ret);

            goto out;
        }
        updater->downloader = NULL;

        DUER_LOGI("OTA Updater: Destroy downloader");
    }

    if (updater->update_cmd != NULL) {
        DUER_FREE(updater->update_cmd);
        s_ota_update_cmd = NULL;
    }

    mbedtls_md5_free(&updater->md5_ctx);

    DUER_FREE(updater);

    DUER_LOGI("OTA Updater: Destroy updater");
out:
    return ret;
}

// To support multiple-updater
duer_ota_updater_t *duer_ota_get_updater(int id)
{
    return NULL;
}

static int duer_ota_create_updater(
        OTA_Updater ota_updater,
        duer_ota_update_command_t const *update_cmd)
{
    int id = 1; // ID for Updater (Unused)
    int ret = DUER_OK;
    int tmp = DUER_OK;
    duer_ota_updater_t *updater = NULL;
    duer_ota_update_command_t *cmd = NULL;

    if (update_cmd == NULL) {
        DUER_LOGE("OTA Updater: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    do {
        cmd = DUER_MALLOC(sizeof(*cmd));
        if (cmd == NULL) {
            DUER_LOGE("OTA Updater: Malloc cmd failed");

            ret = DUER_ERR_MEMORY_OVERLOW;

            break;
        }

        DUER_MEMMOVE(cmd, update_cmd, sizeof(*cmd));
        s_ota_update_cmd = cmd;

        updater = DUER_MALLOC(sizeof(*updater));
        if (updater == NULL) {
            DUER_LOGE("OTA Updater: Malloc updater failed");

            ret = DUER_ERR_MEMORY_OVERLOW;

            break;
        }

        DUER_MEMSET(updater, 0, sizeof(*updater));

        updater->update_cmd = cmd;
        updater->received_data_size = 0;
        updater->progress = 1;

        if (s_ota_updater_handler == NULL) {
            s_ota_updater_handler = duer_events_create(
                    "lightduer_OTA_Updater",
                    STACK_SIZE,
                    QUEUE_LENGTH);
            if (s_ota_updater_handler == NULL) {
                DUER_LOGE("OTA Updater: Create OTA Updater failed");

                break;
            }
        }

        ret = duer_events_call(s_ota_updater_handler, ota_updater, id, updater);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Call Updater failed");

            ret = DUER_ERR_FAILED;

            break;
        }

        return ret;
    } while (0);

    tmp = duer_ota_notifier_event(updater, OTA_EVENT_REJECT, 0, "Create Updater failed");
    if (tmp != DUER_OK) {
        DUER_LOGE("OTA Updater: Notifier OTA begin failed ret: %d", ret);
    }

    if (s_ota_updater_handler != NULL) {
        duer_events_destroy(s_ota_updater_handler);
    }

    if (updater != NULL) {
        DUER_FREE(updater);
    }

    if (cmd != NULL) {
        DUER_FREE(cmd);
    }

    return ret;
}

static duer_downloader_protocol get_download_protocol(char const *url, size_t len)
{
    /*
     * Now the Duer Cloud use HTTP protocol
     * So we only need to support it
     * Implement this function in order to expand
     */

    return HTTP;
}

static void ota_updater(int arg, void *ota_updater)
{
    int i = 0;
    int ret = DUER_OK;
    char md5_buf[33];
    size_t buf_len = 33;
    unsigned char md5[16];
    char *p_buf = md5_buf;
    char const *updater_err_msg = NULL;
    char const *unpacker_err_msg = NULL;
    duer_ota_event event;
    duer_downloader_protocol dp;
    duer_ota_updater_t *updater       = NULL;
    duer_ota_unpacker_t *unpacker     = NULL;
    duer_ota_downloader_t *downloader = NULL;

    if (ota_updater == NULL) {
        DUER_LOGE("OTA Updater: Argument Error");

        goto out;
    }

    DUER_LOGI("OTA Updater: Start OTA Update");

    updater = (duer_ota_updater_t *)ota_updater;

    ret = duer_ota_notifier_event(updater, OTA_EVENT_BEGIN, 0, "Creater Updater success");
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Notifier OTA begin failed ret: %d", ret);

        goto destroy_updater;
    }

    unpacker = duer_ota_unpacker_create_unpacker();
    if (unpacker == NULL) {
        DUER_LOGE("OTA Updater: Create unpacker failed");

        event = OTA_EVENT_REJECT;

        updater_err_msg = "Create unpacker failed";

        goto notify_ota_event;
    }
    updater->unpacker = unpacker;

    dp = get_download_protocol(updater->update_cmd->url, URL_LEN);
    if (dp < 0 || dp >= MAX_PROTOCOL_COUNT) {
        DUER_LOGE("OTA Updater: Do support the download protocol");

        event = OTA_EVENT_REJECT;

        updater_err_msg = "Do not support download protocol";

        goto notify_ota_event;
    }

    downloader = duer_ota_downloader_get_downloader(dp);
    if (downloader == NULL) {
        DUER_LOGE("OTA Updater: Get OTA downloader failed ret: %d", ret);

        event = OTA_EVENT_REJECT;

        updater_err_msg = "Get OTA downloader failed";

        goto notify_ota_event;
    }
    updater->downloader = downloader;

    ret = duer_ota_downloader_init_downloader(downloader);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Init OTA downloader failed ret: %d", ret);

        event = OTA_EVENT_REJECT;

        updater_err_msg = "Init OTA downloader failed";

        goto notify_ota_event;
    }

    ret = duer_ota_downloader_register_data_notify(downloader, package_handler, updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Register data handler failed ret: %d", ret);

        event = OTA_EVENT_REJECT;

        updater_err_msg = "Register package handler failed";

        goto notify_ota_event;
    }

    DUER_LOGI("OTA Updater: URL: %s", updater->update_cmd->url);

    mbedtls_md5_init(&updater->md5_ctx);
    mbedtls_md5_starts(&updater->md5_ctx);

    ret = duer_ota_downloader_connect_server(downloader, updater->update_cmd->url);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Connetc server failed ret: %d", ret);

        event = OTA_EVENT_CONNECT_FAIL;
        updater_err_msg = "Connect server failed";

        goto notify_ota_event;
    }

    mbedtls_md5_finish(&updater->md5_ctx, md5);
    mbedtls_md5_free(&updater->md5_ctx);

    for (i = 0; i < 16; i++) {
        buf_len -= DUER_SNPRINTF(p_buf, buf_len, "%02x", md5[i]);
        p_buf += 2;
    }
    *p_buf = '\0';

    ret = DUER_STRNCMP(updater->update_cmd->signature, (const char *)md5_buf, 32);
    if (ret != 0) {
        DUER_LOGE("OTA Updater: Checksum error failed");
        DUER_LOGE("OTA Updater: MD5: %s", updater->update_cmd->signature);
        DUER_LOGE("OTA Updater: Checksum MD5: %s", md5_buf);

        event = OTA_EVENT_IMAGE_INVALID;

        updater_err_msg = "Checksum failed";

        goto notify_ota_event;
    }

    ret = duer_ota_notifier_event(updater, OTA_EVENT_DOWNLOAD_COMPLETE, 0, NULL);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Notifier OTA download complete failed ret: %d", ret);

        goto notify_ota_event;
    }

    ret = duer_ota_notifier_event(updater, OTA_EVENT_INSTALLED, 0, NULL);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Notifier OTA install complete failed ret: %d", ret);

        goto destroy_updater;
    }

    duer_sleep(REBOOT_DELAY_TIME);

    if (s_ota_init_ops->reboot != NULL) {
        if (s_ota_reboot == ENABLE_REBOOT) {
            ret = s_ota_init_ops->reboot(NULL);
            if (ret != DUER_OK) {
                DUER_LOGE("OTA Updater: Reboot failed ret: %d", ret);

                event = OTA_EVENT_INSTALLED;

                updater_err_msg = "Reboot failed";

                goto notify_ota_event;
            }
        } else {
            DUER_LOGE("OTA Updater: Disable Reboot");
        }
    } else {
        DUER_LOGE("OTA Updater: No Unpack OPS");

        event = OTA_EVENT_INSTALLED;

        updater_err_msg = "Reboot failed";

        goto notify_ota_event;
    }

    ret = notify_package_info(updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Notify package info failed ret: %d", ret);
    }

    goto destroy_updater;

notify_ota_event:
    unpacker_err_msg = duer_ota_unpacker_get_err_msg(unpacker);
    if (unpacker_err_msg != NULL) {
        ret = duer_ota_notifier_event(updater, event, 0, unpacker_err_msg);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Notifier OTA begin failed ret: %d", ret);
        }
    } else {
        ret = duer_ota_notifier_event(updater, event, 0, updater_err_msg);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Notifier OTA begin failed ret: %d", ret);
        }
    }
destroy_updater:
    duer_ota_destroy_updater(updater);

    //Edit by Realtek
    duer_sleep(15000);
    sys_reset();
	
out:
    duer_events_destroy(s_ota_updater_handler);

    s_ota_updater_handler = NULL;
}

static int duer_analyze_update_cmd(char const *update_cmd, size_t cmd_len, duer_ota_update_command_t *ota_update_cmd)
{
    int ret = DUER_OK;
    char *cmd_str = NULL;
    baidu_json *cmd = NULL;
    baidu_json *child = NULL;

    if (update_cmd == NULL || cmd_len <= 0) {
        DUER_LOGE("OTA Updater: Argument Error");

        ret = DUER_ERR_INVALID_PARAMETER;

        goto out;
    }

    cmd_str = (char *)DUER_MALLOC(cmd_len + 1);
    if (cmd_str == NULL) {
        DUER_LOGE("OTA Updater: Malloc failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto out;
    }

    DUER_MEMSET(cmd_str, 0, cmd_len + 1);
    DUER_MEMMOVE(cmd_str, update_cmd, cmd_len);

    DUER_LOGI("OTA Updater: Update command: %s Length: %d", cmd_str, cmd_len);

    cmd = baidu_json_Parse(cmd_str);
    if (cmd == NULL) {
        DUER_LOGE("OTA Updater: Invalid command: %s", cmd);

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto free_cmd_string;
    }

    DUER_MEMSET(ota_update_cmd, 0, sizeof(*ota_update_cmd));

    /*
     * This code is less ugly, but it is not security
     * The information of command will be cut off, if it is out of bounds
     * and there is no warning, I do not like it, but ...
     * We need to refactor the function later
     */

    child = cmd->child;
    child = child ? child->child : NULL;
    while (child) {
        if (DUER_STRCMP(child->string, "transaction") == 0) {
            DUER_MEMMOVE(ota_update_cmd->transaction, child->valuestring, sizeof(ota_update_cmd->transaction) - 1);
            DUER_LOGI("Transaction = %s", ota_update_cmd->transaction);
        } else if (DUER_STRCMP(child->string, "version") == 0) {
            DUER_MEMMOVE(ota_update_cmd->version, child->valuestring, sizeof(ota_update_cmd->version) - 1);
            DUER_LOGI("Version = %s", ota_update_cmd->version);
        } else if (DUER_STRCMP(child->string, "old_version") == 0) {
            DUER_MEMMOVE(ota_update_cmd->old_version, child->valuestring, sizeof(ota_update_cmd->old_version) - 1);
            DUER_LOGI("Old Version = %s", ota_update_cmd->old_version);
        } else if (DUER_STRCMP(child->string, "url") == 0) {
            DUER_MEMMOVE(ota_update_cmd->url, child->valuestring, sizeof(ota_update_cmd->url) - 1);
            DUER_LOGI("URL = %s", ota_update_cmd->url);
        } else if (DUER_STRCMP(child->string, "size") == 0) {
            ota_update_cmd->size = child->valueint;
            DUER_LOGI("Size = %d", ota_update_cmd->size);
        } else if (DUER_STRCMP(child->string, "signature") == 0) {
            DUER_MEMMOVE(ota_update_cmd->signature, child->valuestring, sizeof(ota_update_cmd->signature) - 1);
            DUER_LOGI("Signature = %s", ota_update_cmd->signature);
        } else {
            // Do nothing
        }

        child = child->next;
    }

    baidu_json_Delete(cmd);
free_cmd_string:
    DUER_FREE(cmd_str);
out:
    return ret;
}

static duer_status_t ota_update(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    int msg_code;
    duer_status_t ret = DUER_OK;
    duer_ota_update_command_t ota_update_cmd;

    duer_ota_switch ota_switch = duer_ota_get_switch();
    if (ota_switch == DISABLE_OTA) {
        // May be We need to tell the Duer Cloud that user do not want to run OTA
        DUER_LOGI("OTA Updater: OTA upgrade is prohibited");

        ret = DUER_ERR_FAILED;

        goto response_msg;
    }

    if (msg == NULL || msg->payload == NULL || msg->payload_len <= 0) {
        DUER_LOGE("OTA Updater: Update command error");

        ret = DUER_ERR_FAILED;

        goto response_msg;
    }

    ret = duer_analyze_update_cmd((const char *)(msg->payload), msg->payload_len, &ota_update_cmd);

    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Analyze update command failed");
    }
response_msg:
    msg_code = (ret == DUER_OK) ? DUER_MSG_RSP_CHANGED : DUER_MSG_RSP_BAD_REQUEST;

    duer_response(msg, msg_code, NULL, 0);

    if (ret == DUER_OK) {
        ret = duer_ota_create_updater(ota_updater, &ota_update_cmd);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Create updater failed ret: %d", ret);
        }
    }

    return ret;
}

int duer_init_ota(duer_ota_init_ops_t const *ops)
{
    int ret = DUER_OK;

    if (ops == NULL
       || ops->register_installer == NULL
       || ops->unregister_installer == NULL
       || ops->reboot == NULL) {
        DUER_LOGE("OTA Updater: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    static bool first_time = true;

    if (first_time) {
        duer_res_t res[] = {
            {
                DUER_RES_MODE_DYNAMIC,
                DUER_RES_OP_PUT,
                "newfirmware",
                .res.f_res = ota_update
            },
        };

        duer_add_resources(res, sizeof(res) / sizeof(res[0]));

        first_time = false;
    } else {
        return ret;
    }

    do {
        if (s_lock_ota_updater == NULL) {
            s_lock_ota_updater = duer_mutex_create();
            if (s_lock_ota_updater == NULL) {
                DUER_LOGE("OTA Updater: Create mutex failed");

                ret = DUER_ERR_MEMORY_OVERLOW;

                break;
            }
        }

        s_ota_init_ops = ops;

        ret = duer_init_ota_downloader();
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Init OTA downloader failed ret: %d", ret);

            break;
        }

        ret = duer_ota_init_decompression();
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Init OTA decompression failed ret: %d", ret);

            break;
        }

        ret = duer_ota_init_installer();
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Init OTA installer failed ret: %d", ret);

            break;
        }

        ret = ops->register_installer();
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Register OTA installer failed ret: %d", ret);

            break;
        }

        return ret;
    } while (0);

    if (s_lock_ota_updater  != NULL) {
        ret = duer_mutex_destroy(s_lock_ota_updater);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Updater: Destroy mutex failed ret: %d", ret);
        }
    }

    return ret;
}

int duer_ota_set_switch(duer_ota_switch flag)
{
    int ret = DUER_OK;

    if (flag != ENABLE_OTA && flag != DISABLE_OTA) {
        DUER_LOGE("OTA Updater: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_lock_ota_updater == NULL) {
        DUER_LOGE("OTA Updater: Uninit mutex");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_ota_updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Lock ota switch failed");

        goto out;
    }

    s_ota_switch = flag;

    ret = duer_mutex_unlock(s_lock_ota_updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Lock ota switch failed");

        goto out;
    }
out:
    return ret;
}

int duer_ota_get_switch(void)
{
    int ret;

    if (s_lock_ota_updater == NULL) {
        DUER_LOGE("OTA Updater: Uninit mutex");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_ota_updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Lock ota switch failed");

        return DUER_ERR_FAILED;
    }

    ret = s_ota_switch;

    ret = duer_mutex_unlock(s_lock_ota_updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Lock ota switch failed");

        return DUER_ERR_FAILED;
    }

    return ret;
}

int duer_ota_set_reboot(duer_ota_reboot reboot)
{
    int ret = DUER_OK;

    if (reboot != ENABLE_REBOOT && reboot != DISABLE_REBOOT) {
        DUER_LOGE("OTA Updater: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_lock_ota_updater == NULL) {
        DUER_LOGE("OTA Updater: Uninit mutex");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_ota_updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Lock ota switch failed");

        goto out;
    }

    s_ota_reboot = reboot;

    ret = duer_mutex_unlock(s_lock_ota_updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Lock ota switch failed");

        goto out;
    }
out:
    return ret;
}

int duer_ota_get_reboot(void)
{
    int ret;

    if (s_lock_ota_updater == NULL) {
        DUER_LOGE("OTA Updater: Uninit mutex");

        return DUER_ERR_FAILED;
    }

    ret = duer_mutex_lock(s_lock_ota_updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Lock ota switch failed");

        return DUER_ERR_FAILED;
    }

    ret = s_ota_reboot;

    ret = duer_mutex_unlock(s_lock_ota_updater);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Lock ota switch failed");

        return DUER_ERR_FAILED;
    }

    return ret;
}

duer_ota_update_command_t const *duer_ota_get_update_cmd(void)
{
    return s_ota_update_cmd;
}

int duer_ota_update_firmware(duer_ota_update_command_t const *update_cmd)
{
    int ret = DUER_OK;

    if (update_cmd == NULL) {
        DUER_LOGE("OTA Updater: Argument Error");

        ret = DUER_ERR_INVALID_PARAMETER;

        return ret;
    }

    ret = duer_ota_create_updater(ota_updater, update_cmd);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Updater: Create updater failed ret: %d", ret);

        ret = DUER_ERR_FAILED;
    }

    return ret;
}
