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
 * Auth: Su Hao(suhao@baidu.com)
 * Desc: Record the audio player errors
 */

#include "lightduer_ds_log_audio_player.h"
#include "lightduer_ds_log.h"
#include "lightduer_log.h"

duer_status_t duer_ds_log_audply(duer_dslog_audply_code_t log_code, const char *uri, float speed,
        int code, const char *file, int line) {
    duer_ds_log_level_enum_t log_level = DUER_DS_LOG_LEVEL_INFO;
    baidu_json *msg = NULL;

    if (((code >> 16) & 0xF) == 5) {
        log_level = DUER_DS_LOG_LEVEL_ERROR;
    }

    msg = baidu_json_CreateObject();
    if (msg) {
        if (uri != NULL) {
            baidu_json_AddStringToObject(msg, "uri", uri);
        } else {
            DUER_LOGE("The uri is not defined!!!");
        }

        if (file != NULL) {
            baidu_json_AddStringToObject(msg, "file", file);
            baidu_json_AddNumberToObject(msg, "line", line);
        }

        if (code != 0) {
            baidu_json_AddNumberToObject(msg, "code", code);
        }

        if (speed > 0.000001f) {
            baidu_json_AddNumberToObject(msg, "download_speed", speed);
        }

        duer_ds_log(log_level, DUER_DS_LOG_MODULE_MEDIA, log_code, msg);
    } else {
        DUER_LOGE("Memory overflow!!!");
    }
    return DUER_OK;
}
