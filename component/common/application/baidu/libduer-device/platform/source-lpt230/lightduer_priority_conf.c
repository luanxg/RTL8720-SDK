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
 * File: lightduer_priority_conf.c
 * Auth: Chen Hanwen (chenhanwen@baidu.com)
 * Desc: Adapt the priority function to LPT230 VER1.0
 * babeband:RDA5981A:total internal sram:352KB,flash size:1MB,system:mbed
 */

#include <string.h>
#include "lightduer_priority_conf.h"
#include "hsf.h"

static int s_task_priorities[DUER_TASK_TOTAL] = {
#if defined(DUER_PLATFORM_MARVELL)
    2, 3, 2, 4, 3
#else
    HFTHREAD_MAX_PRIORITIES, HFTHREAD_MAX_PRIORITIES, HFTHREAD_MAX_PRIORITIES, HFTHREAD_PRIORITIES_HIGH, HFTHREAD_MAX_PRIORITIES    
#endif
};

static const char* const g_priority_tags[] = {
    "lightduer_ca",
    "lightduer_voice",
    "lightduer_socket",
    "lightduer_OTA_Updater",
    "lightduer_msgdispatch",
};

int duer_priority_get_task_id(const char* name) {
    int i = 0;

    for (i = 0; i < DUER_TASK_TOTAL; i++) {
        if (strcmp(g_priority_tags[i], name) == 0) {
            return i;
        }
    }

    return -1;
}

void duer_priority_set(int task, int priority) {
    if (task >= 0 && task < DUER_TASK_TOTAL) {
        s_task_priorities[task] = priority;
    }
}

int duer_priority_get(int task) {
    return (task >= 0 && task < DUER_TASK_TOTAL) ? s_task_priorities[task] : -1;
}
