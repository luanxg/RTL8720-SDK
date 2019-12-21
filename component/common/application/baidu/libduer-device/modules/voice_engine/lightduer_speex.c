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
 * File: lightduer_speex.c
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Light duer Speex encoder.
 */

#include "lightduer_speex.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_memory.h"
#include <speex/speex.h>

#include "lightduer_ds_log_recorder.h"
#include "lightduer_timestamp.h"

#define DUER_OUTPUT_HEADER_MAGIC_1     'P'
#define DUER_OUTPUT_HEADER_MAGIC_2     'X'

typedef enum _duer_sample_rate_e {
    DUER_SR_8000,
    DUER_SR_16000,
    DUER_SR_TOTAL,
} duer_sample_rate_t;

typedef struct _duer_speex_s {
    duer_sample_rate_t  _rate;
    void *              _encoder;

    size_t              _frame_size;
    size_t              _frame_cur;

    short *             _buffer_in;
    char *              _buffer_out;

    SpeexBits           _bits;
} duer_speex_t;

static const char DUER_OUTPUT_HEADER_MAGIC_0[DUER_SR_TOTAL] = {
    'S',    //< DUER_SR_8000
    'T',    //< DUER_SR_16000
};

duer_speex_handler duer_speex_create(int rate, int quality)
{
    int rs = -1;
    duer_speex_t *speex = NULL;

    do {
        speex = (duer_speex_t *)DUER_MALLOC(sizeof(duer_speex_t)); // ~= 60
        if (speex == NULL) {
            DUER_LOGE("Memory overflow!!! Alloc speex handler failed.");
            break;
        }

        DUER_MEMSET(speex, 0, sizeof(duer_speex_t));

        if (rate == 8000) {
            speex->_rate = DUER_SR_8000;
        } else if (rate == 16000) {
            speex->_rate = DUER_SR_16000;
        } else {
            DUER_LOGE("Not supported sample rate: %d", rate);
            break;
        }

        speex->_encoder = speex_encoder_init(speex_mode_list[speex->_rate]);
        if (speex->_encoder == NULL) {
            DUER_LOGE("Speex encoder state init failed!");
            break;
        }

        speex_encoder_ctl(speex->_encoder, SPEEX_SET_QUALITY, &quality);

        speex_encoder_ctl(speex->_encoder, SPEEX_GET_FRAME_SIZE, &speex->_frame_size);
        DUER_LOGV("Speex frame size: %d", speex->_frame_size);

        speex->_buffer_out = DUER_MALLOC(speex->_frame_size + sizeof(int)); // ~= 324
        if (speex->_buffer_out == NULL) {
            DUER_LOGE("Memory overflow!!! Alloc speex buffer out failed.");
            break;
        }

        speex->_buffer_in = DUER_MALLOC(sizeof(short) * speex->_frame_size); // ~= 640
        if (speex->_buffer_in == NULL) {
            DUER_LOGE("Memory overflow!!! Alloc speex buffer in failed.");
            break;
        }

        speex_bits_init(&speex->_bits);

        rs = 0;
    } while (0);

    if (rs < 0) {
        duer_speex_destroy(speex);
        speex = NULL;
    }

    return speex;
}

static void duer_speex_encode_frame(duer_speex_t *speex, duer_encoded_func func)
{
    duer_u32_t timestamp = duer_timestamp();

    speex_bits_reset(&speex->_bits);

    /*Encode the frame*/
    speex_encode_int(speex->_encoder, speex->_buffer_in, &speex->_bits);

    /*Copy the bits to an array of char that can be written*/
    int written = speex_bits_write(&speex->_bits, speex->_buffer_out + sizeof(int), speex->_frame_size);
    DUER_LOGV("Speex encoded written: %d", written);
    speex->_buffer_out[0] = written & 0xFF;
    speex->_buffer_out[1] = DUER_OUTPUT_HEADER_MAGIC_0[speex->_rate];
    speex->_buffer_out[2] = DUER_OUTPUT_HEADER_MAGIC_1;
    speex->_buffer_out[3] = DUER_OUTPUT_HEADER_MAGIC_2;

    if (func) {
        func(speex->_buffer_out, sizeof(int) + written);
    }

    duer_ds_rec_compress_info_update(timestamp, duer_timestamp());
}

void duer_speex_encode(duer_speex_handler handler, const void *data, size_t size, duer_encoded_func func)
{
    duer_speex_t *speex = (duer_speex_t *)handler;
    size_t cur = 0;
    DUER_LOGD("duer_speex_encode, speex:%p", speex);
    if (speex && speex->_encoder && speex->_buffer_in && speex->_buffer_out) {
        size_t framesize = speex->_frame_size * sizeof(short);
        char *buffer_in = (char *)speex->_buffer_in;

        DUER_LOGD("duer_speex_encode: size = %d, framesize = %d", size, framesize);

        while (cur + framesize - speex->_frame_cur <= size) {
            DUER_LOGV("duer_speex_encode: cur = %d, speex->_frame_cur = %d", cur, speex->_frame_cur);
            DUER_MEMCPY(buffer_in + speex->_frame_cur, (const char *)data + cur, framesize - speex->_frame_cur);
            cur += framesize - speex->_frame_cur;
            speex->_frame_cur = 0;
            duer_speex_encode_frame(speex, func);
        }

        DUER_LOGV("duer_speex_encode: cur = %d, speex->_frame_cur = %d", cur, speex->_frame_cur);

        if (speex->_frame_cur > 0 && (data == NULL || size == 0)) {
            DUER_MEMSET(buffer_in + speex->_frame_cur, 0, framesize - speex->_frame_cur);
            duer_speex_encode_frame(speex, func);
        } else if (cur < size) {
            DUER_MEMCPY(buffer_in + speex->_frame_cur, (const char *)data + cur, size - cur);
            speex->_frame_cur += size - cur;
        }

        DUER_LOGV("duer_speex_encode: speex->_frame_cur = %d", speex->_frame_cur);
    }
}

void duer_speex_destroy(duer_speex_handler handler)
{
    duer_speex_t *speex = (duer_speex_t *)handler;

    if (speex) {
        if (speex->_encoder) {
            speex_encoder_destroy(speex->_encoder);
            speex->_encoder = NULL;
        }

        if (speex->_buffer_out) {
            DUER_FREE(speex->_buffer_out);
            speex->_buffer_out = NULL;
        }

        if (speex->_buffer_in) {
            DUER_FREE(speex->_buffer_in);
            speex->_buffer_in = NULL;
        }

        speex_bits_destroy(&speex->_bits);

        DUER_FREE(speex);
    }
}
