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
 * File: lightduer_dcs_voice_input.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS voice input interface
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_event_emitter.h"
#include "lightduer_log.h"
#include "lightduer_timers.h"
#include "lightduer_mutex.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_ds_log_e2e.h"
#include "lightduer_voice.h"

static int s_wait_time = 0;
static duer_timer_handler s_listen_timer;
static volatile duer_bool s_is_multiple_dialog = DUER_FALSE;

#define LIMIT_LISTEN_TIME

#ifdef LIMIT_LISTEN_TIME
// If no listen stop directive received in 60s, close the recorder
static const int MAX_LISTEN_TIME = 60000;
static duer_timer_handler s_close_mic_timer;
#endif
static volatile duer_bool s_is_listening;
static baidu_json *s_initiator;
static volatile duer_bool s_is_initialized;

duer_bool duer_is_recording_internal(void)
{
    return s_is_listening;
}

void duer_listen_stop_internal(void)
{
#ifdef LIMIT_LISTEN_TIME
    if (s_is_listening && s_close_mic_timer) {
        duer_timer_stop(s_close_mic_timer);
    }
#endif
    if (s_is_listening) {
        s_is_listening = DUER_FALSE;
        DUER_DCS_CRITICAL_EXIT();
        duer_dcs_stop_listen_handler();
        DUER_DCS_CRITICAL_ENTER();
    }
}

int duer_dcs_on_listen_started(void)
{
    baidu_json *event = NULL;
    baidu_json *header = NULL;
    baidu_json *payload = NULL;
    baidu_json *data = NULL;
    baidu_json *client_context = NULL;
    int rs = DUER_OK;
    duer_voice_mode user_mode = DUER_VOICE_MODE_DEFAULT;
    const char *dialog_id = NULL;

    user_mode = duer_voice_get_mode();
    DUER_DCS_CRITICAL_ENTER();
    if (!s_is_initialized) {
        DUER_LOGW("DCS voice input module has not been intialized");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

#ifdef LIMIT_LISTEN_TIME
    if (s_close_mic_timer) {
        if (s_is_listening) {
            duer_timer_stop(s_close_mic_timer);
        }
        duer_timer_start(s_close_mic_timer, MAX_LISTEN_TIME);
    }
#endif
    s_is_listening = DUER_TRUE;

    if (s_is_multiple_dialog && (s_wait_time != 0) && (s_listen_timer != NULL)) {
        duer_timer_stop(s_listen_timer);
    }

    if (user_mode == DUER_VOICE_MODE_DEFAULT || user_mode == DUER_VOICE_MODE_C2E_BOT) {
        duer_user_activity_internal();
    }
    s_is_multiple_dialog = DUER_FALSE;
    duer_play_channel_control_internal(DCS_RECORD_STARTED);

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    client_context = duer_get_client_context_internal();
    if (client_context) {
        baidu_json_AddItemToObjectCS(data, DCS_CLIENT_CONTEXT_KEY, client_context);
    }

    event = duer_create_dcs_event(DCS_VOICE_INPUT_NAMESPACE,
                                  DCS_LISTEN_STARTED_NAME,
                                  DUER_TRUE);
    if (event == NULL) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObjectCS(data, DCS_EVENT_KEY, event);

    header = baidu_json_GetObjectItem(event, DCS_HEADER_KEY);
    dialog_id = duer_create_request_id_internal();
    if (!dialog_id) {
        DUER_LOGE("Failed to create dialog id");
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    duer_ds_e2e_event(DUER_E2E_REQUEST);
    baidu_json_AddStringToObjectCS(header, DCS_DIALOG_REQUEST_ID_KEY, dialog_id);

    payload = baidu_json_GetObjectItem(event, DCS_PAYLOAD_KEY);
    baidu_json_AddStringToObjectCS(payload, DCS_FORMAT_KEY, "AUDIO_L16_RATE_16000_CHANNELS_1");

    if (s_initiator) {
        baidu_json_AddItemReferenceToObject(payload, DCS_INITIATOR_KEY, s_initiator);
    }
    rs = duer_dcs_data_report_internal(data, DUER_FALSE);
    if (s_initiator) {
        baidu_json_Delete(s_initiator);
        s_initiator = NULL;
    }

RET:
    DUER_DCS_CRITICAL_EXIT();

    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail(DCS_LISTEN_STARTED_NAME);
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

static int duer_dcs_report_listen_timeout_event(void)
{
    baidu_json *event = NULL;
    baidu_json *data = NULL;
    int rs = DUER_OK;

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    event = duer_create_dcs_event(DCS_VOICE_INPUT_NAMESPACE,
                                  DCS_LISTEN_TIMED_OUT_NAME,
                                  DUER_TRUE);
    if (event == NULL) {
        rs = DUER_ERR_FAILED;
        goto RET;
    }

    baidu_json_AddItemToObjectCS(data, DCS_EVENT_KEY, event);
    rs = duer_dcs_data_report_internal(data, DUER_FALSE);
RET:
    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail(DCS_LISTEN_TIMED_OUT_NAME);
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

void duer_open_mic_internal(void)
{
    if ((s_wait_time != 0) && (s_listen_timer != NULL)) {
        duer_timer_start(s_listen_timer, s_wait_time);
    }
#ifndef DISABLE_OPEN_MIC_AUTOMATICALLY
    DUER_DCS_CRITICAL_EXIT();
    duer_dcs_listen_handler();
    DUER_DCS_CRITICAL_ENTER();
#endif
}

static duer_status_t duer_listen_start_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *timeout = NULL;
    baidu_json *initiator = NULL;

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    DUER_DCS_CRITICAL_ENTER();
    if (s_initiator) {
        baidu_json_Delete(s_initiator);
        s_initiator = NULL;
    }

    initiator = baidu_json_GetObjectItem(payload, DCS_INITIATOR_KEY);
    if (initiator) {
        s_initiator = baidu_json_Duplicate(initiator, 1);
    }

    timeout = baidu_json_GetObjectItem(payload, DCS_TIMEOUT_IN_MILLISECONDS_KEY);
    if (!timeout) {
        DUER_DCS_CRITICAL_EXIT();
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    s_wait_time = timeout->valueint;
    s_is_multiple_dialog = DUER_TRUE;
    DUER_LOGD("%d", s_wait_time);

    duer_play_channel_control_internal(DCS_NEED_OPEN_MIC);
    DUER_DCS_CRITICAL_EXIT();

    return DUER_OK;
}

static duer_status_t duer_listen_stop_cb(const baidu_json *directive)
{
    DUER_DCS_CRITICAL_ENTER();
#ifdef LIMIT_LISTEN_TIME
    if (s_is_listening && s_close_mic_timer) {
        duer_timer_stop(s_close_mic_timer);
    }
#endif
    if (s_is_listening) {
        s_is_listening = DUER_FALSE;
        DUER_DCS_CRITICAL_EXIT();
        duer_dcs_stop_listen_handler();
    } else {
        DUER_DCS_CRITICAL_EXIT();
    }

    return DUER_OK;
}

int duer_dcs_on_listen_stopped(void)
{
    DUER_DCS_CRITICAL_ENTER();
    if (!s_is_initialized) {
        DUER_LOGW("DCS voice input module has not been intialized");
        DUER_DCS_CRITICAL_EXIT();
        return DUER_ERR_FAILED;
    }
#ifdef LIMIT_LISTEN_TIME
    if (s_is_listening && s_close_mic_timer) {
        duer_timer_stop(s_close_mic_timer);
    }
#endif
    duer_ds_e2e_event(DUER_E2E_RECORD_FINISH);
    s_is_listening = DUER_FALSE;
    DUER_DCS_CRITICAL_EXIT();
    return DUER_OK;
}

#ifdef LIMIT_LISTEN_TIME
static void duer_stop_listen_missing_handler(int what, void *param)
{
    duer_ds_log_dcs(DUER_DS_LOG_DCS_STOP_LISTEN_MISSING, NULL);

    DUER_DCS_CRITICAL_ENTER();
    if (s_is_listening) {
        s_is_listening = DUER_FALSE;
        DUER_DCS_CRITICAL_EXIT();
        duer_dcs_stop_listen_handler();
        return;
    }

    DUER_DCS_CRITICAL_EXIT();
}

static void duer_stop_listen_missing(void *param)
{
    duer_emitter_emit(duer_stop_listen_missing_handler, 0, NULL);
}
#endif

static void duer_listen_timeout_handler(int what, void *param)
{
    DUER_DCS_CRITICAL_ENTER();
    s_is_multiple_dialog = DUER_FALSE;
    if (s_initiator) {
        baidu_json_Delete(s_initiator);
        s_initiator = NULL;
    }
    duer_dcs_report_listen_timeout_event();
    DUER_DCS_CRITICAL_EXIT();
}

static void duer_listen_timeout(void *param)
{
    duer_emitter_emit(duer_listen_timeout_handler, 0, NULL);
}

duer_bool duer_is_multiple_round_dialogue()
{
    return s_is_multiple_dialog;
}

void duer_dcs_voice_input_init(void)
{
    DUER_DCS_CRITICAL_ENTER();

    if (s_is_initialized) {
        DUER_DCS_CRITICAL_EXIT();
        DUER_LOGI("Voice input module has been initialized");
        return;
    }

#ifdef LIMIT_LISTEN_TIME
    s_close_mic_timer = duer_timer_acquire(duer_stop_listen_missing, NULL, DUER_TIMER_ONCE);
    if (!s_close_mic_timer) {
        DUER_LOGE("Failed to create timer to close mic");
    }
#endif

    s_listen_timer = duer_timer_acquire(duer_listen_timeout, NULL, DUER_TIMER_ONCE);
    if (!s_listen_timer) {
        DUER_LOGE("Failed to create listen timeout timer");
    }

    duer_directive_list res[] = {
        {DCS_LISTEN_NAME, duer_listen_start_cb},
        {DCS_STOP_LISTEN_NAME, duer_listen_stop_cb},
    };

    duer_add_dcs_directive_internal(res, sizeof(res) / sizeof(res[0]), DCS_VOICE_INPUT_NAMESPACE);
    s_is_initialized = DUER_TRUE;
    DUER_DCS_CRITICAL_EXIT();
}

void duer_dcs_voice_input_uninitialize_internal(void)
{
    if (!s_is_initialized) {
        return;
    }

    s_is_initialized = DUER_FALSE;
#ifdef LIMIT_LISTEN_TIME
    if (s_close_mic_timer) {
        duer_timer_release(s_close_mic_timer);
        s_close_mic_timer = NULL;
    }
#endif

    if (s_listen_timer) {
        duer_timer_release(s_listen_timer);
        s_listen_timer = NULL;
    }

    s_wait_time = 0;
    s_is_multiple_dialog = DUER_FALSE;
    s_is_listening = DUER_FALSE;
}

