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
 * File: lightduer_store_voice.cpp
 * Auth: Zhang Leliang (zhangleliang@baidu.com)
 * Desc: impl the voice/speex store interface
 */

#ifdef ENABLE_DUER_STORE_VOICE

#include <stdio.h>
#include "lightduer_log.h"
#include "lightduer_store_voice.h"

#define SAMPLE_RATE_16K // sample rate is 16k, undefine will use 8k

#ifndef SDCARD_PATH
#ifdef __MBED__
#define SDCARD_PATH     "sd"  // refer to sdpath initialize g_sd
#elif defined ESP_PLATFORM
#define SDCARD_PATH     "sdcard"
#else
#error "store voice function could not be implemented in current platform"
#endif
#endif

typedef struct _pcm_header_t {
    char    pcm_header[4];
    size_t  pcm_length;
    char    format[8];
    int     bit_rate;
    short   pcm;
    short   channel;
    int     sample_rate;
    int     byte_rate;
    short   block_align;
    short   bits_per_sample;
    char    fix_data[4];
    size_t  data_length;
} pcm_header_t;

static pcm_header_t s_pcm_header = {
    {'R', 'I', 'F', 'F'},
    (size_t)-1,
    {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '},
    0x10,
    0x01,
    0x01,
#ifdef SAMPLE_RATE_16K
    0x3E80,
    0x7D00,
#else //8k
    0x1F40,
    0x3E80,
#endif // SAMPLE_RATE_16K
    0x02,
    0x10,
    {'d', 'a', 't', 'a'},
    (size_t)-1
};

static FILE *s_voice_file;
static FILE *s_speex_file;

int duer_store_voice_start(int name_id)
{
    DUER_LOGD("start");
    if (s_voice_file) {
        fclose(s_voice_file);
        s_voice_file = NULL;
    }
    if (s_speex_file) {
        fclose(s_speex_file);
        s_speex_file = NULL;
    }

    char _name[64];
    snprintf(_name, sizeof(_name), "/"SDCARD_PATH"/%08d.wav", name_id);
    s_voice_file = fopen(_name, "wb");
    if (!s_voice_file) {
        DUER_LOGE("can't open file %s", _name);
        return -1;
    } else {
        DUER_LOGD("begin write to file:%s", _name);
    }
    fwrite(&s_pcm_header, 1, sizeof(s_pcm_header), s_voice_file);
    s_pcm_header.data_length = 0;
    s_pcm_header.pcm_length = sizeof(s_pcm_header) - 8;

    snprintf(_name, sizeof(_name), "/"SDCARD_PATH"/%08d.speex", name_id);
    s_speex_file = fopen(_name, "wb");
    if (!s_speex_file) {
        DUER_LOGE("can't open file %s", _name);
        return -1;
    } else {
        DUER_LOGD("begin write to file:%s", _name);
    }

    return 0;
}

int duer_store_voice_write(const void *data, uint32_t size)
{
    DUER_LOGD("data");
    if (s_voice_file) {
        fwrite(data, 1, size, s_voice_file);
        s_pcm_header.data_length += size;
    }
    return 0;
}

int duer_store_speex_write(const void *data, uint32_t size)
{
    DUER_LOGD("data");
    if (s_speex_file) {
        fwrite(data, 1, size, s_speex_file);
    }
    return 0;
}

int duer_store_voice_end()
{
    if (s_voice_file) {
        s_pcm_header.pcm_length += s_pcm_header.data_length;
        fseek(s_voice_file, 0, SEEK_SET);
        fwrite(&s_pcm_header, 1, sizeof(s_pcm_header), s_voice_file);
        fclose(s_voice_file);
        s_voice_file = NULL;
        DUER_LOGD("finish the voice record");
    }
    if (s_speex_file) {
        fclose(s_speex_file);
        s_speex_file = NULL;
        DUER_LOGD("finish the speex record");
    }
    return 0;
}

#endif // ENABLE_DUER_STORE_VOICE
