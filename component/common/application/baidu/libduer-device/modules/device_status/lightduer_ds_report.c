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
 * Auth: Leliang Zhang(zhangleliang@baidu.com)
 * Desc: device status report
 */

#include "lightduer_ds_report.h"

#include "lightduer_connagent.h"
#include "lightduer_event_emitter.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_timers.h"

#define DEFAULT_INTERVAL (5 * 60 * 1000) // 5min
//#define LIGHTDUER_DS_REPORT_DEBUG

/**
 * TODO:
 *     1. whether a mutex necessary to protect the list
 *     2. all the register function are global resource, if need a clean function
 */
typedef struct function_node {
    duer_ds_report_function_t report_function;
    struct function_node      *next;
} function_node_t;

static function_node_t *s_head = NULL;

duer_status_t duer_ds_register_report_function(duer_ds_report_function_t report_function)
{

    if (report_function == NULL) {
        DUER_LOGI("parameter report_function is NULL");
        return DUER_ERR_INVALID_PARAMETER;
    }
    if (!s_head) {
        s_head = (function_node_t*)DUER_MALLOC(sizeof(function_node_t));

        if (!s_head) {
            DUER_LOGE("memory overflow");
            return DUER_ERR_MEMORY_OVERLOW;
        }

        s_head->report_function = report_function;
        s_head->next = NULL;
        return DUER_OK;
    } else {
        if (s_head->report_function == report_function) {
            DUER_LOGI("function:%p already exist", report_function);
            return DUER_OK;
        }
    }

    function_node_t *p = s_head;

    while (p->next) {
        p = p->next;
        if (p->report_function == report_function) {
            DUER_LOGI("function:%p already exist", report_function);
            return DUER_OK;
        }
    }

    p->next = (function_node_t*)DUER_MALLOC(sizeof(function_node_t));
    if (!p->next) {
        DUER_LOGE("memory overflow");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    p->next->report_function = report_function;
    p->next->next = NULL;

    return DUER_OK;
}

static void duer_ds_report_handler(int what, void *param)
{
    baidu_json *report = NULL;
    baidu_json *device_status = NULL;
    function_node_t *p = NULL;
    int ret = DUER_OK;

    report = baidu_json_CreateObject();
    if (report == NULL) {
        DUER_LOGE("report json create fail");
        goto out;
    }

    device_status = baidu_json_CreateArray();
    if (device_status == NULL) {
        DUER_LOGE("device_status json create fail");
        goto out;
    }

    p = s_head;
    while (p) {
        if (p->report_function) {
            baidu_json_AddItemToArray(device_status, p->report_function());
        }
        p = p->next;
    }

    baidu_json_AddItemToObjectCS(report, "duer_device_status", device_status);
#ifdef LIGHTDUER_DS_REPORT_DEBUG
    const char *str_report = baidu_json_PrintUnformatted(report);
    DUER_LOGI("report: %s", str_report);
    baidu_json_release((void*)str_report);
#endif
    ret = duer_data_report(report);
    if (ret != DUER_OK) {
        DUER_LOGE("report device status fail: %d", ret);
    }

out:
    if (report != NULL) {
        baidu_json_Delete(report);
    }
}

static void duer_ds_report(void *param)
{
    duer_emitter_emit(duer_ds_report_handler, 0, NULL);
}

static duer_timer_handler s_ds_report_timer = NULL;

duer_status_t duer_ds_report_start(duer_ds_report_type_enum_t type, duer_u32_t interval)
{
    int result = DUER_OK;

    if (type == DUER_DS_ONCE) {
        duer_ds_report(NULL);
    } else if (type == DUER_DS_PERIODIC) {
        if (interval == 0) { // use the default value 5min
            interval = DEFAULT_INTERVAL;
        }
        if (s_ds_report_timer == NULL) {
            s_ds_report_timer = duer_timer_acquire(duer_ds_report, NULL, DUER_TIMER_PERIODIC);
            if (s_ds_report_timer == NULL) {
                DUER_LOGW("create timer failed");
                return DUER_ERR_FAILED;
            }
            duer_ds_report(NULL);
            result = duer_timer_start(s_ds_report_timer, interval);
        } else {
            duer_timer_stop(s_ds_report_timer);
            duer_ds_report(NULL);
            result = duer_timer_start(s_ds_report_timer, interval);
        }
    } else {
        DUER_LOGW("unsupported type:%d", type);
    }

    return result;
}

duer_status_t duer_ds_report_stop()
{
    if (s_ds_report_timer) {
        duer_timer_stop(s_ds_report_timer);
        duer_timer_release(s_ds_report_timer);
        s_ds_report_timer = NULL;
    }
    return DUER_OK;
}

duer_status_t duer_ds_report_destroy(void)
{
    duer_ds_report_stop();

    function_node_t *p = s_head;
    function_node_t *next = NULL;
    s_head = NULL;
    while (p) {
        next = p->next;
        DUER_FREE(p);
        p = next;
    }

    return DUER_OK;
}
