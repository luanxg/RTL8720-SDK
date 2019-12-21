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
 * Desc: http module log report
 */

#include "lightduer_ds_log_http.h"
#include "lightduer_ds_log.h"
#include "lightduer_log.h"

/*
 * In HTTP module, log code 0x1xx means debug,
 * 0x2xx means info, 0x3xx means wanning,
 * 0x4xx means error, 0x5xx means fatal.
 */
#define HTTP_LOG_LEVEL_SHIFT  8
#define HTTP_LOG_LEVEL_DEBUG  1
#define HTTP_LOG_LEVEL_INFO   2
#define HTTP_LOG_LEVEL_WARN   3
#define HTTP_LOG_LEVEL_ERROR  4
#define HTTP_LOG_LEVEL_FATAL  5

duer_status_t duer_ds_log_http(duer_ds_log_http_code_t log_code, const baidu_json *log_message)
{
    duer_status_t ret = DUER_OK;
    duer_ds_log_level_enum_t level = DUER_DS_LOG_LEVEL_DEBUG;

    switch (log_code >> HTTP_LOG_LEVEL_SHIFT) {
    case HTTP_LOG_LEVEL_DEBUG:
        level = DUER_DS_LOG_LEVEL_DEBUG;
        break;
    case HTTP_LOG_LEVEL_INFO:
        level = DUER_DS_LOG_LEVEL_INFO;
        break;
    case HTTP_LOG_LEVEL_WARN:
        level = DUER_DS_LOG_LEVEL_WARN;
        break;
    case HTTP_LOG_LEVEL_ERROR:
        level = DUER_DS_LOG_LEVEL_ERROR;
        break;
    case HTTP_LOG_LEVEL_FATAL:
        level = DUER_DS_LOG_LEVEL_FATAL;
        break;
    default:
        ret = DUER_ERR_INVALID_PARAMETER;
        DUER_LOGE("Unknown log code: %d", log_code);
        goto exit;
    }

    ret = duer_ds_log(level, DUER_DS_LOG_MODULE_HTTP, log_code, log_message);
exit:
    return ret;
}

duer_status_t duer_ds_log_http_report_with_dir(duer_ds_log_http_code_t log_code,
                                               const char *file,
                                               int line)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObject(msg, "file", file);
    baidu_json_AddNumberToObject(msg, "line", line);

    ret = duer_ds_log_http(log_code, msg);

    return ret;
}

duer_status_t duer_ds_log_http_redirect_fail(const char *location)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    if (location) {
        baidu_json_AddStringToObject(msg, "location", location);
    } else {
        baidu_json_AddStringToObject(msg, "location", "");
    }

    ret = duer_ds_log_http(DUER_DS_LOG_HTTP_REDIRECT_FAILED, msg);

    return ret;
}

duer_status_t duer_ds_log_http_report_err_code(duer_ds_log_http_code_t log_code, int err_code)
{
        duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObject(msg, "error_code", err_code);

    ret = duer_ds_log_http(log_code, msg);

    return ret;
}

duer_status_t duer_ds_log_http_report_with_url(duer_ds_log_http_code_t log_code, const char *url)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    if (url) {
        baidu_json_AddStringToObject(msg, "url", url);
    } else {
        baidu_json_AddStringToObject(msg, "url", "");
    }

    ret = duer_ds_log_http(log_code, msg);

    return ret;
}

duer_status_t duer_ds_log_http_download_exit(duer_ds_log_http_code_t log_code,
                                             duer_ds_log_http_statistic_t *statistic)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    if (statistic->url) {
        baidu_json_AddStringToObject(msg, "url", statistic->url);
    } else {
        baidu_json_AddStringToObject(msg, "url", "");
    }

    baidu_json_AddNumberToObject(msg, "ret_code", statistic->ret_code);
    baidu_json_AddNumberToObject(msg, "upload_size", statistic->upload_size);
    baidu_json_AddNumberToObject(msg, "download_size", statistic->download_size);
    baidu_json_AddNumberToObject(msg, "download_speed", statistic->download_speed);
    baidu_json_AddNumberToObject(msg, "resume_count", statistic->resume_count);
    baidu_json_AddBoolToObject(msg, "is_chunked", statistic->is_chunked);
    baidu_json_AddNumberToObject(msg, "content_len", statistic->content_len);

    ret = duer_ds_log_http(log_code, msg);

    return ret;
}

duer_status_t duer_ds_log_http_persisten_conn_timeout(const char *host)
{
    duer_status_t ret = DUER_OK;
    baidu_json *msg = NULL;

    msg = baidu_json_CreateObject();
    if (!msg) {
        DUER_LOGE("Failed to create log message json");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    if (host) {
        baidu_json_AddStringToObject(msg, "host", host);
    } else {
        baidu_json_AddStringToObject(msg, "host", "");
    }

    ret = duer_ds_log_http(DUER_DS_LOG_HTTP_PERSISTENT_CONN_TIMEOUT, msg);

    return ret;
}

