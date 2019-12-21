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
 * File: lightduer_http_client_ops.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: HTTP Client Socket OPS
 */

#include "lightduer_log.h"
#include "lightduer_http_client.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"
#include "lightduer_net_transport.h"
#include "lightduer_net_transport_wrapper.h"

static int socket_init(void *socket_args)
{
    duer_trans_handler *trans_handle = NULL;
    DUER_LOGV("%s", __func__);
    trans_handle = duer_trans_acquire(NULL, NULL);
    if (!trans_handle) {
        DUER_LOGE("socket is NULL!\n");
        return 0;
    }
    return (int)trans_handle;
}

static int socket_open(int socket_handle)
{
    return 0;
}

static int socket_connect(int socket_handle, const char *host, const int port)
{
    duer_trans_handler *trans_handle = (duer_trans_handler *)socket_handle;
    duer_status_t rs;
    duer_trans_ptr trans = (duer_trans_ptr)socket_handle;
    DUER_LOGI("%s", __func__);
    if (!trans_handle) {
        DUER_LOGE("socket is NULL!\n");
        return -1;
    }
    trans->addr.host = host;
    trans->addr.host_size = DUER_STRLEN(host);
    trans->addr.port = port;
    trans->addr.type = DUER_PROTO_TCP;
    rs = duer_trans_connect(trans_handle, &trans->addr);
    if (rs < 0 && rs != DUER_INF_TRANS_IP_BY_HTTP_DNS) {
        if (rs != DUER_ERR_TRANS_WOULD_BLOCK) {
            DUER_LOGE("HTTP OPS: Connect to server failed! errno=%d", rs);
        }
    } else {
        DUER_LOGI("HTTP OPS: Connected to server");
    }
    return rs;
}

static void socket_set_blocking(int socket_handle, int blocking)
{
}

static void socket_set_timeout(int socket_handle, int timeout)
{
    duer_trans_handler *trans_handle = (duer_trans_handler *)socket_handle;

    if (!trans_handle) {
        DUER_LOGE("socket is NULL!\n");
        return;
    }
    duer_trans_set_read_timeout(trans_handle, timeout);
}

static int socket_recv(int socket_handle, void *data, unsigned size)
{
    duer_trans_handler *trans_handle = (duer_trans_handler *)socket_handle;
    duer_status_t rs;

    if (!trans_handle) {
        DUER_LOGE("socket is NULL!\n");
        return -1;
    }

    DUER_MEMSET(data, 0x0, size);
    rs = duer_trans_recv_timeout(trans_handle, data, size, NULL);
    if (rs < 0) {
        if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
            DUER_LOGV("socket recv WOULD BLCK");
        } else if (rs == DUER_ERR_TRANS_TIMEOUT) {
            DUER_LOGV("socket recv TIMEOUT");
        } else {
            DUER_LOGV("socket recv abnormal!");
        }
    }

    return rs;
}

static int socket_send(int socket_handle, const void *data, unsigned size)
{
    duer_trans_handler *trans_handle = (duer_trans_handler *)socket_handle;
    duer_status_t rs;
    duer_trans_ptr trans = (duer_trans_ptr)socket_handle;
    DUER_LOGV("%s", __func__);

    if (!trans_handle) {
        DUER_LOGE("socket is NULL!\n");
        return -1;
    }

    rs = duer_trans_send(trans_handle, data, size, &trans->addr);
    if (rs < 0) {
        if(rs != DUER_ERR_TRANS_WOULD_BLOCK) {
            DUER_LOGE("HTTP OPS: Send failed %d", rs);
        }
    } else {
        DUER_LOGI("HTTP OPS: Send succeeded");
    }

    return rs;
}

static int socket_close(int socket_handle)
{
    duer_trans_handler *trans_handle = (duer_trans_handler *)socket_handle;
    duer_status_t rs;
    DUER_LOGV("%s", __func__);

    if (!trans_handle) {
        DUER_LOGE("socket is NULL!\n");
        return -1;
    }
    rs = duer_trans_close(trans_handle);
    if (rs < 0) {
        DUER_LOGE("HTTP OPS: Close failed %d", rs);
        return -1;
    }
    return 0;
}

static void socket_destroy(int socket_handle)
{
    duer_trans_handler *trans_handle = (duer_trans_handler *)socket_handle;
    duer_status_t rs;
    DUER_LOGV("%s", __func__);

    if (!trans_handle) {
        DUER_LOGE("socket is NULL!\n");
        return -1;
    }

    duer_trans_release(trans_handle);
    trans_handle = NULL;
}

static duer_http_socket_ops_t s_socket_ops = {
    .init         = socket_init,
    .open         = socket_open,
    .connect      = socket_connect,
    .set_blocking = socket_set_blocking,
    .set_timeout  = socket_set_timeout,
    .recv         = socket_recv,
    .send         = socket_send,
    .close        = socket_close,
    .destroy      = socket_destroy,
};

duer_http_client_t *duer_create_http_client(void)
{
    int ret = 0;
    duer_http_client_t *client = NULL;

    client = (duer_http_client_t *)DUER_MALLOC(sizeof(*client));

    if (client == NULL) {
        DUER_LOGE("OTA Downloader: Malloc failed");

        goto out;
    }

    ret = duer_http_init(client);

    if (ret != DUER_HTTP_OK) {
        DUER_LOGE("HTTP OPS:Init http client failed");

        goto init_client_err;
    }

    ret = duer_http_init_socket_ops(client, &s_socket_ops, NULL);

    if (ret != DUER_HTTP_OK) {
        DUER_LOGE("HTTP OPS:Init socket ops failed");

        goto init_socket_ops_err;
    }

    goto out;

init_socket_ops_err:
init_client_err:
    DUER_FREE(client);
    client = NULL;
out:
    return client;
}
