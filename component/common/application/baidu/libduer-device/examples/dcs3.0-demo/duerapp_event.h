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
 * File: event.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Duer Application Key definition.
 *       Blocking loop function.
 */

#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_EVENT_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_EVENT_H

enum duer_kbd_events{
    PLAY_PAUSE    = 0x7A,  // z
    RECORD_START  = 0x78,  // x
    PREVIOUS_SONG = 0x61,  // a
    VOICE_MODE    = 0x63,  // c
    NEXT_SONG     = 0x64,  // d
    VOLUME_INCR   = 0x77,  // w
    VOLUME_DECR   = 0x73,  // s
    VOLUME_MUTE   = 0x65,  // e
    QUIT          = 0x71,  // q
};

void duer_event_loop();
void duer_voice_mode_translate_record();

#endif // BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_EVENT_H
