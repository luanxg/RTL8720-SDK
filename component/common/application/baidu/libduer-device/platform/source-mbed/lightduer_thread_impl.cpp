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
 * File: lightduer_thread_impl.cpp
 * Auth: Zhang Leliang(zhangleliang@baidu.com)
 * Desc: Provide the thread APIs.
 */

#include <mbed.h>
#include "lightduer_thread_impl.h"

duer_u32_t duer_get_thread_id_impl() {
    osThreadId thread_id = Thread::gettid();
    return (duer_u32_t)thread_id;
}
