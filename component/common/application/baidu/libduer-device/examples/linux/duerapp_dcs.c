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
 * File: duerapp_dcs.c
 * Auth: Gang Chen(chengang12@baidu.com)
 * Desc: Implement DCS related APIs.
 */

#include "lightduer_log.h"
#include "lightduer_dcs.h"
#include "lightduer_http_client_ops.h"
#include "lightduer_http_client.h"
#include "lightduer_lib.h"
#include "lightduer_memory.h"

#include <unistd.h>
#include <pthread.h>

static int s_speak_id = 0;
static int s_audio_id = 0;
static int s_vol = 20;
static duer_bool s_mute = DUER_FALSE;
static duer_bool s_pause = DUER_FALSE;

void duer_dcs_listen_handler(void)
{
    DUER_LOGI("listen");
}

void duer_dcs_stop_listen_handler(void)
{
    DUER_LOGI("Stop listen");
}

static int download_callback(void *user_ctx, duer_http_data_pos_t pos,
                const char *buf, size_t len, const char *type) {
    DUER_LOGI("download_callback size:%d", len);
    return DUER_OK;
}

static void speak_thread(void *arg)
{
    pthread_detach(pthread_self());
    int id = s_speak_id;

    // HTTP
    char *url = (char*)arg;
// now http downloader only support 32bit env,
// so before enable the http downloader below,
// make sure compile in 32bit mode
#if 0
    duer_http_client_t *http_client = duer_create_http_client();
    duer_http_reg_data_hdlr(http_client, download_callback, NULL);
    //duer_http_reg_stop_notify_cb(data->client, check_stop_download);

    int position = 0;
    int connect_keep_time = 0;
    DUER_LOGI("url:%s", url);
    int ret = duer_http_get(http_client, url, position, connect_keep_time);

    duer_http_destroy(http_client);
#else
    sleep(3);
#endif
    if (url) {
        DUER_FREE(url);
        url = arg = NULL;
    }

    if (id == s_speak_id) {
        DUER_LOGI("duer_dcs_speech_on_finished");
        duer_dcs_speech_on_finished();
    }
}

void duer_dcs_speak_handler(const char *url)
{
    DUER_LOGI("Speak url: %s", url);
    int length = DUER_STRLEN(url) + 1;
    char *new_url = (char*)DUER_MALLOC(length);
    DUER_MEMSET(new_url, 0, length);
    DUER_MEMCPY(new_url, url, length);
    pthread_t speak_tid;
    s_audio_id++;
    s_speak_id++;
    s_pause = DUER_FALSE;
    int ret = pthread_create(&speak_tid, NULL, (void *)speak_thread, (void*)new_url);
    if (ret) {
        DUER_LOGE("Create speak pthread error!");
        exit(1);
    } else {
        pthread_setname_np(speak_tid, "linux_demo_speak");
    }
}

void duer_dcs_stop_speak_handler(void)
{
    DUER_LOGI("speak stop");
    s_speak_id++;
}

static void audio_thread()
{
    pthread_detach(pthread_self());
    int id = s_audio_id;
    if (s_audio_id % 50 == 0) {
        DUER_LOGI("duer_dcs_audio_play_failed");
        duer_dcs_audio_play_failed(DCS_MEDIA_ERROR_UNKNOWN, "DUEROS_QA_TEST");
        return;
    }
    if (!s_pause){
        sleep(10);
        if (id == s_audio_id) {
            DUER_LOGI("duer_dcs_audio_on_stuttered:true");
            duer_dcs_audio_on_stuttered(DUER_TRUE);
        } else {
            return;
        }
        sleep(2);
        if (id == s_audio_id) {
            DUER_LOGI("duer_dcs_audio_on_stuttered:false");
            duer_dcs_audio_on_stuttered(DUER_FALSE);
        } else {
            return;
        }
        sleep(10);
        if (id == s_audio_id) {
            DUER_LOGI("DCS_PAUSE_CMD");
            duer_dcs_send_play_control_cmd(DCS_PAUSE_CMD);
        } else {
            return;
        }
        sleep(10);
        if (id == s_audio_id) {
            DUER_LOGI("DCS_PLAY_CMD");
            s_pause = DUER_TRUE;
            duer_dcs_send_play_control_cmd(DCS_PLAY_CMD);
            return;
        } else {
            return;
        }
    } else {
        s_pause = DUER_FALSE;
        sleep(10);
        if (id == s_audio_id) {
            DUER_LOGI("duer_dcs_audio_on_finished");
            duer_dcs_audio_on_finished();
        } else {
            return;
        }
    }
}

void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t *audio_info)
{
    DUER_LOGI("Audio url: %s", audio_info->url);
    pthread_t audio_tid;
    s_audio_id++;
    int ret = pthread_create(&audio_tid, NULL, (void *)audio_thread, NULL);
    if (ret) {
        DUER_LOGE("Create audio pthread error!");
        exit(1);
    } else {
        pthread_setname_np(audio_tid, "linux_demo_audio");
    }
}

void duer_dcs_get_speaker_state(int *volume, duer_bool *is_mute)
{
    *volume = s_vol;
    *is_mute = s_mute;
}

void duer_dcs_volume_set_handler(int volume)
{
    s_vol = volume;
    duer_dcs_on_volume_changed();
}

void duer_dcs_volume_adjust_handler(int volume)
{
    s_vol += volume;
    duer_dcs_on_volume_changed();
}

void duer_dcs_mute_handler(duer_bool is_mute)
{
    s_mute = is_mute;
    duer_dcs_on_mute();
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
/*
duer_status_t duer_dcs_input_text_handler(const char *text, const char *type)
{
    DUER_LOGI("ASR result: %s", text);
    FILE *fp = NULL;
    if(strcmp(type, "FINAL") == 0)
    {
        if((fp = fopen("asr_result.txt", "a")) == NULL)
        {
            printf("不能打开文件\n");
            return DUER_OK;
        }
        fwrite(text, 1, strlen(text), fp);
        fclose(fp);
    }
    return DUER_OK;
}*/

int duer_dcs_audio_get_play_progress(void)
{
    static int offset = 0;
    offset = (offset + 15) % 100;
    return offset;
}

void duer_dcs_audio_pause_handler(void)
{
    DUER_LOGI("audio pause");
    s_audio_id++;
}

void duer_dcs_audio_resume_handler(const duer_dcs_audio_info_t *audio_info)
{
    DUER_LOGI("audio resume url: %s", audio_info->url);
    duer_dcs_audio_on_stopped();
}

void duer_dcs_audio_stop_handler(void)
{
    DUER_LOGI("audio stop");
}

