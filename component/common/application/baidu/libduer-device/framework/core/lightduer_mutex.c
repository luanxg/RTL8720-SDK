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
// Description: Wrapper for mutex

#include "lightduer_mutex.h"

typedef struct _baidu_ca_mutex_cbs_s {
    duer_mutex_create_f f_create;
    duer_mutex_lock_f f_lock;
    duer_mutex_unlock_f f_unlock;
    duer_mutex_destroy_f f_destroy;
} duer_mutex_cbs;

DUER_LOC_IMPL duer_mutex_cbs s_duer_mutex_cbs = {NULL};

DUER_EXT_IMPL void baidu_ca_mutex_init(duer_mutex_create_f f_create,
                                      duer_mutex_lock_f f_lock,
                                      duer_mutex_unlock_f f_unlock,
                                      duer_mutex_destroy_f f_destroy) {
    s_duer_mutex_cbs.f_create = f_create;
    s_duer_mutex_cbs.f_lock = f_lock;
    s_duer_mutex_cbs.f_unlock = f_unlock;
    s_duer_mutex_cbs.f_destroy = f_destroy;
}

DUER_INT_IMPL duer_mutex_t duer_mutex_create() {
    return s_duer_mutex_cbs.f_create ? s_duer_mutex_cbs.f_create() : NULL;
}

DUER_INT_IMPL duer_status_t duer_mutex_lock(duer_mutex_t mutex) {
    return s_duer_mutex_cbs.f_lock ? s_duer_mutex_cbs.f_lock(mutex) : DUER_ERR_FAILED;
}

DUER_INT_IMPL duer_status_t duer_mutex_unlock(duer_mutex_t mutex) {
    return s_duer_mutex_cbs.f_unlock ? s_duer_mutex_cbs.f_unlock(mutex) : DUER_ERR_FAILED;
}

DUER_INT_IMPL duer_status_t duer_mutex_destroy(duer_mutex_t mutex) {
    return s_duer_mutex_cbs.f_destroy ? s_duer_mutex_cbs.f_destroy(mutex) : DUER_ERR_FAILED;
}
