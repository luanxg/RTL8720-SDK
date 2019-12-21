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
 * File: baidu_ca_debug_adp.c
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Adapt the debug function to esp32.
 */

#include "baidu_ca_adapter_internal.h"
#include "lightduer_log.h"
#include "lightduer_timestamp.h"

#ifdef ENABLE_LIGHTDUER_SNPRINTF
#include "lightduer_snprintf.h"
#endif

#if defined(DUER_PLATFORM_ESPRESSIF)
#include "esp_log.h"
#include "esp_err.h"
//Edit by RTK
#include "FreeRTOS.h"
#include "task.h"
#elif defined(DUER_PLATFORM_MARVELL)
int wmprintf(const char *format, ...);
#endif

#ifdef DUER_DEBUG_LEVEL

static int g_debug_level = DUER_DEBUG_LEVEL;

void duer_debug_level_set(int level)
{
    g_debug_level = level;
}

#endif/*DUER_DEBUG_LEVEL*/

const char *duer_get_tag(int level)
{
    static const char *tags[] = {
        "WTF",
        "E",
        "W",
        "I",
        "D",
        "W"
    };
    static const size_t length = sizeof(tags) / sizeof(tags[0]);

    if (level > 0 && level < length) {
        return tags[level];
    }

    return "UNDEF";
}

void bcadbg(duer_context ctx, duer_u32_t level, const char *file, duer_u32_t line, const char *msg)
{
    if (file == NULL) {
        file = "unkown";
    }

#ifdef DUER_DEBUG_LEVEL
    if (level > g_debug_level) {
        return;
    }
#endif

#if defined(DUER_PLATFORM_ESPRESSIF)
    TaskHandle_t tid = xTaskGetCurrentTaskHandle();
#ifdef ENABLE_LIGHTDUER_SNPRINTF
    char line_header[32];
    lightduer_snprintf(line_header, sizeof(line_header), "%s (%u,tid:%x) ", duer_get_tag(level), duer_timestamp(), tid);
    fputs(line_header, stdout);
    fputs(msg, stdout);
#else
    esp_log_write(ESP_LOG_INFO, "duer", "%s (%u,tid:%x) %s(%4d): %s", duer_get_tag(level), duer_timestamp(), tid, file, line, msg);
#endif
#elif defined(DUER_PLATFORM_MARVELL)
    wmprintf("%s (%u) %s(%4d): %s", duer_get_tag(level), duer_timestamp(), file, line, msg);
#else
    printf("%s (%u) %s(%4d): %s", duer_get_tag(level), duer_timestamp(), file, line, msg);
#endif
}
