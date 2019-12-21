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
 * File: lightduer_dcs_voice_output.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS voice output interface
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_ds_log_e2e.h"

static char *s_latest_token = NULL;

enum _play_state {
    PLAYING,
    FINISHED,
    MAX_PLAY_STATE,
};

static const char* const s_player_activity[MAX_PLAY_STATE] = {"PLAYING", "FINISHED"};
static volatile int s_play_state = FINISHED;
static volatile duer_bool s_is_initialized;

static int duer_report_speech_event(const char *name)
{
    baidu_json *data = NULL;
    baidu_json *event = NULL;
    baidu_json *payload = NULL;
    int rs = DUER_OK;

    if (!s_latest_token) {
        DUER_LOGW("No token was stored!");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    event = duer_create_dcs_event(DCS_VOICE_OUTPUT_NAMESPACE,
                                  name,
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
    baidu_json_AddStringToObjectCS(payload, DCS_TOKEN_KEY, s_latest_token);

    duer_dcs_data_report_internal(data, DUER_FALSE);

RET:
    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail(name);
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

void duer_dcs_speech_on_finished()
{
    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        duer_speak_directive_finished_internal();
        if (s_play_state == PLAYING) {
            s_play_state = FINISHED;
            duer_report_speech_event(DCS_SPEECH_FINISHED_NAME);
        }
        duer_play_channel_control_internal(DCS_SPEECH_FINISHED);
    } else {
        DUER_LOGW("DCS voice output has not been initialized");
    }
    DUER_DCS_CRITICAL_EXIT();
}

void duer_speech_on_stop_internal()
{
    if (s_is_initialized) {
        s_play_state = FINISHED;
    } else {
        DUER_LOGW("DCS voice output has not been initialized");
    }
}

static duer_status_t duer_speak_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *url = NULL;
    baidu_json *token = NULL;

    DUER_LOGV("Entry");

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    url = baidu_json_GetObjectItem(payload, DCS_URL_KEY);
    token = baidu_json_GetObjectItem(payload, DCS_TOKEN_KEY);

    if (!url || !token) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    DUER_DCS_CRITICAL_ENTER();

    if (s_latest_token) {
        DUER_FREE(s_latest_token);
    }
    s_latest_token = duer_strdup_internal(token->valuestring);
    if (!s_latest_token) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        DUER_LOGE("Memory too low");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    s_play_state = PLAYING;

    duer_play_channel_control_internal(DCS_SPEECH_NEED_PLAY);
    duer_report_speech_event(DCS_SPEECH_STARTED_NAME);
    duer_ds_e2e_event(DUER_E2E_PLAY);
    DUER_DCS_CRITICAL_EXIT();

    duer_dcs_speak_handler(url->valuestring);

    return DUER_OK;
}

static duer_status_t duer_stop_speak_cb(const baidu_json *directive)
{
    duer_dcs_stop_speak_handler();

    DUER_DCS_CRITICAL_ENTER();
    if (s_play_state == PLAYING) {
        s_play_state = FINISHED;
        duer_play_channel_control_internal(DCS_SPEECH_FINISHED);
    }
    DUER_DCS_CRITICAL_EXIT();

    return DUER_OK;
}

duer_bool duer_speech_need_play_internal(void)
{
    return s_play_state == PLAYING;
}

baidu_json* duer_get_speech_state_internal()
{
    baidu_json *state = NULL;
    baidu_json *payload = NULL;

    state = duer_create_dcs_event(DCS_VOICE_OUTPUT_NAMESPACE, DCS_SPEECH_STATE_NAME, DUER_FALSE);
    if (!state) {
        DUER_LOGE("Filed to create SpeechState event");
        goto error_out;
    }

    payload = baidu_json_GetObjectItem(state, DCS_PAYLOAD_KEY);
    if (!payload) {
        DUER_LOGE("Filed to get payload item");
        goto error_out;
    }

    baidu_json_AddStringToObjectCS(payload, DCS_TOKEN_KEY, s_latest_token ? s_latest_token : "");
    baidu_json_AddNumberToObjectCS(payload, DCS_OFFSET_IN_MILLISECONDS_KEY, 0);
    baidu_json_AddStringToObjectCS(payload, DCS_PLAYER_ACTIVITY_KEY, s_player_activity[s_play_state]);

    return state;

error_out:
    if (state) {
        baidu_json_Delete(state);
    }

    return NULL;
}

void duer_dcs_voice_output_init(void)
{
    DUER_DCS_CRITICAL_ENTER();
    if (!s_is_initialized) {
        duer_directive_list res[] = {
            {DCS_SPEAK_NAME, duer_speak_cb},
            {DCS_STOP_SPEAK_NAME, duer_stop_speak_cb},
        };

        duer_add_dcs_directive_internal(res,
                                        sizeof(res) / sizeof(res[0]),
                                        DCS_VOICE_OUTPUT_NAMESPACE);
        duer_reg_client_context_cb_internal(duer_get_speech_state_internal);
        s_is_initialized = DUER_TRUE;
    } else {
        DUER_LOGW("Voice input module has been initialized");
    }
    DUER_DCS_CRITICAL_EXIT();
}

void duer_dcs_voice_output_uninitialize_internal(void)
{
    if (!s_is_initialized) {
        DUER_LOGW("Voice input module has not been initialized");
        return;
    }

    s_is_initialized = DUER_FALSE;

    if (s_latest_token) {
        DUER_FREE(s_latest_token);
        s_latest_token = NULL;
    }
}

