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
 * File: lightduer_dcs_local.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Provide some functions for dcs module local.
 */

#include "lightduer_dcs_local.h"
#include <stdlib.h>
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_mutex.h"
#include "lightduer_random.h"
#include "lightduer_voice.h"
#include "lightduer_connagent.h"
#include "lightduer_engine.h"
#include "lightduer_timestamp.h"

#define RANDOM_NUM_COUNT           3
#define RANDOM_NUM_LEN             8
#define RANDOM_PART_LEN (RANDOM_NUM_COUNT * RANDOM_NUM_LEN)

// resouce path
const char *DCS_DUER_DIRECTIVE_PATH = "duer_directive";
const char *DCS_DUER_PRIVATE_PATH = "duer_private";
const char *DCS_IOTCLOUD_DIRECTIVE_PATH = "duer_iotcloud_directive";
const char *DCS_IOTCLOUD_EVENT_PATH = "duer_iotcloud_event";

// namespace
const char *DCS_AUDIO_PLAYER_NAMESPACE = "ai.dueros.device_interface.audio_player";
const char *DCS_VOICE_OUTPUT_NAMESPACE = "ai.dueros.device_interface.voice_output";
const char *DCS_VOICE_INPUT_NAMESPACE = "ai.dueros.device_interface.voice_input";
const char *DCS_SCREEN_NAMESPACE = "ai.dueros.device_interface.screen";
const char *DCS_SCREEN_EXTENDED_CARD_NAMESPACE = "ai.dueros.device_interface.screen_extended_card";
const char *DCS_IOT_CLOUD_SYSTEM_NAMESPACE = "ai.dueros.device_interface.iot_cloud.system";
const char *DCS_PLAYBACK_CONTROL_NAMESPACE = "ai.dueros.device_interface.playback_controller";
const char *DCS_RECOMMEND_NAMESPACE = "ai.dueros.device_interface.extensions.recommend";

// message keys
const char *DCS_DIRECTIVE_KEY = "directive";
const char *DCS_CLIENT_CONTEXT_KEY = "clientContext";
const char *DCS_EVENT_KEY = "event";
const char *DCS_HEADER_KEY = "header";
const char *DCS_PAYLOAD_KEY = "payload";

// header keys
const char *DCS_NAMESPACE_KEY = "namespace";
const char *DCS_NAME_KEY = "name";
const char *DCS_MESSAGE_ID_KEY = "messageId";
const char *DCS_DIALOG_REQUEST_ID_KEY = "dialogRequestId";

// payload keys
const char *DCS_TOKEN_KEY = "token";
const char *DCS_TYPE_KEY = "type";
const char *DCS_CODE_KEY = "code";
const char *DCS_VOLUME_KEY = "volume";
const char *DCS_URL_KEY = "url";
const char *DCS_ACTIVE_ALERTS_KEY = "activeAlerts";
const char *DCS_TARGET_KEY = "target";
const char *DCS_SCHEDULED_TIME_KEY = "scheduled_time";
const char *DCS_PLAYER_ACTIVITY_KEY = "playerActivity";
const char *DCS_ALL_ALERTS_KEY = "allAlerts";
const char *DCS_ERROR_KEY = "error";
const char *DCS_MUTE_KEY = "mute";
const char *DCS_INITIATOR_KEY = "initiator";
const char *DCS_AUDIO_ITEM_KEY = "audioItem";
const char *DCS_CLEAR_BEHAVIOR_KEY = "clearBehavior";
const char *DCS_PLAY_BEHAVIOR_KEY = "playBehavior";
const char *DCS_INACTIVE_TIME_IN_SECONDS_KEY = "inactiveTimeInSeconds";
const char *DCS_OFFSET_IN_MILLISECONDS_KEY = "offsetInMilliseconds";
const char *DCS_TIMEOUT_IN_MILLISECONDS_KEY = "timeoutInMilliseconds";
const char *DCS_METADATA_KEY = "metadata";
const char *DCS_TEXT_KEY = "text";
const char *DCS_CONNECTION_SWITCH_KEY = "connectionSwitch";
const char *DCS_DESCRIPTION_KEY = "description";
const char *DCS_BLUETOOTH_KEY = "bluetooth";
const char *DCS_MUTED_KEY = "muted";
const char *DCS_UNPARSED_DIRECTIVE_KEY = "unparsedDirective";
const char *DCS_FORMAT_KEY = "format";
const char *DCS_MESSAGE_KEY = "message";
const char *DCS_QUERY_KEY = "query";
const char *DCS_TIME_OF_RECOMMEND = "timeOfRecommendation";

// directive name
const char *DCS_SPEAK_NAME = "Speak";
const char *DCS_PLAY_NAME = "Play";                 // Audio
const char *DCS_STOP_NAME = "Stop";                 // Audio
const char *DCS_CLEAR_QUEUE_NAME = "ClearQueue";    // Audio
const char *DCS_LISTEN_NAME = "Listen";
const char *DCS_GET_STATUS_NAME = "GetStatus";
const char *DCS_STOP_SPEAK_NAME = "StopSpeak";
const char *DCS_SET_BLUETOOTH_NAME = "SetBluetooth";
const char *DCS_RENDER_CARD_NAME = "RenderCard";
const char *DCS_SET_ALERT_NAME = "SetAlert";
const char *DCS_STOP_LISTEN_NAME = "StopListen";
const char *DCS_SET_BLUETOOTH_CONNECTION_NAME = "SetBluetoothConnection";
const char *DCS_SET_MUTE_NAME = "SetMute";
const char *DCS_DELETE_ALERT_NAME = "DeleteAlert";
const char *DCS_RENDER_VOICE_INPUT_TEXT_NAME = "RenderVoiceInputText";
const char *DCS_SET_VOLUME_NAME = "SetVolume";
const char *DCS_ADJUST_VOLUME_NAME = "AdjustVolume";
const char *DCS_TEXT_INPUT_NAME = "TextInput";
const char *DCS_RENDER_WEATHER = "RenderWeather";
const char *DCS_RENDER_PLAYER_INFO = "RenderPlayerInfo";
extern const char *DCS_RENDER_AUDIO_LIST = "RenderAudioList";
extern const char *DCS_RENDER_ALBUM_LIST = "RenderAlbumList";

// internal directive.
const char *DCS_DIALOGUE_FINISHED_NAME = "DialogueFinished";
const char *DCS_THROW_EXCEPTION_NAME = "ThrowException";
const char *DCS_RESET_USER_INACTIVITY_NAME = "ResetUserInactivity";
const char *DCS_NOP_NAME = "Nop";
const char *DCS_IOT_CLOUD_CONTEXT = "Context";

// event name
const char *DCS_SYNCHRONIZE_STATE_NAME = "SynchronizeState";
const char *DCS_EXCEPTION_ENCOUNTERED_NAME = "ExceptionEncountered";
const char *DCS_VOLUME_STATE_NAME = "Volume";
const char *DCS_LISTEN_STARTED_NAME = "ListenStarted";
const char *DCS_SPEECH_STARTED_NAME = "SpeechStarted";
const char *DCS_ALERTS_STATE_NAME = "AlertsState";
const char *DCS_LISTEN_TIMED_OUT_NAME = "ListenTimedOut";
const char *DCS_EXITED_NAME = "Exited";
const char *DCS_PLAYBACK_STATE_NAME = "PlaybackState";
const char *DCS_SPEECH_FINISHED_NAME = "SpeechFinished";
const char *DCS_VOLUME_CHANGED_NAME = "VolumeChanged";
const char *DCS_MUTE_CHANGED_NAME = "MuteChanged";
const char *DCS_SPEECH_STATE_NAME = "SpeechState";
const char *DCS_LINK_CLICKED_NAME = "LinkClicked";
const char *DCS_RECOMMEND_NAME = "RecommendationRequested";

// internal exception type
const char *DCS_UNEXPECTED_INFORMATION_RECEIVED_TYPE = "UNEXPECTED_INFORMATION_RECEIVED";
const char *DCS_INTERNAL_ERROR_TYPE = "INTERNAL_ERROR";
const char *DCS_UNSUPPORTED_OPERATION_TYPE = "UNSUPPORTED_OPERATION";

static duer_mutex_t s_dcs_local_lock;
static char *s_dialog_req_id;
static int s_uuid_len;
static char *s_msg_id;
static duer_bool s_is_dialog_id_exist;

int duer_dcs_data_report_internal(baidu_json *data, duer_bool is_transparent)
{
    int rs = DUER_OK;

    if (!data) {
        return DUER_ERR_INVALID_PARAMETER;
    }

    if (!is_transparent) {
        duer_add_translate_flag(data);
    }

    rs = duer_data_report(data);

    return rs;
}

static const char *duer_create_random_id(char *buf)
{
    static int count = 0;
    int eof_pos = s_uuid_len;
    int size = RANDOM_PART_LEN + 1;

    if (buf) {
        DUER_SNPRINTF(buf + eof_pos, size, "%08x%08x%08x",
                      duer_random(), duer_timestamp(), count++);
    }

    return buf;
}

const char *duer_create_request_id_internal()
{
    duer_create_random_id(s_dialog_req_id);
    duer_cancel_caching_directive_internal();
    DUER_LOGI("Current dialog id: %s", s_dialog_req_id);
    s_is_dialog_id_exist = DUER_TRUE;

    return s_dialog_req_id;
}

const char *duer_get_request_id_internal(void)
{
    if (!s_is_dialog_id_exist) {
        return duer_create_request_id_internal();
    }

    return s_dialog_req_id;
}

const char *duer_generate_msg_id_internal()
{
    return duer_create_random_id(s_msg_id);
}

char *duer_strdup_internal(const char *str)
{
    int len = 0;
    char *dest = NULL;

    if (!str) {
        return NULL;
    }

    len = DUER_STRLEN(str);
    dest = DUER_MALLOC(len + 1);
    if (!dest) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return NULL;
    }

    DUER_SNPRINTF(dest, len + 1, "%s", str);
    return dest;
}

void duer_play_channel_control_internal(duer_dcs_channel_switch_event_t event)
{
    switch (event) {
    case DCS_RECORD_STARTED:
        duer_speech_on_stop_internal();
        if (duer_audio_is_playing_internal()) {
            duer_pause_audio_internal(DUER_TRUE);
        }
        break;
    case DCS_SPEECH_NEED_PLAY:
        if (duer_audio_is_playing_internal()) {
            duer_pause_audio_internal(DUER_TRUE);
        } else if (duer_is_recording_internal()) {
            duer_listen_stop_internal();
        } else {
            // do nothing
        }
        break;
    case DCS_SPEECH_FINISHED:
        if (duer_is_multiple_round_dialogue()) {
            duer_open_mic_internal();
        } else if (duer_audio_is_paused_internal() && !duer_wait_dialog_finished_internal()) {
            duer_resume_audio_internal();
        } else {
            // do nothing
        }
        break;
    case DCS_AUDIO_NEED_PLAY:
        if (duer_speech_need_play_internal() || duer_is_recording_internal()) {
            duer_pause_audio_internal(DUER_FALSE);
        } else {
            duer_start_audio_play_internal();
        }
        break;
    case DCS_DIALOG_FINISHED:
        if (duer_speech_need_play_internal()
            || duer_is_recording_internal()
            || duer_is_multiple_round_dialogue()) {
            break;
        }
        if (duer_audio_is_paused_internal()) {
            duer_resume_audio_internal();
        }
        break;
    case DCS_AUDIO_FINISHED:
        break;
    case DCS_NEED_OPEN_MIC:
        if (duer_speech_need_play_internal()) {
            break;
        }
        if (duer_audio_is_playing_internal()) {
            duer_pause_audio_internal(DUER_TRUE);
        }
        duer_open_mic_internal();
        break;
    case DCS_AUDIO_STOPPED:
        break;
    default:
        break;
    }
}

duer_status_t duer_dcs_critical_enter_internal()
{
    return duer_mutex_lock(s_dcs_local_lock);
}

duer_status_t duer_dcs_critical_exit_internal()
{
    return duer_mutex_unlock(s_dcs_local_lock);
}

void duer_dcs_local_init_internal(void)
{
    const char *uuid = NULL;
    int len = 0;

    if (!s_dcs_local_lock) {
        s_dcs_local_lock = duer_mutex_create();
        if (!s_dcs_local_lock) {
            DUER_LOGE("Failed to create s_dcs_local_lock");
        }
    }

    uuid = duer_engine_get_uuid();
    if (uuid) {
        s_uuid_len = DUER_STRLEN(uuid);
        len = s_uuid_len + RANDOM_PART_LEN + 1;
        if (!s_dialog_req_id) {
            s_dialog_req_id = DUER_MALLOC(len);
            if (s_dialog_req_id) {
                DUER_SNPRINTF(s_dialog_req_id, len, "%s", uuid);
            } else {
                DUER_LOGE("Failed malloc s_dialog_id");
            }
        }

        if (!s_msg_id) {
            s_msg_id = DUER_MALLOC(len);
            if (s_msg_id) {
                DUER_SNPRINTF(s_msg_id, len, "%s", uuid);
            } else {
                DUER_LOGE("Failed malloc s_msg_id");
            }
        }
    } else {
        DUER_LOGE("Failed to get uuid");
    }

}

void duer_dcs_local_uninitialize_internal(void)
{
    s_uuid_len = 0;

    if (s_dialog_req_id) {
        DUER_FREE(s_dialog_req_id);
        s_dialog_req_id = NULL;
    }

    if (s_msg_id) {
        DUER_FREE(s_msg_id);
        s_msg_id = NULL;
    }
}

