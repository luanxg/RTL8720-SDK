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
 * File: lightduer_dcs_dummy.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Define some weak symbols for DCS module.
 *       Developers need to implement these APIs according to their requirements.
 */
#include "lightduer_dcs.h"
#include "lightduer_log.h"
#include "lightduer_dcs_alert.h"
#include "lightduer_ds_log_dcs.h"

#define DUER_DCS_HANDLER_UNREALIZE(X) \
    do { \
        if (X) { \
            X = DUER_FALSE; \
            DUER_DS_LOG_REPORT_DCS_HANDLER_UNREALIZE(); \
        } \
    } while (0) \

__attribute__((weak)) void duer_dcs_speak_handler(const char *url)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to play speech");
}

__attribute__((weak)) void duer_dcs_stop_speak_handler(void)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to stop speech");
}

__attribute__((weak)) void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t *audio_info)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to play audio");
}

__attribute__((weak)) void duer_dcs_audio_stop_handler(void)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to stop audio player");
}

__attribute__((weak)) void duer_dcs_audio_resume_handler(const duer_dcs_audio_info_t *audio_info)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to resume audio player");
}

__attribute__((weak)) void duer_dcs_audio_pause_handler(void)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to pause audio player");
}

__attribute__((weak)) int duer_dcs_audio_get_play_progress(void)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface, it's used to get the audio play progress");
    return DUER_ERR_FAILED;
}

__attribute__((weak)) void duer_dcs_get_speaker_state(int *volume, duer_bool *is_mute)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to get speaker state");
}

__attribute__((weak)) void duer_dcs_volume_set_handler(int volume)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to set volume");
}

__attribute__((weak)) void duer_dcs_volume_adjust_handler(int volume)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to adjust volume");
}

__attribute__((weak)) void duer_dcs_mute_handler(duer_bool is_mute)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to set mute state");
}

__attribute__((weak)) void duer_dcs_listen_handler(void)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to start recording");
}

__attribute__((weak)) void duer_dcs_stop_listen_handler(void)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to stop recording");
}

__attribute__((weak)) duer_status_t duer_dcs_tone_alert_set_handler(const baidu_json *directive)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to set alert");
    return DUER_OK;
}

__attribute__((weak)) void duer_dcs_alert_delete_handler(const char *token)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to delete alert");
}

__attribute__((weak)) void duer_dcs_get_all_alert(baidu_json *alert_array)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to report all alert info");
}

__attribute__((weak)) duer_status_t duer_dcs_input_text_handler(const char *text, const char *type)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to get the ASR result");
    return DUER_OK;
}

__attribute__((weak)) duer_status_t duer_dcs_render_card_handler(baidu_json *payload)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to report get render card");
    return DUER_OK;
}

__attribute__((weak)) void duer_dcs_bluetooth_set_handler(duer_bool is_switch, const char *target)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to open/close bluetooth");
}

__attribute__((weak)) void duer_dcs_bluetooth_connect_handler(duer_bool is_connect,
                                                              const char *target)
{
    static duer_bool is_first_time = DUER_TRUE;

    DUER_DCS_HANDLER_UNREALIZE(is_first_time);
    DUER_LOGW("Please implement this interface to connect/disconnect bluetooth");
}

