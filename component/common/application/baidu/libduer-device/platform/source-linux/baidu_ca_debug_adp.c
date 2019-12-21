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
//
// File: baidu_ca_debug_adp.c
// Auth: Zhang Leliang (zhangleliang@baidu.com)
// Desc: Adapt the debug function to linux.


#include <unistd.h>
#include <sys/syscall.h>
#ifdef __COMPILER_AARCH64__
#include <sys/types.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

#include "baidu_ca_adapter_internal.h"
#include "lightduer_log.h"
#include "lightduer_timestamp.h"

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

#ifndef ANDROID
static pid_t gettid() {
    return syscall(SYS_gettid);
}
#endif

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
#ifdef ANDROID
    switch (level) {
    case 1: // ERROR
        __android_log_print(ANDROID_LOG_ERROR, duer_get_tag(level), "%s(%d):%s", file, line, msg);
        break;
    case 2: // Warning
        __android_log_print(ANDROID_LOG_WARN, duer_get_tag(level), "%s(%d):%s", file, line, msg);
        break;
    case 3: // Info
        __android_log_print(ANDROID_LOG_INFO, duer_get_tag(level), "%s(%d):%s", file, line, msg);
        break;
    case 4: // Debug
        __android_log_print(ANDROID_LOG_DEBUG, duer_get_tag(level), "%s(%d):%s", file, line, msg);
        break;
    case 5: // Verbose
        __android_log_print(ANDROID_LOG_VERBOSE, duer_get_tag(level), "%s(%d):%s", file, line, msg);
        break;
    default:
        __android_log_print(ANDROID_LOG_ERROR, duer_get_tag(level), "%s(%d):%s", file, line, msg);
        break;
    }

#else
    printf("[%s]{tid:%d}(%u)%s(%d):%s",
        duer_get_tag(level), gettid(), duer_timestamp(), file, line, msg);
#endif

}
