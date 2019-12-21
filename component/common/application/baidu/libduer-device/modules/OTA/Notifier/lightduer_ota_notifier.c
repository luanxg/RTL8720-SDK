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
 * File: lightduer_ota_notifier.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA Notifer
 */

#include "lightduer_ota_notifier.h"
#include <string.h>
#include <stdint.h>
#include "baidu_json.h"
#include "lightduer_log.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "lightduer_connagent.h"
#include "lightduer_ota_updater.h"

volatile static duer_mutex_t s_lock_notifier = NULL;
static duer_package_info_ops_t const *s_package_info_ops = NULL;

int duer_ota_notify_package_info(void)
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
    baidu_json_AddStringToObject(os_info_json, "version", (char *)&package_info.os_info.os_version);
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

int duer_ota_get_package_info(duer_package_info_t *info)
{
    int ret = DUER_OK;
    int ret_lock = DUER_OK;

    if (info == NULL) {
        return DUER_ERR_INVALID_PARAMETER;
    }

    ret_lock = duer_mutex_lock(s_lock_notifier);
    if (ret_lock != DUER_OK) {
        DUER_LOGE("OTA Notifier: Lock failed");
        ret = DUER_ERR_FAILED;

        goto out;
    }

    if (s_package_info_ops->get_package_info != NULL) {
        ret = s_package_info_ops->get_package_info(info);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Notifier: Get package info failed");
        }
    } else {
        DUER_LOGE("OTA Notifier: Uninit device info ops");

        ret = DUER_ERR_FAILED;
    }

    ret_lock = duer_mutex_unlock(s_lock_notifier);
    if (ret_lock != DUER_OK) {
        DUER_LOGE("OTA Notifier: Unlock failed");
    }
out:
    return ret;
}

int duer_ota_register_package_info_ops(duer_package_info_ops_t const *ops)
{
    if (ops == NULL || ops->get_package_info == NULL) {
        DUER_LOGE("OTA Notifier: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    if (s_lock_notifier == NULL) {
        s_lock_notifier = duer_mutex_create();
        if (s_lock_notifier == NULL) {
            DUER_LOGE("OTA Notifier: Create mutex failed");

            return DUER_ERR_MEMORY_OVERLOW;
        }
    }

    s_package_info_ops = ops;

    return DUER_OK;
}

// Now we only notifier Duer Cloud, We need to notifier the user the OTA state also (TBD)
int duer_ota_notifier_state(duer_ota_updater_t const *updater,  duer_ota_state state)
{
    int ret = DUER_OK;
    baidu_json *data_json = NULL;
    baidu_json *state_json = NULL;

    if (updater == NULL
        || state < OTA_IDLE
        || state > OTA_INSTALLED) {
        ret = DUER_ERR_INVALID_PARAMETER;

        DUER_LOGE("OTA Notifier: Argument Error");

        goto out;
    }

    data_json = baidu_json_CreateObject();
    if (data_json == NULL) {
        DUER_LOGE("OTA Notifier: Create json object failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto out;
    }

    state_json = baidu_json_CreateObject();
    if (state_json == NULL) {
        DUER_LOGE("OTA Notifier: Create json object failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto free_data_json;
    }

    baidu_json_AddStringToObject(state_json, "transaction", updater->update_cmd->transaction);
    baidu_json_AddNumberToObject(state_json, "state", state);
    baidu_json_AddItemToObject(data_json, "ota_state", state_json);

    ret = duer_data_report(data_json);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Notifier: Report OTA state failed ret: %d", ret);
    }

    /*
     * Json object use linked list to link related json objects
     * and, it will delete all json objects which linked automatic
     * So, We do not need to delete them(data_json, state_json) one by one
     */
    baidu_json_Delete(data_json);

    return ret;

free_data_json:
    baidu_json_Delete(data_json);
out:
    return ret;
}

// Now we only notifier Duer Cloud, We need to notifier the user the OTA event also (TBD)
int duer_ota_notifier_event(
        duer_ota_updater_t const *updater,
        duer_ota_event event,
        double progress,
        char const *description)
{
    int ret = DUER_OK;
    baidu_json *data_json = NULL;
    baidu_json *event_json = NULL;

    if (updater == NULL
        || event < OTA_EVENT_BEGIN
        || event > OTA_EVENT_MAX) {
        ret = DUER_ERR_INVALID_PARAMETER;

        DUER_LOGE("OTA Notifier: Argument Error");

        goto out;
    }

    data_json = baidu_json_CreateObject();
    if (data_json == NULL) {
        DUER_LOGE("OTA Notifier: Create json object failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto out;
    }

    event_json = baidu_json_CreateObject();
    if (event_json == NULL) {
        DUER_LOGE("OTA Notifier: Create json object failed");

        ret = DUER_ERR_MEMORY_OVERLOW;

        goto free_data_json;
    }

    baidu_json_AddItemToObject(data_json, "ota_event", event_json);
    baidu_json_AddStringToObject(event_json, "transaction", updater->update_cmd->transaction);
    if (description != NULL) {
        baidu_json_AddStringToObject(event_json, "description", description);
    } else {
        baidu_json_AddItemToObject(event_json, "description", baidu_json_CreateNull());
    }
    baidu_json_AddNumberToObject(event_json, "event", event);

    if (event == OTA_EVENT_DOWNLOADING || event == OTA_EVENT_INSTALLING) {
	//Edit by Realtek
        DUER_LOGI("######progress %f", progress);
        baidu_json_AddNumberToObject(event_json, "percent", progress);
    }

    ret = duer_data_report(data_json);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Notifier: Report OTA state failed ret: %d", ret);
    }

    /*
     * Json object use linked list to link related json objects
     * and, it will delete all json objects which linked automatic
     * So, We do not need to delete them(data_json, event_json) one by one
     */
    baidu_json_Delete(data_json);

    return ret;

free_data_json:
    baidu_json_Delete(data_json);

out:
    return ret;
}
