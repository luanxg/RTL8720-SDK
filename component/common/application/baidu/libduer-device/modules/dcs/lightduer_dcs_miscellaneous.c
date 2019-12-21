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
/**
 * File: lightduer_dcs_miscellaneous.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS APIS such as recommend request.
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_dcs.h"

static const char *s_dcs_recommend_time[DCS_RECOMMEND_TIME_NUMBER] = {"POWER_ON"};

int duer_dcs_recommend_request(duer_dcs_recommend_time_t time)
{
    baidu_json *data = NULL;
    baidu_json *event = NULL;
    baidu_json *client_context = NULL;
    baidu_json *header = NULL;
    baidu_json *payload = NULL;
    int rs = DUER_OK;
    const char *dialog_id = NULL;

    if ((int)time < 0 || time >= DCS_RECOMMEND_TIME_NUMBER) {
        DUER_LOGE("Invalid command type!");
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        rs = DUER_ERR_INVALID_PARAMETER;
        goto RET;
    }

    data = baidu_json_CreateObject();
    if (!data) {
        DUER_LOGE("Memory not enough");
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    client_context = duer_get_client_context_internal();
    if (client_context) {
        baidu_json_AddItemToObjectCS(data, DCS_CLIENT_CONTEXT_KEY, client_context);
    }

    event = duer_create_dcs_event(DCS_RECOMMEND_NAMESPACE,
                                  DCS_RECOMMEND_NAME,
                                  DUER_TRUE);
    if (event == NULL) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObjectCS(data, DCS_EVENT_KEY, event);
    header = baidu_json_GetObjectItem(event, DCS_HEADER_KEY);
    payload = baidu_json_GetObjectItem(event, DCS_PAYLOAD_KEY);
    if (!header || !payload) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddStringToObjectCS(payload, DCS_TIME_OF_RECOMMEND, s_dcs_recommend_time[time]);
    DUER_DCS_CRITICAL_ENTER();
    dialog_id = duer_get_request_id_internal();
    if (!dialog_id) {
        DUER_DCS_CRITICAL_EXIT();
        DUER_LOGE("Failed to create dialog id");
        rs = DUER_ERR_FAILED;
        goto RET;
    }
    baidu_json_AddStringToObjectCS(header, DCS_DIALOG_REQUEST_ID_KEY, dialog_id);
    DUER_DCS_CRITICAL_EXIT();
    duer_dcs_data_report_internal(data, DUER_TRUE);

RET:
    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail(DCS_RECOMMEND_NAME);
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

