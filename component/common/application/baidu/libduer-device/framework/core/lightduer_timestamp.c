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
// Author: Su Hao (suhao@baidu.com)
//
// Description: The timestamp adapter.

#include "lightduer_timestamp.h"
#ifdef __linux__
#include <sys/time.h>
#endif
#include "lightduer_lib.h"

DUER_LOC_IMPL duer_timestamp_f s_duer_timestamp = NULL;

DUER_EXT_IMPL void baidu_ca_timestamp_init(duer_timestamp_f f_timestamp) {
    s_duer_timestamp = f_timestamp;
}

DUER_INT_IMPL duer_u32_t duer_timestamp() {
    duer_u32_t rs = 0;

    if (s_duer_timestamp) {
        rs = s_duer_timestamp();
    }
#ifdef __linux__
    else {
        struct timeval current;
        gettimeofday(&current, NULL);
        rs = current.tv_sec * 1000 + current.tv_usec / 1000;
    }
#endif
    return rs;
}
