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

/**
 * DESC:
 * Developer needs to implement this interface to play speech.
 *
 * PARAM:
 * @param[in] url: the url of the speech need to play
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_speak_handler(const char *url)
{
    DUER_LOGW("Please implement this interface to play speech");
}

/**
 * DESC:
 * Developer needs to implement this interface to play audio.
 *
 * PARAM:
 * @param[in] url: the url of the audio need to play
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t *audio_info)
{
    DUER_LOGW("Please implement this interface to play audio");
}

/**
 * DESC:
 * Developer needs to implement this interface to stop audio player.
 *
 * PARAM: none
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_audio_stop_handler(void)
{
    DUER_LOGW("Please implement this interface to stop audio player");
}

/**
 * DESC:
 * Developer needs to implement this interface to resume audio play.
 *
 * PARAM:
 * @param[in] url: the url of the audio need to play
 * @param[in] offset: the position of the audio need to seek
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_audio_seek_handler(const char *url, int offset)
{
    DUER_LOGW("Please implement this interface to seek audio play");
}

/**
 * DESC:
 * Developer needs to implement this interface to pause audio play.
 *
 * PARAM: none
 *
 * @RETURN: the play position of the current audio.
 */
__attribute__((weak)) int duer_dcs_audio_pause_handler(void)
{
    DUER_LOGW("Please implement this interface to pause audio player");
    return 0;
}

/**
 * DESC:
 * Developer needs to implement this interface, it is used to get volume state.
 *
 * @param[out] volume: current volume value.
 * @param[out] is_mute: current mute state.
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_get_speaker_state(int *volume, bool *is_mute)
{
    DUER_LOGW("Please implement this interface to get speaker state");
}

/**
 * DESC:
 * Developer needs to implement this interface to set volume.
 *
 * PARAM:
 * @param[in] volume: the value of the volume need to set
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_volume_set_handler(int volume)
{
    DUER_LOGW("Please implement this interface to set volume");
}

/**
 * DESC:
 * Developer needs to implement this interface to adjust volume.
 *
 * PARAM:
 * @param[in] volume: the value need to adjusted.
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_volume_adjust_handler(int volume)
{
    DUER_LOGW("Please implement this interface to adjust volume");
}

/**
 * DESC:
 * Developer needs to implement this interface to change mute state.
 *
 * PARAM:
 * @param[in] is_mute: set/discard mute.
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_mute_handler(bool is_mute)
{
    DUER_LOGW("Please implement this interface to set mute state");
}

/**
 * DESC:
 * Developer needs to implement this interface to start recording.
 *
 * PARAM: none
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_listen_handler(void)
{
    DUER_LOGW("Please implement this interface to start recording");
}

/**
 * DESC:
 * Developer needs to implement this interface to stop recording.
 *
 * PARAM: none
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_stop_listen_handler(void)
{
    DUER_LOGW("Please implement this interface to stop recording");
}

/**
 * DESC:
 * Developer needs to implement this interface to set alert.
 *
 * PARAM:
 * @param[in] token: the token of the alert, it's the ID of the alert.
 * @param[in] time: the schedule time of the alert, ISO 8601 format.
 * @param[in] type: the type of the alert, TIMER or ALARM.
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_alert_set_handler(const char *token,
                                                      const char *time,
                                                      const char *type)
{
    DUER_LOGW("Please implement this interface to set alert");
}

/**
 * DESC:
 * Developer needs to implement this interface to delete alert.
 *
 * PARAM:
 * @param[in] token: the token of the alert need to delete.
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_alert_delete_handler(const char *token)
{
    DUER_LOGW("Please implement this interface to delete alert");
}

/**
 * DESC:
 * It's used to get all alert info by DCS.
 * Developer can implement this interface by call duer_dcs_report_alert to report all alert info.
 *
 * PARAM:
 * @param[in] token: the token of the alert need to delete.
 *
 * @RETURN: none.
 */
__attribute__((weak)) void duer_dcs_get_all_alert(baidu_json *alert_array)
{
    DUER_LOGW("Please implement this interface to report all alert info");
}

