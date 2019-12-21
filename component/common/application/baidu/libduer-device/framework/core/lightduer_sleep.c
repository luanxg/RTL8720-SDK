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
// Author: Chang Li(changli@baidu.com)
//
// Description: The sleep adapter.

#include "lightduer_sleep.h"

duer_sleep_f_t f_sleep = NULL;

void baidu_ca_sleep_init(duer_sleep_f_t _f_sleep) {
    f_sleep = _f_sleep;
}

void duer_sleep(duer_u32_t ms) {
    if (f_sleep) {
        f_sleep(ms);
    }
}

