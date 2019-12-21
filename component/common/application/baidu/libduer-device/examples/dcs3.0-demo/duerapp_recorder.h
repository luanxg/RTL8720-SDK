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
 * File: duerapp_recorder.h
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Record module API.
 */

#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_RECORDER_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_RECORDER_H

#include <alsa/asoundlib.h>

typedef enum{
    RECORDER_START,
    RECORDER_STOP
}duer_rec_state_t;

typedef struct{
    int dir;
    int size;
    unsigned int val;
    snd_pcm_t *handle;
    snd_pcm_uframes_t frames;
    snd_pcm_hw_params_t *params;
}duer_rec_config_t;

int duer_recorder_start();
int duer_recorder_stop();
int duer_recorder_suspend();
duer_rec_state_t duer_get_recorder_state();

#endif // BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_RECORDER_H
