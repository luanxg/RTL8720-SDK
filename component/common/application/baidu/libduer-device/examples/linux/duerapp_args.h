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
// Author: Zhang Leliang (zhangleliang@baidu.com)
//
// Description: parser for the args

#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_LINUX_DUERAPP_ARGS_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_LINUX_DUERAPP_ARGS_H

#include <getopt.h>
#include <stdbool.h>

#define PROGRAM_NAME    "linux-demo"
#define PROFILE_PATH    "/profile/profile"
#define VOICE_RECORD_PATH "/sample/sample.wav"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _duer_args {
    const char*  profile_path;       // profile
    const char*  voice_record_path;  // the voice file or folder
    short        interval_between_query; // the interval between two query

} duer_args_t;

#define DEFAULT_ARGS \
    {PROFILE_PATH, VOICE_RECORD_PATH, 5}

void duer_args_parse(int argc, char* args[], duer_args_t* argsp, bool allow_default);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_LINUX_DUERAPP_ARGS_H
