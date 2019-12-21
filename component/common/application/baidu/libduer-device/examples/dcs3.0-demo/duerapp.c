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
 * File: duerapp.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Duer Application Main.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "lightduer_connagent.h"
#include "lightduer_voice.h"
#include "lightduer_dcs.h"
#include "lightduer_timestamp.h"
#include "duerapp_recorder.h"
#include "duerapp_media.h"
#include "duerapp_event.h"
#include "duerapp_alert.h"
#include "duerapp.h"
#include "lightduer_system_info.h"

const duer_system_static_info_t g_system_static_info = {
    .os_version         = "Ubuntu 16.04.4",
    .sw_version         = "dcs3.0-linux-demo",
    .brand              = "Baidu",
    .hardware_version   = "1.0.0",
    .equipment_type     = "PC",
    .ram_size           = 4096 * 1024,
    .rom_size           = 2048 * 1024,
};

static void duer_test_start(const char *profile);

static bool s_started = false;
static char *s_pro_path = NULL;
static int s_reconnect_time = 1;

static duer_status_t duer_app_test_control_point(duer_context ctx, duer_msg_t* msg, duer_addr_t* addr)
{
    DUER_LOGI("duer_app_test_control_point");

    if (msg) {
        duer_response(msg, DUER_MSG_RSP_INVALID, NULL, 0);
    } else {
        return DUER_ERR_FAILED;
    }
    printf("token:0x");
    for (int i = 0; i < msg->token_len; ++i) {
        printf("%x", msg->token[i]);
    }
    printf("\n");
    duer_u8_t *cachedToken = malloc(msg->token_len);
    memcpy(cachedToken, msg->token, msg->token_len);
    printf("token:0x");
    for (int i = 0; i < msg->token_len; ++i) {
        printf("%x", cachedToken[i]);
    }
    printf("\n");

    baidu_json *payload = baidu_json_CreateObject();
    baidu_json_AddStringToObject(payload, "result", "OK");
    baidu_json_AddNumberToObject(payload, "timestamp", duer_timestamp());

    duer_seperate_response(cachedToken, msg->token_len, DUER_MSG_RSP_CONTENT, payload);

    if (payload) {
        baidu_json_Delete(payload);
    }
    free(cachedToken);
    return DUER_OK;
}

static void duer_app_test_control_point_init()
{
    duer_res_t res[] = {
        {
            DUER_RES_MODE_DYNAMIC,
            DUER_RES_OP_PUT,
            "duer_app_test_cp",
            .res.f_res = duer_app_test_control_point
        },

    };

    duer_add_resources(res, sizeof(res) / sizeof(res[0]));
}

static void duer_app_dcs_init()
{
    duer_dcs_framework_init();
    duer_dcs_voice_input_init();
    duer_dcs_voice_output_init();
    duer_dcs_speaker_control_init();
    duer_dcs_audio_player_init();
    duer_dcs_screen_init();
    if(NULL != duer_get_alert_ring()) {
        duer_alert_init();
    }
    duer_dcs_sync_state();
}

static void duer_reconnect_thread()
{
    pthread_detach(pthread_self());
    if (s_pro_path) {
        if (s_reconnect_time >= 16) {
            s_reconnect_time = 1;
        }
        sleep(s_reconnect_time);
        s_reconnect_time *= 2;
        DUER_LOGI("Start reconnect baidu cloud!");
        duer_test_start(s_pro_path);
    } else {
        DUER_LOGI("No profile path, can not be reconnected");
    }
}

static void duer_event_hook(duer_event_t *event)
{
    if (!event) {
        DUER_LOGE("NULL event!!!");
    }

    DUER_LOGD("event: %d", event->_event);
    switch (event->_event) {
        case DUER_EVENT_STARTED:
            {
                duer_app_dcs_init();
                duer_app_test_control_point_init();
                s_reconnect_time = 1;
                s_started = true;
            }
            break;
        case DUER_EVENT_STOPPED: {
                s_started = false;
                if (s_pro_path) {
                    pthread_t reconnect_threadid;
                    int ret = pthread_create(&reconnect_threadid, NULL,
                                             (void *)duer_reconnect_thread, NULL);
                    if(ret != 0)
                    {
                        DUER_LOGE("Create reconnect pthread error!");
                    } else {
                        pthread_setname_np(reconnect_threadid, "reconnect");
                    }
                }
            }
            break;
        default:
            break;
    }
}

void duer_test_start(const char *profile)
{
    const char *data = duer_load_profile(profile);
    if (data == NULL) {
        DUER_LOGE("load profile failed!");
        return;
    }

    DUER_LOGD("profile: \n%s", data);

    // We start the duer with duer_start(), when network connected.
    duer_start(data, strlen(data));

    free((void *)data);
}

static void duer_args_usage(char *param) {
    fprintf(stderr, "Usage: %s [options]\n\n", param);
    fprintf(stderr,
    "Optins:\n"
    "-p  the profile which will be used\n"
    "-r  the alarm bell file\n"
    "-h  Print this message\n\n"
    );
}

int main(int argc, char *argv[])
{
    // Check input arguments
    int c = 0;
    while((c = getopt(argc, argv, "p:r:")) != -1) {
        switch(c) {
            case 'p':
                s_pro_path = optarg;
                break;
            case 'r':
                duer_set_alert_ring(optarg);
                break;
            case 'h':
                duer_args_usage(argv[0]);
                exit(EXIT_SUCCESS);
                break;
        }
    }

    if(NULL == s_pro_path) {
        duer_args_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    // init CA
    duer_initialize();

    // Set the Duer Event Callback
    duer_set_event_callback(duer_event_hook);

    // init media
    duer_media_init();

    // try conntect baidu cloud
    duer_test_start(s_pro_path);

    duer_event_loop();

    duer_media_destroy();

    s_pro_path = NULL;

    DUER_LOGI("Bye-Bye ^_^");

    return 0;
}

void duer_dcs_speak_handler(const char *url)
{
    if (DUER_VOICE_MODE_DEFAULT != duer_voice_get_mode()
        && RECORDER_START == duer_get_recorder_state()) {
        duer_voice_mode_translate_record();
    }
    duer_media_speak_play(url);
    DUER_LOGI("SPEAK url:%s", url);
}

void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t *audio_info)
{
    DUER_LOGI("Audio paly offset:%d url:%s", audio_info->offset, audio_info->url);
    if(audio_info->offset) {
        duer_media_audio_resume(audio_info->url, audio_info->offset);
    } else {
        duer_media_audio_start(audio_info->url);
    }
}

void duer_dcs_get_speaker_state(int *volume, duer_bool *is_mute)
{
    if (volume) {
        *volume = duer_media_get_volume();
    }
    if (is_mute) {
        *is_mute = duer_media_get_mute();
    }
}

void duer_dcs_volume_set_handler(int volume)
{
    duer_media_set_volume(volume);
}

void duer_dcs_volume_adjust_handler(int volume)
{
    duer_media_volume_change(volume);
}

void duer_dcs_mute_handler(duer_bool is_mute)
{
    duer_media_set_mute(is_mute);
}

void duer_dcs_audio_stop_handler(void)
{
    duer_media_audio_pause();
    DUER_LOGI("Audio stop");
}

void duer_dcs_audio_resume_handler(const duer_dcs_audio_info_t *audio_info)
{
    duer_media_audio_resume(audio_info->url, audio_info->offset);
    DUER_LOGI("Audio resume offset:%d url:%s", audio_info->offset, audio_info->url);
}

void duer_dcs_audio_pause_handler(void)
{
    duer_media_audio_pause();
}

int duer_dcs_audio_get_play_progress(void)
{
    int pos = duer_media_audio_get_position();
    DUER_LOGI("PAUSE offset : %d", pos);
    return pos;
}

bool duer_app_get_connect_state()
{
    return s_started;
}

duer_status_t duer_dcs_render_card_handler(baidu_json *payload)
{
    baidu_json *type = NULL;
    baidu_json *content = NULL;
    duer_status_t ret = DUER_OK;

    do {
        if (!payload) {
            ret = DUER_ERR_FAILED;
            break;
        }

        type = baidu_json_GetObjectItem(payload, "type");
        if (!type) {
            ret = DUER_ERR_FAILED;
            break;
        }

        if (strcmp("TextCard", type->valuestring) == 0) {
            content = baidu_json_GetObjectItem(payload, "content");
            if (!content) {
                ret = DUER_ERR_FAILED;
                break;
            }
            DUER_LOGI("Render card content: %s", content->valuestring);
        }
    } while (0);

    return ret;
}

duer_status_t duer_dcs_input_text_handler(const char *text, const char *type)
{
    DUER_LOGI("ASR result: %s", text);
    return DUER_OK;
}

void duer_dcs_stop_speak_handler(void)
{
    DUER_LOGI("stop speak");
    duer_media_speak_stop();
}
