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
 * File: duerapp_media.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Media module function implementation.
 */

#include <gst/gst.h>

#include "duerapp_media.h"
#include "duerapp_config.h"
#include "lightduer_dcs.h"

#define VOLUME_MAX (10.0)
#define VOLUME_MIX (0.000001)
#define VOLUME_INIT (2.0)

typedef void (*play_handler)(void);

typedef struct _play_info {
    GstElement *pip;
    play_handler func;
} play_info_t;

//1: now playing, 2: ready play, 3: audio paused
static play_info_t *s_pinfo[3] = {NULL, NULL, NULL};
static int s_seek = 0;
static bool s_mute = false;
static pthread_cond_t s_cond;
static pthread_mutex_t s_loack;
static pthread_t s_media_tid;
static bool s_isblock = false;
static bool s_start_up = false;
static GMainLoop *s_loop = NULL;
static double s_vol = VOLUME_INIT;
static duer_speak_state_t s_speak_state = MEDIA_SPEAK_STOP;
static duer_audio_state_t s_audio_state = MEDIA_AUDIO_STOP;

static play_info_t *create_play_info(const char *url, play_handler func)
{
    play_info_t *info = NULL;
    info = (play_info_t *)malloc(sizeof(play_info_t));

    if (info) {
        info->func = func;
        info->pip = gst_element_factory_make("playbin", NULL);
        g_object_set(G_OBJECT(info->pip), "uri", url, NULL);

        if (!info->pip) {
            info->func = NULL;
            free(info);
            info = NULL;
        }
    }
    return info;
}

static void delete_play_info(play_info_t **info)
{
    if (*info) {
        if ((*info)->pip) {
            gst_element_set_state((*info)->pip, GST_STATE_NULL);
            gst_object_unref(GST_OBJECT((*info)->pip));
            (*info)->pip = NULL;
        }

        (*info)->func = NULL;
        free(*info);
        *info = NULL;
    }
}

static play_info_t *pop_ready_play_info()
{
    play_info_t *info = NULL;
    pthread_mutex_lock(&s_loack);
    if (!s_pinfo[1]) {
        s_isblock = true;
        pthread_cond_wait(&s_cond, &s_loack);
        s_isblock = false;
    }
    info = s_pinfo[1];
    s_pinfo[1] = NULL;
    pthread_mutex_unlock(&s_loack);
    return info;
}

static void push_ready_play_info(play_info_t **info)
{
    pthread_mutex_lock(&s_loack);
    if (s_pinfo[1]) {
        delete_play_info(&(s_pinfo[1]));
    }
    s_pinfo[1] = *info;
    *info = NULL;
    if (s_isblock) {
        pthread_cond_signal(&s_cond);
    }
    pthread_mutex_unlock(&s_loack);
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {

        case GST_MESSAGE_EOS:
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
                gchar  *debug = NULL;
                GError *error = NULL;

                gst_message_parse_error(msg, &error, &debug);
                g_free(debug);

                DUER_LOGE("gstreamer play : %s\n", error->message);
                duer_dcs_audio_play_failed(DCS_MEDIA_ERROR_INTERNAL_DEVICE_ERROR,
                    error->message);
                g_error_free(error);

                g_main_loop_quit(loop);
            }
            break;
        default:
            break;
    }
}

static void speak_play()
{
    if (s_mute) {
        g_object_set(G_OBJECT(s_pinfo[0]->pip), "volume", 0.0, NULL);
    } else {
        g_object_set(G_OBJECT(s_pinfo[0]->pip), "volume", s_vol, NULL);
    }
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(s_pinfo[0]->pip));

    guint bus_watch_id = gst_bus_add_watch(bus, bus_call, s_loop);
    gst_object_unref(bus);
    gst_element_set_state(s_pinfo[0]->pip, GST_STATE_PLAYING);
    g_main_loop_run(s_loop);

    delete_play_info(&(s_pinfo[0]));
    if (MEDIA_SPEAK_PLAY == s_speak_state) {
        s_speak_state = MEDIA_SPEAK_STOP;
        duer_dcs_speech_on_finished();
    }
}

static void audio_play()
{
    if (s_pinfo[2]) {
        delete_play_info(&(s_pinfo[2]));
    }
    if (s_mute) {
        g_object_set(G_OBJECT(s_pinfo[0]->pip), "volume", 0.0, NULL);
    } else {
        g_object_set(G_OBJECT(s_pinfo[0]->pip), "volume", s_vol, NULL);
    }
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(s_pinfo[0]->pip));

    guint bus_watch_id = gst_bus_add_watch(bus, bus_call, s_loop);
    gst_object_unref(bus);
    gst_element_set_state(s_pinfo[0]->pip, GST_STATE_PLAYING);
    g_main_loop_run(s_loop);

    if (MEDIA_AUDIO_PLAY == s_audio_state) {
        duer_dcs_audio_on_finished();
        delete_play_info(&(s_pinfo[0]));
        s_seek = 0;
    } else if (MEDIA_AUDIO_STOP == s_audio_state) {
        delete_play_info(&(s_pinfo[0]));
        s_seek = 0;
    } else if (MEDIA_AUDIO_PAUSE == s_audio_state) {
        gst_element_set_state(s_pinfo[0]->pip, GST_STATE_PAUSED);
        s_pinfo[2] = s_pinfo[0];
        s_pinfo[0] = NULL;
    } else {
        // do nothing
    }
}

static void media_thread()
{
    while (s_start_up) {
        s_pinfo[0] = pop_ready_play_info();
        if (!s_start_up) {
            break;
        }
        if (!s_pinfo[0]) {
            continue;
        }
        s_pinfo[0]->func();
    }
}

void duer_media_init()
{
    pthread_mutex_init(&s_loack, NULL);
    pthread_cond_init(&s_cond, NULL);

    gst_init(NULL, NULL);

    s_loop = g_main_loop_new(NULL, FALSE);
    if (!s_loop) {
        DUER_LOGE("Create media loop error!");
        exit(1);
    }

    s_start_up = true;
    int ret = pthread_create(&s_media_tid, NULL, (void *)media_thread, NULL);
    if (ret) {
        DUER_LOGE("Create media pthread error!");
        exit(1);
    } else {
        pthread_setname_np(s_media_tid, "dcs3_demo_media");
    }
}

void duer_media_destroy()
{
    s_start_up = false;
    if(s_isblock) {
        pthread_cond_signal(&s_cond);
    } else {
        g_main_loop_quit(s_loop);
    }

    pthread_join(s_media_tid, NULL);
}

void duer_media_speak_play(const char *url)
{
    if (MEDIA_SPEAK_PLAY != s_speak_state) {
        duer_media_speak_stop();
    }

    play_info_t *speak = create_play_info(url, speak_play);
    if (speak) {
        push_ready_play_info(&speak);
        s_speak_state = MEDIA_SPEAK_PLAY;
    } else {
        DUER_LOGI("Speak info create failed!");
    }
}

void duer_media_speak_stop()
{
    if (MEDIA_SPEAK_PLAY == s_speak_state) {
        s_speak_state = MEDIA_SPEAK_STOP;
        g_main_loop_quit(s_loop);
    } else {
        DUER_LOGI("Speak stop state : %d", s_speak_state);
    }
}

void duer_media_audio_start(const char *url)
{
    if (MEDIA_AUDIO_STOP != s_audio_state) {
        duer_media_audio_stop();
    }

    play_info_t *audio = create_play_info(url, audio_play);
    if (audio) {
        push_ready_play_info(&audio);
        s_audio_state = MEDIA_AUDIO_PLAY;
    } else {
        DUER_LOGI("Audio info create failed!");
    }
}

void duer_media_audio_resume(const char *url, int offset)
{
    if (MEDIA_AUDIO_PAUSE == s_audio_state) {
        if ((offset == s_seek) && (s_pinfo[2])) {
            push_ready_play_info(&(s_pinfo[2]));
            s_audio_state = MEDIA_AUDIO_PLAY;
        } else {
            duer_media_audio_start(url);
        }
    } else {
        duer_media_audio_start(url);
    }
}

void duer_media_audio_stop()
{
    if (MEDIA_AUDIO_PLAY == s_audio_state) {
        s_audio_state = MEDIA_AUDIO_STOP;
        g_main_loop_quit(s_loop);
    } else if (MEDIA_AUDIO_PAUSE == s_audio_state) {
        if (s_pinfo[2]) {
            delete_play_info(&(s_pinfo[2]));
        }
        s_audio_state = MEDIA_AUDIO_STOP;
    } else {
        DUER_LOGI("Audio stop state : %d", s_audio_state);
    }
}

void duer_media_audio_pause()
{
    if (MEDIA_AUDIO_PLAY == s_audio_state) {
        s_audio_state = MEDIA_AUDIO_PAUSE;
        g_main_loop_quit(s_loop);
    } else {
        DUER_LOGI("Audio pause state : %d", s_audio_state);
    }
}

int duer_media_audio_get_position()
{
    gint64 pos = 0;
    if (s_pinfo[0] && s_pinfo[0]->pip && MEDIA_AUDIO_PLAY == s_audio_state) {
        if (gst_element_query_position(s_pinfo[0]->pip, GST_FORMAT_TIME, &pos)) {
            s_seek = pos / GST_MSECOND;
        }
    }

    return s_seek;
}

duer_audio_state_t duer_media_audio_state()
{
    return s_audio_state;
}

void duer_media_volume_change(int volume)
{
    if (s_mute) {
        return;
    }
    s_vol += volume / 10.0;
    if (s_vol < VOLUME_MIX) {
        s_vol = 0.0;
    } else if (s_vol > VOLUME_MAX){
        s_vol = VOLUME_MAX;
    } else {
        // do nothing
    }

    if (s_pinfo[0] && s_pinfo[0]->pip) {
        g_object_set(G_OBJECT(s_pinfo[0]->pip), "volume", s_vol, NULL);
    }
    DUER_LOGI("volume : %.1f", s_vol);
    if (DUER_OK == duer_dcs_on_volume_changed()) {
        DUER_LOGI("volume change OK");
    }
}

void duer_media_set_volume(int volume)
{
    if (s_mute) {
        return;
    }
    s_vol = volume / 10.0;
    if (s_pinfo[0] && s_pinfo[0]->pip) {
        g_object_set(G_OBJECT(s_pinfo[0]->pip), "volume", s_vol, NULL);
    }
    DUER_LOGI("volume : %.1f", s_vol);
    if (DUER_OK == duer_dcs_on_volume_changed()) {
        DUER_LOGI("volume change OK");
    }
}

int duer_media_get_volume()
{
    return (int)(s_vol * 10);
}

void duer_media_set_mute(bool mute)
{
    s_mute = mute;

    if (s_pinfo[0] && s_pinfo[0]->pip) {
        if (mute) {
            g_object_set(G_OBJECT(s_pinfo[0]->pip), "volume", 0.0, NULL);
        } else {
            g_object_set(G_OBJECT(s_pinfo[0]->pip), "volume", s_vol, NULL);
        }
    }
    duer_dcs_on_mute();
}

bool duer_media_get_mute()
{
    return s_mute;
}
