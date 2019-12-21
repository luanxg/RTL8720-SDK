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
 * Auth: Chen Hanwen (chenhanwen@baidu.com)
 * Desc: Adapt the debug function to LPT230 VER1.0
 * babeband:RDA5981A:total internal sram:352KB,flash size:1MB,system:mbed
 */

#include "baidu_ca_adapter_internal.h"
#include "lightduer_log.h"
#include "lightduer_timestamp.h"
#include "hsf.h"

#ifdef DUER_DEBUG_LEVEL

static int s_debug_level = DUER_DEBUG_LEVEL;

void duer_debug_level_set(int level) {
    s_debug_level = level;
}

#endif/*DUER_DEBUG_LEVEL*/

const char* duer_get_tag(int level) {
    static const char* tags[] = {
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

void bcadbg(duer_context ctx, duer_u32_t level, const char* file, duer_u32_t line,
            const char* msg) {
    if (file == NULL) {
        file = "unkown";
    }

#ifdef DUER_DEBUG_LEVEL

    if (level > s_debug_level) {
        return;
    }

#endif

    HF_Debug(DEBUG_ERROR, "%s (%u) %s(%4d): %s", duer_get_tag(level), hfsys_get_time(), file, line,
             msg);
}
