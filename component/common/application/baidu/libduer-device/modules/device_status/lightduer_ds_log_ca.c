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
 * Desc: ca related trace log
 */

#include "lightduer_ds_log_ca.h"

#include "lightduer_log.h"
#include "lightduer_ds_log_cache.h"
#include "lightduer_ds_report_ca.h"
#include "lightduer_timestamp.h"

#define MODULE_NAME "ca"

duer_status_t duer_ds_log_ca(duer_ds_log_level_enum_t log_level,
                             int log_code,
                             const baidu_json *log_message)
{
    return duer_ds_log(log_level, DUER_DS_LOG_MODULE_CA, log_code, log_message);
}

duer_status_t duer_ds_log_ca_fatal(int log_code,
                                   const baidu_json *log_message)
{
    return duer_ds_log_ca(DUER_DS_LOG_LEVEL_FATAL, log_code, log_message);
}

duer_status_t duer_ds_log_ca_error(int log_code,
                                   const baidu_json *log_message)
{
    return duer_ds_log_ca(DUER_DS_LOG_LEVEL_ERROR, log_code, log_message);
}

duer_status_t duer_ds_log_ca_warn(int log_code,
                                  const baidu_json *log_message)
{
    return duer_ds_log_ca(DUER_DS_LOG_LEVEL_WARN, log_code, log_message);
}

duer_status_t duer_ds_log_ca_info(int log_code,
                                  const baidu_json *log_message)
{
    return duer_ds_log_ca(DUER_DS_LOG_LEVEL_INFO, log_code, log_message);
}

duer_status_t duer_ds_log_ca_debug(int log_code,
                                   const baidu_json *log_message)
{
    return duer_ds_log_ca(DUER_DS_LOG_LEVEL_DEBUG, log_code, log_message);
}

duer_status_t duer_ds_log_ca_cache_error(int log_code)
{
    duer_u32_t code = duer_ds_log_generate_code(
              DUER_DS_LOG_VERSION_1_0, DUER_DS_LOG_LEVEL_ERROR,
              DUER_DS_LOG_MODULE_CA, DUER_DS_LOG_FAMILY_UNKNOWN, log_code);
    return duer_ds_log_cache_push(code, NULL, duer_timestamp());
}

duer_status_t duer_ds_log_ca_start_state_change(int from, int to)
{
    duer_status_t result = DUER_OK;
    baidu_json *log_ca = NULL;

    log_ca = baidu_json_CreateObject();
    if (log_ca == NULL) {
        DUER_LOGE("log_ca json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObject(log_ca, "from", from);
    baidu_json_AddNumberToObject(log_ca, "to", to);

    result = duer_ds_log_ca_debug(DUER_DS_LOG_CA_START_STATE_CHANGE, log_ca);

    return result;
}

duer_status_t duer_ds_log_ca_start_connect(const char *host, int port)
{
    duer_status_t result = DUER_OK;
    baidu_json *log_ca = NULL;

    log_ca = baidu_json_CreateObject();
    if (log_ca == NULL) {
        DUER_LOGE("log_ca json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObject(log_ca, "host", host);
    baidu_json_AddNumberToObject(log_ca, "port", port);

    result = duer_ds_log_ca_debug(DUER_DS_LOG_CA_START_CONNECT, log_ca);

    return result;
}

duer_status_t duer_ds_log_ca_started(duer_status_t reason, int count, duer_u32_t connect_time)
{
    duer_status_t result = DUER_OK;
    baidu_json *log_ca = NULL;

    log_ca = baidu_json_CreateObject();
    if (log_ca == NULL) {
        DUER_LOGE("log_ca json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    switch(reason) {
    case DUER_OK:
        baidu_json_AddStringToObject(log_ca, "reason", "STARTUP");
        break;
    case DUER_ERR_FAILED:
        baidu_json_AddStringToObject(log_ca, "reason", "INTERNAL ERROR");
        break;
    case DUER_ERR_TRANS_TIMEOUT:
        baidu_json_AddStringToObject(log_ca, "reason", "SEND TIMEOUT");
        break;
    case DUER_ERR_TRANS_DNS_FAIL:
        baidu_json_AddStringToObject(log_ca, "reason", "DNS FAIL");
        break;
    default:
        baidu_json_AddNumberToObject(log_ca, "reason", reason);
        break;
    }

    baidu_json_AddNumberToObject(log_ca, "count", count);
    baidu_json_AddNumberToObject(log_ca, "connect_time", connect_time);

    result = duer_ds_log_ca_info(DUER_DS_LOG_CA_STARTED, log_ca);

    return result;
}

duer_status_t duer_ds_log_ca_register_cp(const char *name,
                                         duer_ds_log_ca_controlpoint_type_enum_t type)
{
    duer_status_t result = DUER_OK;
    baidu_json *log_ca = NULL;

    log_ca = baidu_json_CreateObject();
    if (log_ca == NULL) {
        DUER_LOGE("log_ca json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObject(log_ca, "name", name);
    switch(type) {
    case DUER_CA_CP_TYPE_STATIC:
        baidu_json_AddStringToObject(log_ca, "type", "STATIC");
        break;
    case DUER_CA_CP_TYPE_DYNAMIC:
        baidu_json_AddStringToObject(log_ca, "type", "DYNAMIC");
        break;
    default:
        baidu_json_AddStringToObject(log_ca, "type", "UNKNOWN");
        break;
    }

    result = duer_ds_log_ca_debug(DUER_DS_LOG_CA_REGISTER_CONTROL_POINT, log_ca);

    return result;
}

duer_status_t duer_ds_log_ca_call_cp(const char *name)
{
    duer_status_t result = DUER_OK;
    baidu_json *log_ca = NULL;

    log_ca = baidu_json_CreateObject();
    if (log_ca == NULL) {
        DUER_LOGE("log_ca json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObject(log_ca, "name", name);

    result = duer_ds_log_ca_debug(DUER_DS_LOG_CA_CALL_CONTROL_POINT, log_ca);

    return result;
}

static duer_bool duer_ds_log_ca_is_mbedtls_error(duer_s32_t code)
{
    if ((code > DUER_ERR_MBEDTLS_NET_MIN && code < DUER_ERR_MBEDTLS_NET_MAX)
            || (code > DUER_ERR_MBEDTLS_SSL_MIN && code < DUER_ERR_MBEDTLS_SSL_MAX)) {
        return DUER_TRUE;
    }
    return DUER_FALSE;
}

duer_status_t duer_ds_log_ca_mbedtls_error(duer_s32_t code)
{
    duer_status_t result = DUER_OK;
    // baidu_json *log_ca = NULL;

    if (duer_ds_log_ca_is_mbedtls_error(code) == DUER_FALSE) {
        DUER_LOGI("not mbedtls error, %x", code);
        return DUER_OK;
    }

    duer_ds_report_ca_inc_error_mbedtls();

    // log_ca = baidu_json_CreateObject();
    // if (log_ca == NULL) {
    //     DUER_LOGE("log_ca json create fail");
    //     return DUER_ERR_MEMORY_OVERLOW;
    // }
    // DUER_LOGE("mbedtls error : %d", code);
    // baidu_json_AddNumberToObject(log_ca, "code", code);
    // result = duer_ds_log_ca_error(DUER_DS_LOG_CA_MBEDTLS_ERR, log_ca);

    result = duer_ds_log_ca_cache_error(DUER_DS_LOG_CA_MBEDTLS_ERR);

    return result;
}

duer_status_t duer_ds_log_ca_malloc_error(const char *file, duer_u32_t line)
{
    duer_status_t result = DUER_OK;
    // baidu_json *log_ca = NULL;

    // log_ca = baidu_json_CreateObject();
    // if (log_ca == NULL) {
    //     DUER_LOGE("log_ca json create fail");
    //     return DUER_ERR_MEMORY_OVERLOW;
    // }

    // baidu_json_AddStringToObject(log_ca, "file", file);
    // baidu_json_AddNumberToObject(log_ca, "line", line);

    // result = duer_ds_log_ca_error(DUER_DS_LOG_CA_MALLOC_FAIL, log_ca);
    result = duer_ds_log_ca_cache_error(DUER_DS_LOG_CA_MALLOC_FAIL);

    return result;
}
