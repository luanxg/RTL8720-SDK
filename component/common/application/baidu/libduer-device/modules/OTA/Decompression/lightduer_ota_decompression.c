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
 * File: lightduer_ota_decompression.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA decompression abstraction layer
 */

#include "lightduer_ota_decompression.h"
#include "lightduer_ota_zlib.h"
#include <stdint.h>
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"

static duer_mutex_t s_lock_ota_decom;
static duer_ota_decompression_t *s_decompression[DECOMPRESSION_MAX];

int duer_ota_init_decompression(void)
{
    int i = 0;
    int ret = DUER_OK;

    if (s_lock_ota_decom == NULL) {
        s_lock_ota_decom = duer_mutex_create();
        if (s_lock_ota_decom == NULL) {
            DUER_LOGE("OTA Decompression: Create lock failed");

            ret = DUER_ERR_FAILED;

            return ret;
        }
    }

    for (i = 0; i < DECOMPRESSION_MAX; i++) {
        s_decompression[i] = NULL;
    }

    ret = duer_ota_init_zlib();
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Init Zlib failed");
    }

    return ret;
}

duer_ota_decompression_t *duer_ota_unpacker_create_decompression(char const *name)
{
    duer_ota_decompression_t *decom = NULL;

    if (name == NULL || DUER_STRLEN(name) > NAME_LENGTH) {
        DUER_LOGE("OTA Decompression: Argument error");

        return NULL;
    }

    do {
        decom = DUER_MALLOC(sizeof(*decom));
        if (decom == NULL) {
            DUER_LOGE("OTA Decompression: Malloc failed");

            return NULL;
        }

        DUER_MEMSET(decom, 0, sizeof(*decom));

        DUER_MEMMOVE(decom->name, name, DUER_STRLEN(name));

        decom->lock = duer_mutex_create();
        if (decom->lock == NULL) {
            DUER_LOGE("OTA Decompression: Create mutex failed");

            break;
        }

        return decom;
    } while(0);

    if (decom != NULL) {
        DUER_FREE(decom);
    }

    return NULL;
}

int duer_ota_unpacker_destroy_decompression(duer_ota_decompression_t *decompression)
{
    int ret = DUER_OK;

    if (decompression == NULL) {
        DUER_LOGE("OTA Decompression: Argument error");

        return DUER_ERR_FAILED;
    }

    if (decompression->lock != NULL) {
        ret = duer_mutex_destroy(decompression->lock);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Decompression: Argument error");

            return ret;
        }
    }

    DUER_FREE(decompression);

    return DUER_OK;
}

const char *duer_ota_unpacker_get_decompression_name(
        duer_ota_decompression_t const *decompression)
{
    if (decompression == NULL) {
        DUER_LOGE("OTA Decompression: Argument error");

        return NULL;
    }

    return decompression->name;
}

duer_ota_decompression_t *duer_ota_unpacker_get_decompression(duer_ota_decompress_type_t type)
{
    int ret = DUER_OK;
    duer_ota_decompression_t *decompression = NULL;

    if (type < 0 || type >= DECOMPRESSION_MAX) {
        DUER_LOGE("OTA Decompression: Argument error");

        return NULL;
    }

    ret = duer_mutex_lock(s_lock_ota_decom);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Lock failed");

        return NULL;
    }

    decompression = s_decompression[type];

    ret = duer_mutex_unlock(s_lock_ota_decom);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Unlock failed");

        return NULL;
    }

    return decompression;
}

int duer_ota_unpacker_register_decompression(
        duer_ota_decompression_t *decompression,
        duer_ota_decompress_type_t type)
{
    int ret = DUER_OK;

    if (decompression == NULL
            || decompression->init == NULL
            || decompression->unzip == NULL
            || decompression->unint == NULL
            || type >= DECOMPRESSION_MAX
            || s_decompression[type] != NULL) {
        DUER_LOGE("OTA Decompression: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(s_lock_ota_decom);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Lock failed");

        return ret;
    }

    s_decompression[type] = decompression;

    ret = duer_mutex_unlock(s_lock_ota_decom);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Unlock failed");

        return ret;
    }

    return ret;
}

int duer_ota_unpacker_unregister_decompression(duer_ota_decompress_type_t type)
{
    int ret = DUER_OK;
    int status = DUER_OK;

    if (type >= DECOMPRESSION_MAX || s_decompression[type] == NULL) {
        DUER_LOGE("OTA Decompression: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ret = duer_mutex_lock(s_lock_ota_decom);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Lock failed");

        return ret;
    }

    status = duer_ota_unpacker_destroy_decompression(s_decompression[type]);
    if (status != DUER_OK) {
        DUER_LOGE("OTA Decompression: Destroy decompression failed");
    } else {
        s_decompression[type] = NULL;
    }

    ret = duer_mutex_unlock(s_lock_ota_decom);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Unlock failed");

        return ret;
    }

    return status;
}

int duer_ota_unpacker_init_decompression(duer_ota_decompression_t *decompression)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (decompression == NULL || decompression->init == NULL) {
        DUER_LOGE("OTA Decompression: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    err = duer_mutex_lock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Lock failed");

        return err;
    }

    ret = decompression->init(decompression, &decompression->pdata);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Init decompression failed");
    }

    err = duer_mutex_unlock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Unlock failed");

        return err;
    }

    return ret;
}

int duer_ota_unpacker_unzip(
        duer_ota_decompression_t *decompression,
        char const *compressed_data,
        size_t data_size,
        void *unpacker,
        DECOMPRESS_DATA_HANDLER_FUN data_handler)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (decompression == NULL
            || unpacker == NULL
            || decompression->unzip == NULL
            || data_handler == NULL
            || compressed_data == NULL
            || data_size <= 0) {
        DUER_LOGE("OTA Decompression: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    err = duer_mutex_lock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Lock failed");

        return err;
    }

    ret = decompression->unzip(
            decompression,
            decompression->pdata,
            compressed_data,
            data_size,
            unpacker,
            data_handler);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Unzip failed");
    }

    err = duer_mutex_unlock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Unlock failed");

        return err;
    }

    return ret;
}

int duer_ota_unpacker_uninit_decompression(duer_ota_decompression_t *decompression)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (decompression == NULL || decompression->unint == NULL) {
        DUER_LOGE("OTA Decompression: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    err = duer_mutex_lock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Lock failed");

        return err;
    }

    ret = decompression->unint(decompression, decompression->pdata);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Init decompression failed");
    }

    err = duer_mutex_unlock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Unlock failed");

        return err;
    }

    return ret;
}

int duer_ota_unpacker_recovery_decompression(duer_ota_decompression_t *decompression)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (decompression == NULL || decompression->recovery == NULL) {
        DUER_LOGE("OTA Decompression: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    err = duer_mutex_lock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Lock failed");

        return err;
    }

    ret = decompression->recovery(decompression, decompression->pdata);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Recovery decompression failed");
    }

    err = duer_mutex_unlock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Unlock failed");

        return err;
    }

    return ret;
}

int duer_ota_unpacker_sync_decompression(duer_ota_decompression_t *decompression)
{
    int ret = DUER_OK;
    int err = DUER_OK;

    if (decompression == NULL || decompression->sync == NULL) {
        DUER_LOGE("OTA Decompression: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    err = duer_mutex_lock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Lock failed");

        return err;
    }

    ret = decompression->sync(decompression, decompression->pdata);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Decompression: Sync decompression failed");
    }

    err = duer_mutex_unlock(decompression->lock);
    if (err != DUER_OK) {
        DUER_LOGE("OTA Decompression: Unlock failed");

        return err;
    }

    return ret;
}

char const *duer_ota_decompression_get_err_msg(duer_ota_decompression_t const *decompression)
{

    if (decompression == NULL) {
        DUER_LOGE("OTA Decompression: Argument error");

        return NULL;
    }

    return decompression->err_msg;
}
