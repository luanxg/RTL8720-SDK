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
 * File: lightduer_dev_info.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: Device Information
 */

#include "lightduer_dev_info.h"
#include <string.h>
#include <stdint.h>
#include "baidu_json.h"
#include "lightduer_log.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "lightduer_connagent.h"
#include "lightduer_system_info.h"

static struct DevInfoOps *s_dev_info_ops = NULL;
volatile static duer_mutex_t s_lock_dev_info = NULL;

int duer_register_device_info_ops(struct DevInfoOps *ops)
{
    if (ops == NULL || ops->get_firmware_version == NULL) {
        DUER_LOGE("Dev Info: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_lock_dev_info == NULL) {
        s_lock_dev_info = duer_mutex_create();
        if (s_lock_dev_info == NULL) {
            DUER_LOGE("Dev Info: Create mutex failed");

            return DUER_ERR_MEMORY_OVERLOW;
        }
    }

    s_dev_info_ops = ops;

    return DUER_OK;
}

int duer_report_device_info(void)
{
    int ret = DUER_OK;
    baidu_json *data = NULL;
    baidu_json *device_info = NULL;
    char firmware_version[FIRMWARE_VERSION_LEN + 1];

    if (s_dev_info_ops == NULL) {
        DUER_LOGE("Dev Info: No dev info ops");

        ret = DUER_ERR_INVALID_PARAMETER;

        goto out;
    }

    ret = duer_get_firmware_version(firmware_version, FIRMWARE_VERSION_LEN);
    if (ret != DUER_OK) {
        DUER_LOGE("Dev Info: Get firmware version failed");

        goto out;
    }

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_LOGE("Dev Info: Create json object failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto out;
    }

    device_info = baidu_json_CreateObject();
    if (device_info == NULL) {
        DUER_LOGE("Dev Info: Create json object failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto out;
    }

    baidu_json_AddStringToObjectCS(device_info, "firmware_version", (char *)&firmware_version);
    baidu_json_AddItemToObjectCS(data, "device_info", device_info);

    ret = duer_data_report(data);
    if (ret != DUER_OK) {
        DUER_LOGE("Dev Info: Report device info failed ret: %d", ret);
    }
out:
    if (data != NULL) {
    /*
     * Json object use linked list to link related json objects
     * and, it will delete all json objects which linked automatic
     * So, We do not need to delete them(data, device_info) one by one
     */
        baidu_json_Delete(data);
    }

    return ret;
}

int duer_get_firmware_version(char *firmware_version, size_t len)
{
    int ret = DUER_OK;
    int ret_lock = DUER_OK;

    if(firmware_version == NULL || len < FIRMWARE_VERSION_LEN) {
        return DUER_ERR_INVALID_PARAMETER;
    }

    ret_lock = duer_mutex_lock(s_lock_dev_info);
    if (ret_lock != DUER_OK) {
        DUER_LOGE("Dev Info: Lock failed");
        ret = DUER_ERR_FAILED;

        goto out;
    }

    if (s_dev_info_ops->get_firmware_version != NULL) {
        ret = s_dev_info_ops->get_firmware_version(firmware_version);
        if (ret != DUER_OK) {
            DUER_LOGE("Dev Info: Get firmware version failed");
        }
    } else {
        DUER_LOGE("Dev Info: Uninit device info ops");

        ret = DUER_ERR_FAILED;
    }

    ret_lock = duer_mutex_unlock(s_lock_dev_info);
    if (ret_lock != DUER_OK) {
        DUER_LOGE("Dev Info: Unlock failed");
    }
out:
    return ret;
}

int duer_set_firmware_version(char const *firmware_version)
{
    int ret = DUER_OK;
    int ret_lock = DUER_OK;

    if(firmware_version == NULL) {
        return DUER_ERR_INVALID_PARAMETER;
    }

    ret_lock = duer_mutex_lock(s_lock_dev_info);
    if (ret_lock != DUER_OK) {
        DUER_LOGE("Dev Info: Lock failed");
        ret = DUER_ERR_FAILED;

        goto out;
    }

    if (s_dev_info_ops->set_firmware_version != NULL) {
        ret = s_dev_info_ops->set_firmware_version(firmware_version);
        if (ret != DUER_OK) {
            DUER_LOGE("Dev Info: Set firmware version failed");
        }
    } else {
        DUER_LOGE("Dev Info: Uninit device info ops");

        ret = DUER_ERR_FAILED;
    }

    ret_lock = duer_mutex_unlock(s_lock_dev_info);
    if (ret_lock != DUER_OK) {
        DUER_LOGE("Dev Info: Unlock failed");
    }
out:
    return ret;
}
