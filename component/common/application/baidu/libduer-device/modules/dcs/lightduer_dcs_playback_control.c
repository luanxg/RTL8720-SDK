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
 * File: lightduer_dcs_playback_control.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS playback control interface
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_dcs.h"

static const char* const s_play_cmd_name_tab[] = {"PlayCommandIssued",
                                                  "PauseCommandIssued",
                                                  "PreviousCommandIssued",
                                                  "NextCommandIssued"};
static volatile duer_bool s_is_initialized;

int duer_dcs_send_play_control_cmd(duer_dcs_play_control_cmd_t cmd)
{
    baidu_json *data = NULL;
    baidu_json *event = NULL;
    baidu_json *client_context = NULL;
    int rs = DUER_OK;

    if (!s_is_initialized) {
        DUER_LOGW("Voice input module has not been initialized");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    if ((int)cmd < 0 || cmd >= sizeof(s_play_cmd_name_tab) / sizeof(s_play_cmd_name_tab[0])) {
        DUER_LOGE("Invalid command type!");
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        rs = DUER_ERR_INVALID_PARAMETER;
        goto RET;
    }

    duer_user_activity_internal();

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

    event = duer_create_dcs_event(DCS_PLAYBACK_CONTROL_NAMESPACE,
                                  s_play_cmd_name_tab[cmd],
                                  DUER_TRUE);
    if (event == NULL) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObjectCS(data, DCS_EVENT_KEY, event);
    duer_dcs_data_report_internal(data, DUER_TRUE);

RET:
    if (s_is_initialized && (rs != DUER_OK) && (rs != DUER_ERR_INVALID_PARAMETER)) {
        duer_ds_log_dcs_event_report_fail(s_play_cmd_name_tab[cmd]);
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

void duer_dcs_play_control_init_internal(void)
{
    s_is_initialized = DUER_TRUE;
}

void duer_dcs_play_control_uninitialize_internal(void)
{
    s_is_initialized = DUER_FALSE;
}

