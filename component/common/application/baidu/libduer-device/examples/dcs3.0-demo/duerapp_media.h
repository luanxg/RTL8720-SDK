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
 * File: duerapp_media.h
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Media module API.
 */

#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_MEDIA_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_MEDIA_H

#include "duerapp_config.h"

typedef enum{
    MEDIA_SPEAK_PLAY,
    MEDIA_SPEAK_STOP,
}duer_speak_state_t;

typedef enum{
    MEDIA_AUDIO_PLAY,
    MEDIA_AUDIO_PAUSE,
    MEDIA_AUDIO_STOP,
}duer_audio_state_t;

void duer_media_init();
void duer_media_destroy();

void duer_media_speak_play(const char *url);
void duer_media_speak_stop();

void duer_media_audio_start(const char *url);
void duer_media_audio_resume(const char *url, int offset);
void duer_media_audio_stop();
void duer_media_audio_pause();
int duer_media_audio_get_position();
duer_audio_state_t duer_media_audio_state();

void duer_media_volume_change(int volume);
void duer_media_set_volume(int volume);
int duer_media_get_volume();
void duer_media_set_mute(bool mute);
bool duer_media_get_mute();

#endif // BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_MEDIA_H
