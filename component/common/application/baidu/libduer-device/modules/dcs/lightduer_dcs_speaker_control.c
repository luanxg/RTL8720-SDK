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
 * File: lightduer_dcs_speaker_control.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: apply APIs to support DCS speaker controler interface
 */

#include "lightduer_dcs.h"
#include "lightduer_dcs_router.h"
#include "lightduer_dcs_local.h"
#include "lightduer_connagent.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_dcs.h"

static const char *speaker_control_namespace = "ai.dueros.device_interface.speaker_controller";
static volatile duer_bool s_is_initialized;

static int duer_report_speaker_event(const char *name)
{
    baidu_json *event = NULL;
    baidu_json *payload = NULL;
    baidu_json *data = NULL;
    int volume = 0;
    duer_bool is_mute = DUER_FALSE;
    int rs = DUER_OK;

    data = baidu_json_CreateObject();
    if (data == NULL) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    event = duer_create_dcs_event(speaker_control_namespace,
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

    duer_dcs_get_speaker_state(&volume, &is_mute);
    baidu_json_AddNumberToObjectCS(payload, DCS_VOLUME_KEY, volume);
    baidu_json_AddBoolToObjectCS(payload, DCS_MUTE_KEY, is_mute);

    rs = duer_dcs_data_report_internal(data, DUER_TRUE);

RET:
    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail(name);
    }

    if (data) {
        baidu_json_Delete(data);
    }

    return rs;
}

int duer_dcs_on_volume_changed()
{
    int ret = DUER_OK;

    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        ret = duer_report_speaker_event(DCS_VOLUME_CHANGED_NAME);
    } else {
        ret = DUER_ERR_FAILED;
        DUER_LOGW("DCS has not been initialized");
    }
    DUER_DCS_CRITICAL_EXIT();
    return ret;
}

int duer_dcs_on_mute()
{
    int ret = DUER_OK;

    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        ret = duer_report_speaker_event(DCS_MUTE_CHANGED_NAME);
    } else {
        ret = DUER_ERR_FAILED;
        DUER_LOGW("DCS has not been initialized");
    }
    DUER_DCS_CRITICAL_EXIT();
    return ret;
}

static baidu_json *duer_parse_volume_directive(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *volume = NULL;

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        return NULL;
    }

    volume = baidu_json_GetObjectItem(payload, DCS_VOLUME_KEY);
    return volume;
}

static duer_status_t duer_volume_set_cb(const baidu_json *directive)
{
    baidu_json *volume = NULL;

    volume = duer_parse_volume_directive(directive);
    if (!volume) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    duer_dcs_volume_set_handler(volume->valueint);
    return DUER_OK;
}

static duer_status_t duer_volume_adjust_cb(const baidu_json *directive)
{
    baidu_json *volume = NULL;

    volume = duer_parse_volume_directive(directive);
    if (!volume) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    duer_dcs_volume_adjust_handler(volume->valueint);
    return DUER_OK;
}

static duer_status_t duer_mute_set_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *mute = NULL;

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    mute = baidu_json_GetObjectItem(payload, DCS_MUTE_KEY);
    if (!mute) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    duer_dcs_mute_handler(mute->type == baidu_json_True);

    return DUER_OK;
}

baidu_json *duer_get_speak_control_state_internal()
{
    baidu_json *state = NULL;
    baidu_json *payload = NULL;
    int volume = 0;
    duer_bool is_mute = DUER_FALSE;

    state = duer_create_dcs_event(speaker_control_namespace,
                                  DCS_VOLUME_STATE_NAME,
                                  DUER_FALSE);
    if (!state) {
        DUER_LOGE("Filed to create speaker controler state event");
        goto error_out;
    }

    payload = baidu_json_GetObjectItem(state, DCS_PAYLOAD_KEY);
    if (!payload) {
        DUER_LOGE("Filed to get payload item");
        goto error_out;
    }

    duer_dcs_get_speaker_state(&volume, &is_mute);

    baidu_json_AddNumberToObject(payload, DCS_VOLUME_KEY, volume);
    baidu_json_AddBoolToObject(payload, DCS_MUTED_KEY, is_mute);

    return state;

error_out:
    duer_ds_log_dcs_event_report_fail(DCS_VOLUME_STATE_NAME);

    if (state) {
        baidu_json_Delete(state);
    }

    return NULL;
}

void duer_dcs_speaker_control_init(void)
{
    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        DUER_DCS_CRITICAL_EXIT();
        return;
    }

    duer_directive_list res[] = {
        {DCS_SET_VOLUME_NAME, duer_volume_set_cb},
        {DCS_ADJUST_VOLUME_NAME, duer_volume_adjust_cb},
        {DCS_SET_MUTE_NAME, duer_mute_set_cb},
    };

    duer_add_dcs_directive_internal(res, sizeof(res) / sizeof(res[0]), speaker_control_namespace);
    duer_reg_client_context_cb_internal(duer_get_speak_control_state_internal);

    s_is_initialized = DUER_TRUE;
    DUER_DCS_CRITICAL_EXIT();
}

void duer_dcs_speaker_control_uninitialize_internal(void)
{
    s_is_initialized = DUER_FALSE;
}

