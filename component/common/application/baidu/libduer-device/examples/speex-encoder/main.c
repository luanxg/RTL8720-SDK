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
// File: main.c
// Auth: Su Hao (suhao@baidu.com)
//
// Desc: Encode the WAV to speex.

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>

#include "lightduer_types.h"
#include "lightduer_speex.h"

#define ERROR(...)  fprintf(stderr, "/E %s(%d): ", __FILE__, __LINE__), fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#define DEBUG(...)  //fprintf(stdout, "/D %s(%d): ", __FILE__, __LINE__), fprintf(stdout, __VA_ARGS__), fprintf(stdout, "\n")

#define PRINT(...)  fprintf(stdout, __VA_ARGS__)

#define SUFFIX      ".spx"

typedef enum _duer_error_code_enum {
    DUER_NO_ERROR,
    DUER_ERR_ON_INPUT = -1,
    DUER_ERR_INVALID_ARGUMENT = -2,
    DUER_ERR_INVALID_FILE = -3,
    DUER_ERR_CANNOT_OPEN_FILE = -4,
    DUER_ERR_INVALID_PCM_FILE = -5,
    DUER_ERR_INVALID_PARAM = -6,
    DUER_ERR_MEMORY_OVERFLOW = -7,
} duer_errcode_e;

typedef struct _duer_wav_header_s {

    // Main Chunk
    char    _chunk_id[4];       // "RIFF"标记
    int     _chunk_size;        // 文件大小
    char    _format[4];         // "WAVE"标记

    // Sub Chunk
    char    _subchunk_id[4];    // "fmt "标记
    int     _subchunk_size;     // 预留
    short   _audio_format;      // 音频格式，10H为PCM数据
    short   _num_channels;      // 声道数
    int     _sample_rate;       // 采样率
    int     _byte_rate;         // 波形音频数据传送速率
    short   _block_align;       // 数据块的调整数
    short   _bits_per_sample;   // 每个样本占用的数据位大小

} duer_wavhdr_t;

typedef struct _duer_wav_chunk_s {

    char    _id[4];             // 标记
    int     _size;              // 大小

} duer_wavchk_t;

static FILE *g_file_out = NULL;

void usage(const char *command) {
    if (command == NULL) {
        command = "speexenc";
    }

    PRINT("%s INPUT [OUTPUT]\n\n", basename((char *)command));
}

void *bcamem_alloc(duer_context ctx, duer_size_t size)
{
    return malloc(size);
}

void *bcamem_realloc(duer_context ctx, void *ptr, duer_size_t size)
{
    return realloc(ptr, size);
}

void bcamem_free(duer_context ctx, void *ptr)
{
    free(ptr);
}

int wav_check_header(const duer_wavhdr_t *header) {
    int rs = DUER_ERR_INVALID_PCM_FILE;

    if (header == NULL) {
        ERROR("Invalid parameter!!!");
        rs = DUER_ERR_INVALID_PARAM;
        goto exit;
    }

    if (strncmp("RIFF", header->_chunk_id, 4) != 0) {
        ERROR("No RIFF flag!!!");
        goto exit;
    }

    if (header->_chunk_size <= 0) {
        ERROR("Invalid PCM file size: %d", header->_chunk_size);
        goto exit;
    }

    if (strncmp("WAVE", header->_format, 4) != 0) {
        ERROR("No WAVE flag!!!");
        goto exit;
    }

    if (strncmp("fmt ", header->_subchunk_id, 4) != 0) {
        ERROR("No 'fmt ' flag!!!");
        goto exit;
    }

    if (header->_audio_format != 0x01) {
        ERROR("The WAV is not PCM!!! _audio_format = %d", header->_audio_format);
        goto exit;
    }

    if (header->_num_channels != 0x01) {
        ERROR("Support SINGLE CHANNEL only!!! _num_channels = %d", header->_num_channels);
        goto exit;
    }

    if (header->_sample_rate != 8000 && header->_sample_rate != 16000) {
        ERROR("Support SAMPLE RATE: 8000 & 16000!!! _sample_rate = %d", header->_sample_rate);
        goto exit;
    }

    if (header->_bits_per_sample != (header->_sample_rate / 1000)) {
        ERROR("Support BITS/SAMPLE: <%d> with SAMPLE RATE: <%d>!!! _bits_per_sample = %d", header->_sample_rate / 1000, header->_sample_rate, header->_bits_per_sample);
        goto exit;
    }

    rs = DUER_NO_ERROR;

exit:

    return rs;
}

int wav_read_chunk(FILE *file, duer_wavchk_t *chunk) {

    int rs = DUER_ERR_INVALID_PARAM;

    if (file == NULL || chunk == NULL) {
        ERROR("Invalid parameters!!!");
        return rs;
    }

    do {
        rs = fread(chunk->_id, 1, sizeof(chunk->_id), file);
        if (rs != sizeof(chunk->_id)) {
            ERROR("Can't read the Chunk tag: %d", rs);
            rs = DUER_ERR_INVALID_PCM_FILE;
            break;
        }

        rs = fread(&(chunk->_size), 1, sizeof(chunk->_size), file);
        if (rs != sizeof(chunk->_size)) {
            ERROR("Can't read the Chunk size: %d", rs);
            rs = DUER_ERR_INVALID_PCM_FILE;
            break;
        }

        if (strncmp("data", chunk->_id, 4) == 0) {
            rs = DUER_NO_ERROR;
            break;
        }

        rs = fseek(file, chunk->_size, SEEK_CUR);
        if (rs != 0) {
            ERROR("Can't seek to <%d> from current!!!", rs);
            rs = DUER_ERR_INVALID_PCM_FILE;
            break;
        }
    } while(1);

    return rs;
}

static void speex_encode_callback(const void *data, size_t size)
{
    if (g_file_out) {
        int rs = fwrite(data, 1, size, g_file_out);
        DEBUG("The write size = %d", rs);
    }
}

int pcm2spx(const char *pcm, const char *spx) {
    int rs = DUER_NO_ERROR;

    FILE *f_in = NULL;
    FILE *f_out = NULL;
    duer_wavhdr_t header;
    duer_wavchk_t chunk;
    int frame_size = 0;
    char frame[1024];
    duer_speex_handler speex = NULL;
    const int SPEEX_QUALITY = 5;

    f_in = fopen(pcm, "rb");
    if (f_in == NULL) {
        ERROR("Can't open the PCM file: %s", pcm);
        rs = DUER_ERR_CANNOT_OPEN_FILE;
        goto exit;
    }

    f_out = fopen(spx, "wb");
    if (f_out == NULL) {
        ERROR("Can't open the SPEEX file: %s", spx);
        rs = DUER_ERR_CANNOT_OPEN_FILE;
        goto exit;
    }
    g_file_out = f_out;

    rs = fread(&header, 1, sizeof(header), f_in);
    if (rs != sizeof(header)) {
        ERROR("Invalid PCM file: %s", pcm);
        rs = DUER_ERR_INVALID_PCM_FILE;
        goto exit;
    }

    DEBUG("The header size = %u", rs);

    rs = wav_check_header(&header);
    if (rs < DUER_NO_ERROR) {
        goto exit;
    }

    rs = wav_read_chunk(f_in, &chunk);
    if (rs < DUER_NO_ERROR) {
        goto exit;
    }

    speex = duer_speex_create(header._sample_rate, SPEEX_QUALITY);
    if (speex == NULL) {
        ERROR("Create speex failed!!! rate = %d", header._sample_rate);
        goto exit;
    }

    frame_size = header._sample_rate * 20 / 1000 * header._bits_per_sample / 8;
    DEBUG("The frame size = %u", frame_size);
    do {
        rs = fread(frame, 1, frame_size, f_in);
        DEBUG("The read size = %u", rs);

        if (rs > 0) {
            duer_speex_encode(speex, frame, rs, speex_encode_callback);
        }
    } while (rs > 0);

    duer_speex_destroy(speex);

exit:

    g_file_out = NULL;

    if (f_in) {
        fclose(f_in);
    }

    if (f_out) {
        fclose(f_out);
    }

    return rs;
}

int main(int args, char *argv[]) {
    int rs = 0;

    const char *pcm = NULL;
    const char *spx = NULL;
    const char *suffix = SUFFIX;
    char *temp = NULL;
    int i;

    baidu_ca_memory_init(
        NULL,
        bcamem_alloc,
        bcamem_realloc,
        bcamem_free);

    if (args < 2) {
        ERROR("You need input a PCM file!!!");
        rs = DUER_ERR_ON_INPUT;
        goto exit;
    }

    i = 1;
    while (i < args) {
        DEBUG("%d ==> %s", i, argv[i]);
        if (pcm == NULL) {
            pcm = argv[i];
        } else if (spx == NULL) {
            spx = argv[i];
        } else {
            ERROR("You have specificed the INPUT and OUTPUT files!!!");
            rs = DUER_ERR_INVALID_ARGUMENT;
            goto exit;
        }
        i++;
    }

    if (spx == NULL) {
        i = strlen(pcm) + strlen(suffix);
        temp = (char *)malloc(i + 1);
        if (temp == NULL) {
            ERROR("Memory overflow!!!");
            rs = DUER_ERR_MEMORY_OVERFLOW;
            goto exit;
        }
        snprintf(temp, i + 1, "%s%s", pcm, suffix);
        spx = temp;
    }

    rs = pcm2spx(pcm, spx);

exit:

    if (temp != NULL) {
        free(temp);
    }

    if (rs < DUER_NO_ERROR) {
        usage(argv[0]);
    }

    return rs;
}
