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
// Author: suhao@baidu.com(suhao@baidu.com)
//
// The daemon for holding lightduer.

#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_TRACKER_DUER_DAEMON_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_TRACKER_DUER_DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

int duer_daemon_run(const char *prof);

#ifdef __cplusplus
}
#endif

#endif/*BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_TRACKER_DUER_DAEMON_H*/