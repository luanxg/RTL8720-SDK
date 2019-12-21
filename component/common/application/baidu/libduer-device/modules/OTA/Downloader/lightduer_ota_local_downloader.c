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
 * File: lightduer_ota_http_downloader.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA HTTP Downloader
 */

#include "lightduer_ota_downloader.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "lightduer_log.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"

struct LOCALPPrivateData {
    data_handler handler;
    void *private_data;
    FILE *file;
};

static int init_local_downloader(duer_ota_downloader_t *downloader)
{
    int ret = DUER_OK;
    struct LOCALPPrivateData *pdata = NULL;
    do {
        if (downloader == NULL) {
            DUER_LOGE("LOCAL Downloader: Argument Error");

            ret = DUER_ERR_INVALID_PARAMETER;

            break;
        }

        pdata = (struct LOCALPPrivateData *)DUER_MALLOC(sizeof(*pdata));
        if (pdata == NULL) {
            DUER_LOGE("LOCAL Downloader: Malloc failed");

            ret = DUER_ERR_MEMORY_OVERLOW;

            break;
        }
        DUER_MEMSET(pdata, 0, sizeof(*pdata));
        pdata->file  = NULL;
        downloader->private_data = (void *)pdata;
    } while(0);

    return ret;
}
#define INPUT_BUF_LEN 1025
static int connect_local_server(duer_ota_downloader_t *downloader, const char *url)
{
    int ret = DUER_OK;
    struct LOCALPPrivateData *pdata = NULL;
    char buff[INPUT_BUF_LEN];

    do {
        if (downloader->private_data != NULL) {
            pdata = (struct LOCALPPrivateData *)downloader->private_data;
        } else {
            DUER_LOGE("LOCAL Downloader: There is no local client");

            ret = DUER_ERR_INVALID_PARAMETER;
            break;
        }
        DUER_LOGI("url:%s", url);
        pdata->file = fopen(url, "r");

        if (!pdata->file) {
            DUER_LOGE("LOCAL Downloader: open file fail!!");
            ret = DUER_ERR_INVALID_PARAMETER;
            break;
        }
        DUER_MEMSET(buff, 0, INPUT_BUF_LEN);
        // ret = fread(buff, 1, sizeof(duer_ota_package_header_t), pdata->file);
        // if (ret != sizeof(duer_ota_package_header_t)) {
        //     DUER_LOGI("read header error: %d", ret);
        //     ret = -1;
        //     goto out;
        // }
        int count = 0;
        int read_len = 0;
        do {
            read_len = fread(buff, 1, INPUT_BUF_LEN - 1, pdata->file);
            if (read_len > 0) {
                ret = (pdata->handler)(pdata->private_data, buff, read_len);
                if (ret < 0) {
                    DUER_LOGI("read error: ret = %d", ret);
                    break;
                }
                ++count;
                // DUER_LOGI("len:%d, count:%d", read_len, count);
                // for(int i = 0; i < 16; ++i) {
                //     printf("%02x", buff[i]);
                // }
                // printf("\n");
            }
        } while(read_len > 0);
    } while(0);

    return ret;
}

static int disconnect_local_server(duer_ota_downloader_t *downloader)
{
    int ret = DUER_OK;

    if (downloader == NULL) {
        DUER_LOGE("LOCAL Downloader: Argument Error");

        ret = DUER_ERR_INVALID_PARAMETER;
    }

    return ret;
}

static int register_data_notify(duer_ota_downloader_t *downloader,
                                data_handler handler, void *private_data)
{
    int ret = DUER_OK;
    struct LOCALPPrivateData *pdata = NULL;

    do {
        if (downloader == NULL || handler == NULL) {
            DUER_LOGE("LOCAL Downloader: Argument Error");

            ret = DUER_ERR_INVALID_PARAMETER;

            break;
        }

        if (downloader->private_data != NULL) {
            pdata = (struct LOCALPPrivateData *)downloader->private_data;
        } else {
            DUER_LOGE("LOCAL Downloader: Uninitialized local Downloader");

            ret = DUER_ERR_INVALID_PARAMETER;
            break;
        }

        pdata->handler = handler;
        pdata->private_data = private_data;
    } while(0);

    return ret;
}

static int destroy_local_downloader(duer_ota_downloader_t *downloader)
{
    int ret = DUER_OK;
    struct LOCALPPrivateData *pdata = NULL;

    do {
        if (downloader->private_data != NULL) {
            pdata = (struct LOCALPPrivateData *)downloader->private_data;
        } else {
            DUER_LOGE("LOCAL Downloader: There is no local private data");

            ret = DUER_ERR_INVALID_PARAMETER;
            break;
        }
        if (pdata->file) {
            fclose(pdata->file);
            pdata->file = NULL;
        }

        DUER_FREE(pdata);
    } while(0);

    return ret;
}

static duer_ota_downloader_ops_t s_ops = {
    .init                 = init_local_downloader,
    .connect_server       = connect_local_server,
    .disconnect_server    = disconnect_local_server,
    .register_data_notify = register_data_notify,
    .destroy              = destroy_local_downloader,
};

int duer_init_local_downloader(void)
{
    int ret = DUER_OK;
    duer_ota_downloader_t *downloader = NULL;
    do {
        downloader = duer_ota_downloader_create_downloader();
        if (downloader == NULL) {
            DUER_LOGE("OTA(LOCAL): Create downloader failed");

            break;
        }

        ret = duer_ota_downloader_register_downloader_ops(downloader, &s_ops);
        if (ret != DUER_OK) {
            DUER_LOGE("LOCAL Downloader: Register downloader ops failed ret:%d", ret);

            break;
        }

        ret = duer_ota_downloader_register_downloader(downloader, LOCAL);
        if (ret != DUER_OK) {
            DUER_LOGE("LOCAL Downloader: Register LOCAL downloader ret:%d", ret);

            break;
        }

        return ret;
    } while(0);

    duer_ota_downloader_destroy_downloader(downloader);

    return ret;
}
