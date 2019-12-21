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
 * File: duerapp_recorder.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Record module function implementation.
 */

#include <semaphore.h>

#include "duerapp_recorder.h"
#include "duerapp_config.h"
#include "lightduer_voice.h"

#define ALSA_PCM_NEW_HW_PARAMS_API
#define SAMPLE_RATE (16000)
#define FRAMES_INIT (16)
#define FRAMES_SIZE (2) // bytes / sample * channels
#define CHANNEL (1)

static duer_rec_state_t s_duer_rec_state = RECORDER_STOP;
static pthread_t s_rec_threadID;
static sem_t s_rec_sem;
static duer_rec_config_t *s_index = NULL;
static bool s_is_suspend = false;

static void recorder_thread()
{
    pthread_detach(pthread_self());
    duer_voice_start(16000);
    snd_pcm_hw_params_get_period_size(s_index->params, &(s_index->frames), &(s_index->dir));

    if (s_index->frames < 0) {
        DUER_LOGE("Get period size failed!");
        return;
    }
    s_index->size = s_index->frames * FRAMES_SIZE;
    static char *buffer = NULL;
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
    buffer = (char *)malloc(s_index->size);
    if (!buffer) {
        DUER_LOGE("malloc buffer failed!\n");
    } else {
        memset(buffer, 0, s_index->size);
    }
    while (RECORDER_START == s_duer_rec_state)
    {
        int ret = snd_pcm_readi(s_index->handle, buffer, s_index->frames);

        if (ret == -EPIPE) {
            DUER_LOGE("an overrun occurred!");
            snd_pcm_prepare(s_index->handle);
        } else if (ret < 0) {
            DUER_LOGE("read: %s", snd_strerror(ret));
        } else if (ret != (int)s_index->frames) {
            DUER_LOGE("read %d frames!", ret);
        } else {
            // do nothing
        }
        duer_voice_send(buffer, s_index->size);
    }
    duer_voice_stop();
    if(s_is_suspend) {
        duer_voice_terminate();
        s_is_suspend = false;
    }
    if (buffer) {
        free(buffer);
        buffer = NULL;
    }
    snd_pcm_drain(s_index->handle);
    snd_pcm_close(s_index->handle);
    if(s_index) {
        free(s_index);
        s_index = NULL;
    }
    if(sem_post(&s_rec_sem)) {
        DUER_LOGE("sem_post failed.");
    }
}

static int duer_open_alsa_pcm()
{
    int ret = DUER_OK;
    int result = (snd_pcm_open(&(s_index->handle), "default", SND_PCM_STREAM_CAPTURE, 0));
    if (result < 0)
    {
        DUER_LOGE("unable to open pcm device: %s", snd_strerror(ret));
        ret = DUER_ERR_FAILED;
    }
    return ret;
}

static int duer_set_pcm_params()
{
    int ret = DUER_OK;
    snd_pcm_hw_params_alloca(&(s_index->params));
    snd_pcm_hw_params_any(s_index->handle, s_index->params);
    snd_pcm_hw_params_set_access(s_index->handle, s_index->params,
                                 SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(s_index->handle, s_index->params,
                                 SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(s_index->handle, s_index->params,
                                   CHANNEL);
    snd_pcm_hw_params_set_rate_near(s_index->handle, s_index->params,
                                    &(s_index->val), &(s_index->dir));
    snd_pcm_hw_params_set_period_size_near(s_index->handle, s_index->params,
                                           &(s_index->frames), &(s_index->dir));

    int result = snd_pcm_hw_params(s_index->handle, s_index->params);
    if (result < 0)    {
        DUER_LOGE("unable to set hw parameters: %s", snd_strerror(ret));
        ret = DUER_ERR_FAILED;
    }
    return ret;
}

int duer_recorder_start()
{
    static bool is_first_time = true;
    if(is_first_time) {
        if(sem_init(&s_rec_sem, 0, 1)) {
            DUER_LOGE("Init s_rec_sem failed.");
            exit(1);
        }
        is_first_time = false;
    }
    do {
        if(sem_wait(&s_rec_sem)) {
            DUER_LOGE("sem_wait failed.");
            break;
        }
        if (RECORDER_STOP == s_duer_rec_state) {
            s_index = (duer_rec_config_t *)malloc(sizeof(duer_rec_config_t));
            if (!s_index) {
                break;
            }
            memset(s_index, 0, sizeof(duer_rec_config_t));
            s_index->frames = FRAMES_INIT;
            s_index->val = SAMPLE_RATE; // pcm sample rate
            int ret = duer_open_alsa_pcm();
            if (ret != DUER_OK) {
                DUER_LOGE("open pcm failed");
                break;
            }

            ret = duer_set_pcm_params();
            if (ret != DUER_OK) {
                DUER_LOGE("open pcm failed");
                break;
            }

            ret = pthread_create(&s_rec_threadID, NULL, (void *)recorder_thread, NULL);
            if(ret != 0)
            {
                DUER_LOGE("Create recorder pthread error!");
                break;
            } else {
                pthread_setname_np(s_rec_threadID, "recorder");
            }
            s_duer_rec_state = RECORDER_START;
        }
        else
        {
            DUER_LOGI("Recorder Start failed! state:%d", s_duer_rec_state);
        }
        return DUER_OK;
    } while(0);
    if(s_index) {
        free(s_index);
        s_index = NULL;
    }
    return DUER_ERR_FAILED;
}

int duer_recorder_stop()
{
    int ret = DUER_OK;
    if (RECORDER_START == s_duer_rec_state) {
        s_duer_rec_state = RECORDER_STOP;
    } else {
        ret = DUER_ERR_FAILED;
        DUER_LOGI("Recorder Stop failed! state:%d", s_duer_rec_state);
    }
    return ret;
}

int duer_recorder_suspend()
{
    int ret = duer_recorder_stop();
    if (DUER_OK == ret) {
        s_is_suspend = true;
    } else {
        DUER_LOGI("Recorder Stop failed! state:%d", s_duer_rec_state);
    }
    return ret;
}

duer_rec_state_t duer_get_recorder_state()
{
    return s_duer_rec_state;
}
