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
 * File: lightduer_priority_conf.h
 * Auth: Chen Hanwen (chenhanwen@baidu.com)
 * Desc: Adapt the priority function to LPT230 VER1.0
 * babeband:RDA5981A:total internal sram:352KB,flash size:1MB,system:mbed
 */

#ifndef BAIDU_DUER_LIGHTDUER_PORT_SOURCE_FREERTOS_LIGHTDUER_PRIORITY_CONF_H
#define BAIDU_DUER_LIGHTDUER_PORT_SOURCE_FREERTOS_LIGHTDUER_PRIORITY_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

enum _task_identify_e {
    DUER_TASK_CA,
    DUER_TASK_VOICE,
    DUER_TASK_SOCKET,
    DUER_TASK_OTA,
    DUER_MSG_DISPATCH,
    DUER_TASK_TOTAL
};

int duer_priority_get_task_id(const char* name);

void duer_priority_set(int task, int priority);

int duer_priority_get(int task);

#ifdef __cplusplus
}
#endif

#endif/*BAIDU_DUER_LIGHTDUER_PORT_SOURCE_FREERTOS_LIGHTDUER_PRIORITY_CONF_H*/
