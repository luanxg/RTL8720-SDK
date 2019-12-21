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
// Author: Zhang Leliang(zhangleliang@baidu.com)
//
// Description: The thread adapter.

#include "lightduer_thread.h"
#include "lightduer_log.h"

static duer_get_thread_id_f_t f_get_thread_id = NULL;

void duer_thread_init(duer_get_thread_id_f_t _f_get_thread_id) {
    f_get_thread_id = _f_get_thread_id;
}

duer_u32_t duer_get_thread_id(void) {
    if (f_get_thread_id) {
        return f_get_thread_id();
    } else {
        DUER_LOGW("no impl!");
        return 0;
    }
}

