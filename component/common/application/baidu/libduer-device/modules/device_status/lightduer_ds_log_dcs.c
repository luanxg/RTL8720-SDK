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
 * Auth: Gang Chen(chengang12@baidu.com)
 * Desc: dcs module log report
 */

#include "lightduer_ds_log_dcs.h"
#include "lightduer_ds_log.h"
#include "lightduer_log.h"

/*
 * In DCS module, log code 0x1xx means debug,
 * 0x2xx means info, 0x3xx means wanning,
 * 0x4xx means error, 0x5xx means fatal.
 */
#define DCS_LOG_LEVEL_SHIFT  8
#define DCS_LOG_LEVEL_DEBUG  1
#define DCS_LOG_LEVEL_INFO   2
#define DCS_LOG_LEVEL_WARN   3
#define DCS_LOG_LEVEL_ERROR  4
#define DCS_LOG_LEVEL_FATAL  5

duer_status_t duer_ds_log_dcs(duer_ds_log_dcs_code_t log_code, const baidu_json *log_message)
{
    duer_status_t ret = DUER_OK;
    duer_ds_log_level_enum_t level = DUER_DS_LOG_LEVEL_DEBUG;

    switch (log_code >> DCS_LOG_LEVEL_SHIFT) {
    case DCS_LOG_LEVEL_DEBUG:
        level = DUER_DS_LOG_LEVEL_DEBUG;
        break;
    case DCS_LOG_LEVEL_INFO:
        level = DUER_DS_LOG_LEVEL_INFO;
        break;
    case DCS_LOG_LEVEL_WARN:
        level = DUER_DS_LOG_LEVEL_WARN;
        break;
    case DCS_LOG_LEVEL_ERROR:
        level = DUER_DS_LOG_LEVEL_ERROR;
        break;
    case DCS_LOG_LEVEL_FATAL:
        level = DUER_DS_LOG_LEVEL_FATAL;
        break;
    default:
        ret = DUER_ERR_INVALID_PARAMETER;
        DUER_LOGE("Unknown log code: %d", log_code);
        goto exit;
    }

    ret = duer_ds_log(level, DUER_DS_LOG_MODULE_DCS, log_code, log_message);
exit:
    return ret;
}

duer_status_t duer_ds_log_dcs_report_with_dir(duer_ds_log_dcs_code_t log_code,
                                                   const char *file,
                                                   int line)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;;
    }

    baidu_json_AddStringToObject(msg, "file", file);
    baidu_json_AddNumberToObject(msg, "line", line);

    ret = duer_ds_log_dcs(log_code, msg);

    return ret;
}

duer_status_t duer_ds_log_dcs_directive_drop(const char *current_id,
                                             const char *directive_id,
                                             const char *name)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObject(msg, "current_dialog_id", current_id);
    baidu_json_AddStringToObject(msg, "directive_dialog_id", directive_id);
    if (name) {
        baidu_json_AddStringToObject(msg, "directive", name);
    } else {
        baidu_json_AddStringToObject(msg, "directive", "");
    }
    ret = duer_ds_log_dcs(DUER_DS_LOG_DCS_OLD_DIRECTIVE_DROPPED, msg);

    return ret;
}

duer_status_t duer_ds_log_dcs_handler_unrealize(const char *func)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObject(msg, "handler_name", func);

    ret = duer_ds_log_dcs(DUER_DS_LOG_DCS_HANDLER_UNREALIZED, msg);

    return ret;
}

duer_status_t duer_ds_log_dcs_add_directive_fail(const char *name)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObject(msg, "directive", name);
    ret = duer_ds_log_dcs(DUER_DS_LOG_DCS_ADD_DIRECTIVE_FAILED, msg);

    return ret;
}

duer_status_t duer_ds_log_dcs_event_report_fail(const char *name)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObject(msg, "event_name", name);
    ret = duer_ds_log_dcs(DUER_DS_LOG_DCS_REPORT_EVENT_FAILED, msg);

    return ret;
}

