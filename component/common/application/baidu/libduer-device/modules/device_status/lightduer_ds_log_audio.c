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
/*
 * Auth: Chen Xihao(chenxihao@baidu.com)
 * Desc: audio play related report log
 */

#include "lightduer_ds_log_audio.h"
#include <limits.h>
#include "lightduer_ds_log.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_timestamp.h"

duer_ds_audio_info_t g_audio_infos;

duer_status_t duer_ds_log_audio(duer_ds_log_audio_code_t log_code, const baidu_json *log_message)
{
    duer_ds_log_level_enum_t log_level = DUER_DS_LOG_LEVEL_INFO;
    switch (log_code) {
    case DUER_DS_LOG_AUDIO_BUFFER_INFO:
    case DUER_DS_LOG_AUDIO_PLAY_START:
    case DUER_DS_LOG_AUDIO_PLAY_PAUSE:
    case DUER_DS_LOG_AUDIO_PLAY_RESUME:
    case DUER_DS_LOG_AUDIO_PLAY_STOP:
    case DUER_DS_LOG_AUDIO_PLAY_FINISH:
    case DUER_DS_LOG_AUDIO_INFO:
    case DUER_DS_LOG_AUDIO_M4A_HEAD_SIZE:
    case DUER_DS_LOG_AUDIO_CONTENT_TYPE:
    case DUER_DS_LOG_AUDIO_DOWNLOAD_DELAY:
    case DUER_DS_LOG_AUDIO_PLAY_DELAY:
        log_level = DUER_DS_LOG_LEVEL_DEBUG;
        break;
    case DUER_DS_LOG_AUDIO_CODEC_VERSION:
        log_level = DUER_DS_LOG_LEVEL_INFO;
        break;
    case DUER_DS_LOG_AUDIO_NO_MEMORY:
    case DUER_DS_LOG_AUDIO_UNSUPPORT_FORMAT:
    case DUER_DS_LOG_AUDIO_UNSUPPORT_BITRATE:
    case DUER_DS_LOG_AUDIO_HEAD_TOO_LARGE:
    case DUER_DS_LOG_AUDIO_DOWNLOAD_FAILED:
    case DUER_DS_LOG_AUDIO_PLAYER_ERROR:
    case DUER_DS_LOG_AUDIO_M4A_INVALID_HEADER:
    case DUER_DS_LOG_AUDIO_M4A_PARSE_HEADER_ERROR:
    case DUER_DS_LOG_AUDIO_HLS_PARSE_M3U8_ERROR:
    case DUER_DS_LOG_AUDIO_HLS_DOWNLOAD_ERROR:
    case DUER_DS_LOG_AUDIO_HLS_RELOAD_ERROR:
    case DUER_DS_LOG_AUDIO_NULL_POINTER:
    case DUER_DS_LOG_AUDIO_INVALID_CONTEXT:
    case DUER_DS_LOG_AUDIO_SOFT_DECODER_ERROR:
    case DUER_DS_LOG_AUDIO_USER_DEFINED_ERROR:
    case DUER_DS_LOG_AUDIO_URL_NO_DATA:
    case DUER_DS_LOG_AUDIO_PLAY_FAILED:
        log_level = DUER_DS_LOG_LEVEL_WARN;
        break;
    case DUER_DS_LOG_AUDIO_BUFFER_ERROR:
    case DUER_DS_LOG_AUDIO_CODEC_ERROR:
        log_level = DUER_DS_LOG_LEVEL_ERROR;
        break;
    default:
        DUER_LOGW("unkown log code 0x%0x", log_code);
        return DUER_ERR_FAILED;
    }

    return duer_ds_log(log_level, DUER_DS_LOG_MODULE_MEDIA, log_code, log_message);
}

duer_status_t duer_ds_log_audio_buffer_info(duer_u32_t size, duer_u32_t type)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }


    baidu_json_AddNumberToObject(message, "buffer_size", size);

#if 0
    char buffer[16];
    switch (type) {
    case 0:
        DUER_SNPRINTF(buffer, sizeof(buffer), "heap");
        break;
    case 1:
        DUER_SNPRINTF(buffer, sizeof(buffer), "data segment");
        break;
    case 2:
        DUER_SNPRINTF(buffer, sizeof(buffer), "psram");
        break;
    case 3:
        DUER_SNPRINTF(buffer, sizeof(buffer), "file");
        break;
    default:
        DUER_SNPRINTF(buffer, sizeof(buffer), "unkown");
        break;
    }

    baidu_json_AddStringToObject(message, "buffer_type", buffer);
#else
    baidu_json_AddNumberToObject(message, "type", type);
#endif

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_BUFFER_INFO, message);

    return result;
}

duer_status_t duer_ds_log_audio_play_start(const char *url, duer_u32_t type)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObject(message, "url", url);
    baidu_json_AddNumberToObject(message, "source_type", type);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_PLAY_START, message);

    return result;
}

duer_status_t duer_ds_log_audio_play_stop(duer_u32_t duration, duer_u32_t block_count,
        duer_u32_t max_bitrate, duer_u32_t min_bitrate, duer_u32_t avg_bitrate)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;
    baidu_json *code_bitrate = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    code_bitrate = baidu_json_CreateObject();
    if (code_bitrate == NULL) {
        DUER_LOGE("code_bitrate json create fail");
        baidu_json_Delete(message);
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObjectCS(message, "duration", duration);
    baidu_json_AddNumberToObjectCS(message, "block_count", block_count);

    baidu_json_AddNumberToObjectCS(code_bitrate, "max", max_bitrate);
    baidu_json_AddNumberToObjectCS(code_bitrate, "min", min_bitrate);
    baidu_json_AddNumberToObjectCS(code_bitrate, "avg", avg_bitrate);

    baidu_json_AddItemToObjectCS(message, "codec_bitrate", code_bitrate);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_PLAY_STOP, message);

    return result;
}

duer_status_t duer_ds_log_audio_play_finish(duer_u32_t duration, duer_u32_t block_count,
        duer_u32_t max_bitrate, duer_u32_t min_bitrate, duer_u32_t avg_bitrate)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;
    baidu_json *code_bitrate = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    code_bitrate = baidu_json_CreateObject();
    if (code_bitrate == NULL) {
        DUER_LOGE("code_bitrate json create fail");
        baidu_json_Delete(message);
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObjectCS(message, "duration", duration);
    baidu_json_AddNumberToObjectCS(message, "block_count", block_count);

    baidu_json_AddNumberToObjectCS(code_bitrate, "max", max_bitrate);
    baidu_json_AddNumberToObjectCS(code_bitrate, "min", min_bitrate);
    baidu_json_AddNumberToObjectCS(code_bitrate, "avg", avg_bitrate);

    baidu_json_AddItemToObjectCS(message, "codec_bitrate", code_bitrate);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_PLAY_FINISH, message);

    return result;
}

duer_status_t duer_ds_log_audio_codec_version(const char *version)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObjectCS(message, "codec_version", version);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_CODEC_VERSION, message);

    return result;
}

duer_status_t duer_ds_log_audio_info(duer_u32_t bitrate,
        duer_u32_t sample_rate, duer_u32_t bits_per_sample, duer_u32_t channels)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObjectCS(message, "bitrate", bitrate);
    baidu_json_AddNumberToObjectCS(message, "sample_rate", sample_rate);
    baidu_json_AddNumberToObjectCS(message, "bits_per_sample", bits_per_sample);
    baidu_json_AddNumberToObjectCS(message, "channels", channels);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_INFO, message);

    return result;
}

duer_status_t duer_ds_log_audio_m4a_head_size(duer_u32_t size)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObjectCS(message, "head_size", size);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_M4A_HEAD_SIZE, message);

    return result;
}

duer_status_t duer_ds_log_audio_content_type(const char *type)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObjectCS(message, "content_type", type);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_CONTENT_TYPE, message);

    return result;
}

duer_status_t duer_ds_log_audio_download_delay()
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;
    duer_u32_t now = duer_timestamp();
    duer_u32_t delay = 0;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    delay = now > g_audio_infos.request_download_ts
                ? (now - g_audio_infos.request_download_ts)
                : (UINT_MAX - g_audio_infos.request_download_ts + now);

    baidu_json_AddNumberToObjectCS(message, "delay", delay);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_DOWNLOAD_DELAY, message);

    return result;
}

duer_status_t duer_ds_log_audio_play_delay()
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;
    duer_u32_t now = duer_timestamp();
    duer_u32_t delay = 0;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    now = duer_timestamp();
    delay = now > g_audio_infos.request_play_ts ? (now - g_audio_infos.request_play_ts)
                : (UINT_MAX - g_audio_infos.request_play_ts + now);

    baidu_json_AddNumberToObjectCS(message, "delay", delay);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_PLAY_DELAY, message);

    return result;
}

duer_status_t duer_ds_log_audio_memory_overflow(const char *file, duer_u32_t line)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObjectCS(message, "file", file);
    baidu_json_AddNumberToObjectCS(message, "line", line);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_NO_MEMORY, message);

    return result;
}

duer_status_t duer_ds_log_audio_null_pointer(const char *file, duer_u32_t line)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObjectCS(message, "file", file);
    baidu_json_AddNumberToObjectCS(message, "line", line);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_NULL_POINTER, message);

    return result;
}

duer_status_t duer_ds_log_audio_invalid_context(const char *file, duer_u32_t line)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObjectCS(message, "file", file);
    baidu_json_AddNumberToObjectCS(message, "line", line);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_INVALID_CONTEXT, message);

    return result;
}

duer_status_t duer_ds_log_audio_hls_parse_m3u8_error(duer_u32_t line)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObject(message, "line", line);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_HLS_PARSE_M3U8_ERROR, message);

    return result;
}

duer_status_t duer_ds_log_audio_framework_error(int error_code)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObjectCS(message, "error_code", error_code);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_SOFT_DECODER_ERROR, message);

    return result;
}

duer_status_t duer_ds_log_audio_player_error(int error_code)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObjectCS(message, "error_code", error_code);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_PLAYER_ERROR, message);

    return result;
}

duer_status_t duer_ds_log_audio_play_failed(const char *url)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddStringToObjectCS(message, "url", url);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_PLAY_FAILED, message);

    return result;
}

duer_status_t duer_ds_log_audio_user_defined_error(int error_code)
{
    duer_status_t result = DUER_OK;
    baidu_json *message = NULL;

    message = baidu_json_CreateObject();
    if (message == NULL) {
        DUER_LOGE("message json create fail");
        return DUER_ERR_MEMORY_OVERLOW;
    }

    baidu_json_AddNumberToObjectCS(message, "error_code", error_code);

    result = duer_ds_log_audio(DUER_DS_LOG_AUDIO_USER_DEFINED_ERROR, message);

    return result;
}

duer_status_t duer_ds_log_audio_request_play()
{
    g_audio_infos.request_play_ts = duer_timestamp();
    return DUER_OK;
}

duer_status_t duer_ds_log_audio_request_download()
{
    g_audio_infos.request_download_ts = duer_timestamp();
    return DUER_OK;
}
