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
 * File: duerapp_config.h
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Duer Configuration.
 */

#ifndef BAIDU_DUER_DUERAPP_DUERAPP_CONFIG_H
#define BAIDU_DUER_DUERAPP_DUERAPP_CONFIG_H

void initialize_wifi(void);

void initialize_sdcard(void);

const char *duer_load_profile(const char *path);

/*
 * Recorder APIS
 */

enum duer_record_events_enum {
    REC_START,
    REC_DATA,
    REC_STOP
};

typedef void (*duer_record_event_func)(int status, const void *data, size_t size);

void duer_record_set_event_callback(duer_record_event_func func);

void duer_record_all(const char* path, short interval);

#endif/*BAIDU_DUER_DUERAPP_DUERAPP_CONFIG_H*/
