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
 * File: lightduer_dcs_alert.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Light duer alert procedure.
 */

#include "lightduer_dcs_alert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lightduer_dcs.h"
#include "lightduer_dcs_local.h"
#include "lightduer_dcs_router.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"
#include "lightduer_ds_log_dcs.h"

static const char *ALERT_NAMESPACE = "ai.dueros.device_interface.alerts";
static const char* const s_event_name_tab[] = {"SetAlertSucceeded",
                                               "SetAlertFailed",
                                               "DeleteAlertSucceeded",
                                               "DeleteAlertFailed",
                                               "AlertStarted",
                                               "AlertStopped"};
static volatile duer_bool s_is_initialized;

int duer_dcs_report_alert_event(const char *token, duer_dcs_alert_event_type type)
{
    baidu_json *data = NULL;
    baidu_json *event = NULL;
    baidu_json *payload = NULL;
    int rs = DUER_OK;

    DUER_DCS_CRITICAL_ENTER();
    if (!s_is_initialized) {
        DUER_LOGW("DCS alert has not been initialized");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    if (!token) {
        DUER_LOGE("Param error: token cannot be null!");
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        rs = DUER_ERR_INVALID_PARAMETER;
        goto RET;
    }

    if ((int)type < 0 || type >= sizeof(s_event_name_tab) / sizeof(s_event_name_tab[0])) {
        DUER_LOGE("Invalid event type!");
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        rs = DUER_ERR_INVALID_PARAMETER;
        goto RET;
    }

    data = baidu_json_CreateObject();
    if (data == NULL) {
        rs = DUER_ERR_MEMORY_OVERLOW;
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        goto RET;
    }

    event = duer_create_dcs_event(ALERT_NAMESPACE,
                                  s_event_name_tab[type],
                                  DUER_TRUE);
    if (event == NULL) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObjectCS(data, DCS_EVENT_KEY, event);

    payload = baidu_json_GetObjectItem(event, DCS_PAYLOAD_KEY);
    if (!payload) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }
    baidu_json_AddStringToObjectCS(payload, DCS_TOKEN_KEY, token);

    duer_dcs_data_report_internal(data, DUER_TRUE);

RET:
    DUER_DCS_CRITICAL_EXIT();

    if (data) {
        baidu_json_Delete(data);
    }

    if ((rs != DUER_OK) && (rs != DUER_ERR_INVALID_PARAMETER)) {
        duer_ds_log_dcs_event_report_fail(s_event_name_tab[type]);
    }

    return rs;
}

static duer_status_t duer_alert_set_cb(const baidu_json *directive)
{
    int ret = DUER_OK;

    ret = duer_dcs_tone_alert_set_handler(directive);
    return ret;
}

static duer_status_t duer_alert_delete_cb(const baidu_json *directive)
{
    int ret = DUER_OK;
    baidu_json *payload = NULL;
    baidu_json *token = NULL;

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        DUER_LOGE("Failed to get payload");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    token = baidu_json_GetObjectItem(payload, DCS_TOKEN_KEY);
    if (!token) {
        DUER_LOGE("Failed to get token");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    duer_dcs_alert_delete_handler(token->valuestring);

RET:
    return ret;
}

int duer_insert_alert_list(baidu_json *alert_list,
                            duer_dcs_alert_info_type *alert_info,
                            duer_bool is_active)
{
    int rs = DUER_OK;
    baidu_json *all_alerts = NULL;
    baidu_json *active_alerts = NULL;
    baidu_json *alert_node = NULL;
    baidu_json *active_alert_node = NULL;
    baidu_json *payload = NULL;

    if (!alert_list || !alert_info) {
        DUER_LOGE("Param error");
        rs = DUER_ERR_INVALID_PARAMETER;
        goto exit;
    }

    payload = baidu_json_GetObjectItem(alert_list, DCS_PAYLOAD_KEY);
    if (!payload) {
        DUER_LOGE("Filed to get payload item");
        rs = DUER_ERR_INVALID_PARAMETER;
        goto exit;
    }

    all_alerts = baidu_json_GetObjectItem(payload, DCS_ALL_ALERTS_KEY);
    active_alerts = baidu_json_GetObjectItem(payload, DCS_ACTIVE_ALERTS_KEY);
    if (!all_alerts || !active_alerts) {
        DUER_LOGE("Filed to get alerts item");
        rs = DUER_ERR_INVALID_PARAMETER;
        goto exit;
    }

    alert_node = baidu_json_CreateObject();
    if (!alert_node) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        DUER_LOGE("Memory overlow");
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto exit;
    }

    baidu_json_AddStringToObject(alert_node, DCS_SCHEDULED_TIME_KEY, alert_info->time);
    baidu_json_AddStringToObject(alert_node, DCS_TOKEN_KEY, alert_info->token);
    baidu_json_AddStringToObject(alert_node, DCS_TYPE_KEY, alert_info->type);
    baidu_json_AddItemToArray(all_alerts, alert_node);

    if (is_active) {
        active_alert_node = baidu_json_CreateObject();
        if (!active_alert_node) {
            DUER_LOGE("Memory overlow");
            rs = DUER_ERR_MEMORY_OVERLOW;
            goto exit;
        }

        baidu_json_AddStringToObject(active_alert_node, DCS_SCHEDULED_TIME_KEY, alert_info->time);
        baidu_json_AddStringToObject(active_alert_node, DCS_TOKEN_KEY, alert_info->token);
        baidu_json_AddStringToObject(active_alert_node, DCS_TYPE_KEY, alert_info->type);
        baidu_json_AddItemToArray(active_alerts, active_alert_node);
    }

exit:
    return rs;
}

static baidu_json* duer_get_alert_state_internal(void)
{
    baidu_json *state = NULL;
    baidu_json *payload = NULL;
    baidu_json *alert_array = NULL;
    baidu_json *active_alert_array = NULL;

    state = duer_create_dcs_event(ALERT_NAMESPACE, DCS_ALERTS_STATE_NAME, DUER_FALSE);
    if (!state) {
        DUER_LOGE("Filed to create alert state");
        goto error_out;
    }

    payload = baidu_json_GetObjectItem(state, DCS_PAYLOAD_KEY);
    if (!payload) {
        DUER_LOGE("Filed to get payload item");
        goto error_out;
    }

    alert_array = baidu_json_CreateArray();
    if (!alert_array) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        DUER_LOGE("Filed to create alert_array");
        goto error_out;
    }
    baidu_json_AddItemToObjectCS(payload, DCS_ALL_ALERTS_KEY, alert_array);

    active_alert_array = baidu_json_CreateArray();
    if (!active_alert_array) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        DUER_LOGE("Filed to create baidu_json *");
        goto error_out;
    }
    baidu_json_AddItemToObjectCS(payload, DCS_ACTIVE_ALERTS_KEY, active_alert_array);

    duer_dcs_get_all_alert(state);

    return state;

error_out:
    if (state) {
        baidu_json_Delete(state);
    }

    duer_ds_log_dcs_event_report_fail(DCS_ALERTS_STATE_NAME);

    return NULL;
}

void duer_dcs_alert_init()
{
    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        DUER_DCS_CRITICAL_EXIT();
        return;
    }

    duer_directive_list res[] = {
        {DCS_SET_ALERT_NAME, duer_alert_set_cb},
        {DCS_DELETE_ALERT_NAME, duer_alert_delete_cb},
    };

    duer_add_dcs_directive_internal(res, sizeof(res) / sizeof(res[0]), ALERT_NAMESPACE);
    duer_reg_client_context_cb_internal(duer_get_alert_state_internal);
    s_is_initialized = DUER_TRUE;
    DUER_DCS_CRITICAL_EXIT();
}

void duer_dcs_alert_uninitialize_internal(void)
{
    s_is_initialized = DUER_FALSE;
}

