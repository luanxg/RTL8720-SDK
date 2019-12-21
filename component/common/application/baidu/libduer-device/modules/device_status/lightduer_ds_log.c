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
 * Desc: device log report
 */

#include "lightduer_ds_log.h"

#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_cache.h"
#include "lightduer_timestamp.h"
#include "lightduer_memory.h"

//#define LIGHTDUER_DS_LOG_DEBUG // only for debug

static duer_ds_log_level_enum_t s_report_level = DUER_DS_LOG_DEFAULT_REPORT_LEVEL;

duer_status_t duer_ds_log_set_report_level(duer_ds_log_level_enum_t log_level)
{
    if (log_level < DUER_DS_LOG_LEVEL_FATAL) {
        s_report_level = DUER_DS_LOG_LEVEL_FATAL;
    } else if (log_level > DUER_DS_LOG_LEVEL_DEBUG) {
        s_report_level = DUER_DS_LOG_LEVEL_DEBUG;
    } else {
        s_report_level = log_level;
    }
    return DUER_OK;
}

/**
 * payload format in json:
 * {
 *     "level": 5
 * }
 * 1 Fatal, 2 Error 3 Warn 4 Info 5 Debug
 */
static duer_status_t duer_ds_log_set_level(duer_context ctx, duer_msg_t* msg, duer_addr_t* addr)
{
    duer_handler handler = (duer_handler)ctx;

    if (handler && msg) {
        int msg_code = DUER_MSG_RSP_BAD_REQUEST;
        if (msg->payload && msg->payload_len > 0) {

            duer_ds_log_level_enum_t report_level = DUER_DS_LOG_DEFAULT_REPORT_LEVEL;

            baidu_json *value = baidu_json_Parse(msg->payload);
            if (value) {
                baidu_json *level = baidu_json_GetObjectItem(value, "level");
                if (level) {
                    switch(level->valueint) {
                    case 1:
                        report_level = DUER_DS_LOG_LEVEL_FATAL;
                        break;
                    case 2:
                        report_level = DUER_DS_LOG_LEVEL_ERROR;
                        break;
                    case 3:
                        report_level = DUER_DS_LOG_LEVEL_WARN;
                        break;
                    case 4:
                        report_level = DUER_DS_LOG_LEVEL_INFO;
                        break;
                    case 5:
                        report_level = DUER_DS_LOG_LEVEL_DEBUG;
                        break;
                    default:
                        break;
                    }
                    DUER_LOGI("duer_ds_log_set_level, %d", report_level);
                    duer_ds_log_set_report_level(report_level);
                    msg_code = DUER_MSG_RSP_CHANGED;
                }
                baidu_json_Delete(value);
            }
        }
        duer_response(msg, msg_code, NULL, 0);
    }

    return DUER_OK;
}

void duer_ds_log_init()
{
    duer_res_t res[] = {
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT, "duer_ds_log_set_level", {duer_ds_log_set_level}},
    };

    duer_add_resources(res, sizeof(res) / sizeof(res[0]));
}

duer_u32_t duer_ds_log_generate_code(duer_ds_log_version_enum_t log_version,
                                     duer_ds_log_level_enum_t   log_level,
                                     duer_ds_log_module_enum_t  log_module,
                                     duer_ds_log_family_enum_t  log_family,
                                     int log_code)
{
    return ((log_version & BITS_VERSION) << OFFSET_VERSION)
           | ((log_level & BITS_LEVEL) << OFFSET_LEVEL)
           | ((log_module & BITS_MODULE) << OFFSET_MODULE)
           | ((log_family & BITS_FAMILY) << OFFSET_FAMILY)
           | ((log_code & BITS_CODE) << OFFSET_CODE);
}

duer_u32_t duer_ds_log_get_log_version(duer_u32_t log_code)
{
    return (log_code & MASK_VERSION) >> OFFSET_VERSION;
}

duer_u32_t duer_ds_log_get_log_level(duer_u32_t log_code)
{
    return (log_code & MASK_LEVEL) >> OFFSET_LEVEL;
}

duer_u32_t duer_ds_log_get_log_module(duer_u32_t log_code)
{
    return (log_code & MASK_MODULE) >> OFFSET_MODULE;
}

duer_u32_t duer_ds_log_get_log_family(duer_u32_t log_code)
{
    return (log_code & MASK_FAMILY) >> OFFSET_FAMILY;
}

duer_u32_t duer_ds_log_get_log_code(duer_u32_t log_code)
{
    return (log_code & MASK_CODE) >> OFFSET_CODE;
}

duer_status_t duer_ds_log_v_f(duer_ds_log_version_enum_t log_version,
                              duer_ds_log_level_enum_t   log_level,
                              duer_ds_log_module_enum_t  log_module,
                              duer_ds_log_family_enum_t  log_family,
                              int log_code,
                              const baidu_json *log_message)
{
    duer_status_t result = DUER_OK;
    baidu_json *log = NULL;
    baidu_json *content = NULL;
    const char *str_log = NULL;
    duer_u32_t timestamp = 0;

    duer_u32_t code = 0;

    if (log_level > s_report_level) {
        DUER_LOGD("under report level: version:0x%x, level:0x%x, module:0x%x, family:0x%x, code:0x%x",
                   log_version, log_level, log_module, log_family, log_code);
        goto out;
    }

    code = duer_ds_log_generate_code(
              log_version, log_level, log_module, log_family, log_code);
    log = baidu_json_CreateObject();
    if (log == NULL) {
        DUER_LOGE("log json create fail");
        result = DUER_ERR_MEMORY_OVERLOW;
        goto out;
    }

    content = baidu_json_CreateObject();
    if (content == NULL) {
        DUER_LOGE("content json create fail");
        result = DUER_ERR_MEMORY_OVERLOW;
        goto out;
    }

    if (log_message) {
        baidu_json_AddItemToObjectCS(content, "message", (baidu_json*)log_message);
    }

#ifdef LIGHTDUER_DS_LOG_DEBUG
    DUER_LOGI("logcode:0x%x", code);
    DUER_LOGI("log_version:0x%x", duer_ds_log_get_log_version(code));
    DUER_LOGI("log_level:0x%x", duer_ds_log_get_log_level(code));
    DUER_LOGI("log_module:0x%x", duer_ds_log_get_log_module(code));
    DUER_LOGI("log_family:0x%x", duer_ds_log_get_log_family(code));
    DUER_LOGI("log_code:0x%x", duer_ds_log_get_log_code(code));
#endif

    baidu_json_AddNumberToObjectCS(content, "code", code);
    timestamp = duer_timestamp();
    baidu_json_AddNumberToObjectCS(content, "ts", timestamp);

    baidu_json_AddItemToObjectCS(log, "duer_trace_info", content);
#ifdef LIGHTDUER_DS_LOG_DEBUG
    str_log = baidu_json_PrintUnformatted(log);
    DUER_LOGI("log: %s", str_log);
    baidu_json_release((void*)str_log);
#endif
    result = duer_data_report(log);
    if (result != DUER_OK) {
        if (result == DUER_ERR_CA_NOT_CONNECTED) {
            DUER_LOGW("CA not connected: %d", result);
            // try to store the code
            // which value is DUER_DS_LOG_LEVEL_FATAL<= log_level <=DUER_DS_LOG_DEFAULT_CACHE_LEVEL
            if (DUER_DS_LOG_LEVEL_FATAL <= log_level
                    && log_level<= DUER_DS_LOG_DEFAULT_CACHE_LEVEL) {
                duer_ds_log_cache_push(code, log_message, timestamp);
            }
        } else {
            DUER_LOGE("report log fail: %d", result);
        }
        str_log = baidu_json_PrintUnformatted(log);
        DUER_LOGW("trace log report fail: %s", str_log);
        baidu_json_release((void*)str_log);
    }

out:
    if (log != NULL) {
        baidu_json_Delete(log);
        if (content == NULL && log_message != NULL) {
            baidu_json_Delete((baidu_json*)log_message);
        }
    } else if (log_message != NULL) {
        baidu_json_Delete((baidu_json*)log_message);
    } else {
        // do nothing
    }
    return result;
}

duer_status_t duer_ds_log_f(duer_ds_log_level_enum_t   log_level,
                            duer_ds_log_module_enum_t  log_module,
                            duer_ds_log_family_enum_t  log_family,
                            int log_code,
                            const baidu_json *log_message)
{
    return duer_ds_log_v_f(DUER_DS_LOG_VERSION_1_0,
                           log_level,
                           log_module,
                           log_family,
                           log_code,
                           log_message);
}

duer_status_t duer_ds_log(duer_ds_log_level_enum_t   log_level,
                          duer_ds_log_module_enum_t  log_module,
                          int log_code,
                          const baidu_json *log_message)
{
    return duer_ds_log_f(log_level, log_module, DUER_DS_LOG_FAMILY_UNKNOWN, log_code, log_message);
}
