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
// Description: The random adapter.

#include "lightduer_random.h"
#include <time.h>
#include "lightduer_lib.h"

DUER_LOC_IMPL duer_random_f s_duer_random = NULL;

DUER_EXT_IMPL void duer_random_init(duer_random_f f_random) {
    s_duer_random = f_random;
    srand(time(NULL));
}

DUER_INT_IMPL duer_s32_t duer_random() {
    duer_s32_t rs = 0;

    if (s_duer_random) {
        rs = s_duer_random();
    }
    if (rs == 0) {
        rs = rand();
    }

    return rs;
}
