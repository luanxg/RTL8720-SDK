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
 * Desc: trace log cache, cache the trace log when ca disconnect
 */
#include "lightduer_ds_log_cache.h"

#include "lightduer_connagent.h"
#include "lightduer_lib.h"
#include "lightduer_log.h"
#include "lightduer_mutex.h"

#define MAX_CACHED_TRACELOG 16
// #define LIGHTDUER_DS_LOG_CACHE_DEBUG

duer_u32_t s_trace_log_cache[MAX_CACHED_TRACELOG];
volatile duer_u32_t s_current_index = 0;

duer_mutex_t s_mutex = NULL;

duer_status_t duer_ds_log_cache_initialize()
{
    if (!s_mutex) {
        s_mutex = duer_mutex_create();
        if (!s_mutex) {
            DUER_LOGE("mutex create failed");
            return DUER_ERR_FAILED;
        }
    }
    return DUER_OK;
}

duer_status_t duer_ds_log_cache_finalize()
{
    //Note, this mutex not release, because after stop, there still is message need to report
    //if (s_mutex != NULL) {
    //    duer_mutex_destroy(s_mutex);
    //    s_mutex = NULL;
    //}
    return DUER_OK;
}

duer_status_t duer_ds_log_cache_push(duer_u32_t code, const baidu_json *message, duer_u32_t timestamp)
{
    duer_status_t result = DUER_OK;
    if (!s_mutex) {
        return DUER_ERR_FAILED;
    }
    DUER_LOGI("cached tracecode: 0x%x", code);
    duer_mutex_lock(s_mutex);
    if (s_current_index < MAX_CACHED_TRACELOG) {
        s_trace_log_cache[s_current_index++] = code;
    } else {
        DUER_LOGI("cache is full!");
        result = DUER_ERR_FAILED;
    }
    duer_mutex_unlock(s_mutex);
    return result;
}

duer_status_t duer_ds_log_cache_report()
{
    duer_status_t result = DUER_OK;

    baidu_json *report = NULL;
    baidu_json *cache_log = NULL;
    const char *str_report = NULL;
    char c_index[3];

    if (!s_current_index) {
        DUER_LOGI("no cache report");
        goto out;
    }

    if (!s_mutex) {
        DUER_LOGI("no mutex create");
        result = DUER_ERR_FAILED;
        goto out;
    }

    report = baidu_json_CreateObject();
    if (report == NULL) {
        DUER_LOGE("report json create fail");
        result = DUER_ERR_MEMORY_OVERLOW;
        goto out;
    }

    cache_log = baidu_json_CreateObject();
    if (cache_log == NULL) {
        DUER_LOGE("cache_log json create fail");
        result = DUER_ERR_MEMORY_OVERLOW;
        goto out;
    }

    duer_mutex_lock(s_mutex);
    while (s_current_index > 0) {
        DUER_SNPRINTF(c_index, sizeof(c_index), "%02d", s_current_index);
        duer_u32_t code = s_trace_log_cache[--s_current_index];
        baidu_json_AddNumberToObject(cache_log, c_index, code);
    }
    duer_mutex_unlock(s_mutex);

    baidu_json_AddItemToObjectCS(report, "duer_trace_cache", cache_log);
#ifdef LIGHTDUER_DS_LOG_CACHE_DEBUG
    str_report = baidu_json_PrintUnformatted(report);
    DUER_LOGI("report: %s", str_report);
    baidu_json_release((void*)str_report);
#endif

    result = duer_data_report(report);
    if (result != DUER_OK) {
        str_report = baidu_json_PrintUnformatted(report);
        DUER_LOGW("cached trace log report fail: %s", str_report);
        baidu_json_release((void*)str_report);
    }
out:
    if (report != NULL) {
        baidu_json_Delete(report);
    }
    return result;
}
