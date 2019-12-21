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
 * File: duerapp_event.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Duer Application Key functions to achieve.
 */

#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "duerapp_event.h"
#include "lightduer_dcs.h"
#include "lightduer_voice.h"
#include "duerapp_recorder.h"
#include "duerapp_media.h"
#include "duerapp_config.h"
#include "duerapp_alert.h"
#include "duerapp.h"

#define VOLUME_STEP (5)

static bool s_is_record = false;

static void event_play_puase()
{
    DUER_LOGV("KEY_DOWN");
    if (duer_alert_bell()) {
        duer_alert_stop();
        return;
    }
    if(DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode()) {
        duer_audio_state_t audio_state = duer_media_audio_state();
        if (MEDIA_AUDIO_PLAY == audio_state) {
            duer_dcs_send_play_control_cmd(DCS_PAUSE_CMD);
        } else if (MEDIA_AUDIO_PAUSE == audio_state) {
            duer_dcs_send_play_control_cmd(DCS_PLAY_CMD);
        } else {
            // do nothing
        }
    }
}

static void duer_voice_mode_default_record()
{
    if(RECORDER_START == duer_get_recorder_state()) {
        duer_recorder_suspend();
    }
    if (DUER_OK != duer_dcs_on_listen_started()) {
        DUER_LOGE("duer_dcs_on_listen_started failed!");
    } else{
        duer_alert_stop();
        duer_media_speak_stop();
        duer_recorder_start();
        s_is_record = true;
    }
}

void duer_voice_mode_translate_record()
{
    static bool rec_start = false;
    if (rec_start) {
        rec_start = false;
        duer_recorder_stop();
        duer_dcs_on_listen_stopped();
    } else {
        if (DUER_OK != duer_dcs_on_listen_started()) {
            DUER_LOGE("duer_dcs_on_listen_started failed!");
        } else{
            rec_start = true;
            duer_alert_stop();
            duer_media_speak_stop();
            duer_recorder_start();
        }
    }
}

static void event_record_start()
{
    DUER_LOGV("KEY_DOWN");
    if (DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode()){
        duer_voice_mode_default_record();
    } else {
        duer_voice_mode_translate_record();
    }
}

static void event_previous_song()
{
    DUER_LOGV("KEY_DOWN");
    if(DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode()) {
        duer_dcs_send_play_control_cmd(DCS_PREVIOUS_CMD);
    }
}

static void event_next_song()
{
    DUER_LOGV("KEY_DOWN");
    if(DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode()) {
        duer_dcs_send_play_control_cmd(DCS_NEXT_CMD);
    }
}

static void event_volume_incr()
{
    DUER_LOGV("KEY_DOWN");
    duer_media_volume_change(VOLUME_STEP);
}

static void event_volume_decr()
{
    DUER_LOGV("KEY_DOWN");
    duer_media_volume_change(-VOLUME_STEP);
}

static void event_volune_mute()
{
    static bool mute = false;
    DUER_LOGI("KEY_DOWN : %d", mute);
    if (mute) {
        mute = false;
    } else {
        mute = true;
    }
    duer_media_set_mute(mute);
}

static void event_voice_mode()
{
    static duer_voice_mode mode = DUER_VOICE_MODE_DEFAULT;
    mode = (mode + 1) % 3;
    duer_dcs_close_multi_dialog();
    duer_media_speak_stop();
    if (MEDIA_AUDIO_PLAY == duer_media_audio_state()) {
        duer_media_audio_stop();
        duer_dcs_audio_on_stopped();
    }
    duer_voice_set_mode(mode);
    DUER_LOGI("Current speech interaction mode： %d", mode);
}

void duer_event_loop()
{
    struct termios kbd_ops;
    struct termios kbd_bak;
    int kbd_fd = 0;
    char kbd_event = 0;
    bool loop_state = true;

    tcgetattr(kbd_fd, &kbd_ops);
    memcpy(&kbd_bak, &kbd_ops, sizeof(struct termios));
    kbd_ops.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(kbd_fd, TCSANOW, &kbd_ops);

    while(loop_state) {
        if (read(kbd_fd, &kbd_event, 1) < 0) {
            continue;
        }
        if (!duer_app_get_connect_state()) {
            if (QUIT == kbd_event) {
                loop_state = false;
            }
            continue;
        }
        switch (kbd_event) {
            case PLAY_PAUSE :
                event_play_puase();
                break;
            case RECORD_START :
                event_record_start();
                break;
            case VOICE_MODE :
                event_voice_mode();
                break;
            case PREVIOUS_SONG :
                event_previous_song();
                break;
            case NEXT_SONG :
                event_next_song();
                break;
            case VOLUME_INCR :
                event_volume_incr();
                break;
            case VOLUME_DECR :
                event_volume_decr();
                break;
            case VOLUME_MUTE :
                event_volune_mute();
                break;
            case QUIT :
                loop_state = false;
                break;
            default :
                break;
        }
    }

    tcsetattr(kbd_fd, TCSANOW, &kbd_bak);
}


void duer_dcs_stop_listen_handler(void)
{
    duer_recorder_stop();
    s_is_record = false;
    DUER_LOGI("Listen stop");
}

void duer_dcs_listen_handler(void)
{
    event_record_start();
    DUER_LOGI("Listen");
}
