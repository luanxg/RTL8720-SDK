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

#include "test.h"

#include "lightduer_mutex.h"
#include "lightduer_types.h"

void baidu_ca_mutex_init(duer_mutex_create_f f_create,
        duer_mutex_lock_f f_lock,
        duer_mutex_unlock_f f_unlock,
        duer_mutex_destroy_f f_destroy) {
    check_expected(f_create);
    check_expected(f_lock);
    check_expected(f_unlock);
    check_expected(f_destroy);
}

duer_mutex_t duer_mutex_create() {
    return mock_ptr_type(duer_mutex_t);
}

duer_status_t duer_mutex_lock(duer_mutex_t mutex) {
    check_expected(mutex);
    return mock_type(duer_status_t);
}

duer_status_t duer_mutex_unlock(duer_mutex_t mutex) {
    check_expected(mutex);
    return mock_type(duer_status_t);
}

