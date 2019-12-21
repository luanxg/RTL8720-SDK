/**
 * Copyright (2018) Baidu Inc. All rights reserved.
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
 * Auth: Chen Xihao(chenxihao@baidu.com)
 * Desc: bind device module related report log
 */

#include "lightduer_ds_log_bind.h"
#include "lightduer_ds_log.h"
#include "lightduer_ds_report.h"
#include "lightduer_log.h"

typedef struct duer_ds_report_bind_result_s {
    duer_u32_t req_port;
    duer_u32_t resp_success_count;
    duer_u32_t resp_fail_count;
} duer_ds_report_bind_result_t;

static duer_ds_report_bind_result_t s_bind_result;

duer_status_t duer_ds_log_bind(duer_ds_log_bind_code_t log_code)
{
    duer_ds_log_level_enum_t log_level = DUER_DS_LOG_LEVEL_INFO;
    if (log_code >= DUER_DS_LOG_BIND_CONFIG_WIFI_AIRKISS
            && log_code <= DUER_DS_LOG_BIND_TASK_END) {
        log_level = DUER_DS_LOG_LEVEL_INFO;
    } else if (log_code >= DUER_DS_LOG_BIND_NO_MEMORY
            && log_code <= DUER_DS_LOG_BIND_SOCKET_ERROR) {
        log_level = DUER_DS_LOG_LEVEL_WARN;
    } else {
        DUER_LOGW("unkown log code 0x%0x", log_code);
        return DUER_ERR_FAILED;
    }

    return duer_ds_log(log_level, DUER_DS_LOG_MODULE_BIND, log_code, NULL);
}

/**
 * generate the ca result report
 * this function used in @duer_ds_register_report_function
 * "bind_result": {
 *      "req_port": 12476,
 *      "resp_success_count": 1,
 *      "resp_fail_count": 0
 * }
 */
static baidu_json* duer_ds_report_bind_result(void)
{
    baidu_json *msg = baidu_json_CreateObject();
    baidu_json *bind_result = NULL;

    if (msg == NULL) {
        DUER_LOGE("msg json create fail");
        return NULL;
    }

    do {
        bind_result = baidu_json_CreateObject();
        if (bind_result == NULL) {
            DUER_LOGE("bind_result json create fail");
            break;
        }
        baidu_json_AddNumberToObjectCS(bind_result, "req_port", s_bind_result.req_port);
        baidu_json_AddNumberToObjectCS(bind_result,
                "resp_success_count", s_bind_result.resp_success_count);
        baidu_json_AddNumberToObjectCS(bind_result,
                "resp_fail_count", s_bind_result.resp_fail_count);

        baidu_json_AddItemToObject(msg, "bind_result", bind_result);
    } while (0);

    if (bind_result == NULL) {
        baidu_json_Delete(msg);
        msg = NULL;
    }

    return msg;
}

int duer_ds_log_report_bind_result(duer_u32_t req_port,
            duer_u32_t resp_success_count, duer_u32_t resp_fail_count)
{
    int ret = 0;
    s_bind_result.req_port = req_port;
    s_bind_result.resp_success_count = resp_success_count;
    s_bind_result.resp_fail_count = resp_fail_count;

    ret = duer_ds_register_report_function(duer_ds_report_bind_result);
    if (ret != DUER_OK) {
        DUER_LOGE("report bind result register failed ret: %d", ret);
    }

    return ret;
}
