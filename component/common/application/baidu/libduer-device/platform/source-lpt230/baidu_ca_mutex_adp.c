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
 * File: baidu_ca_mutex_adp.c
 * Auth: Chen Hanwen (chenhanwen@baidu.com)
 * Desc: Adapt the mutex function to LPT230 VER1.0
 * babeband:RDA5981A:total internal sram:352KB,flash size:1MB,system:mbed
 */

#include "hsf.h"
#include "lightduer_mutex.h"

duer_mutex_t create_mutex(void) {
    hfthread_mutex_t mutex;
    int err_code = 0;

    err_code = hfthread_mutext_new(&mutex);

    if (err_code != HF_SUCCESS) {
        mutex = 0;
    }

    return (duer_mutex_t)mutex;
}

duer_status_t lock_mutex(duer_mutex_t mutex) {
    if (!mutex) {
        return DUER_ERR_FAILED;
    }

    hfthread_mutex_t _mutex;
    _mutex = (hfthread_mutex_t)mutex;

    if (hfthread_mutext_lock(_mutex) == HF_SUCCESS) {
        return DUER_OK;
    } else {
        return DUER_ERR_FAILED;
    }
}

duer_status_t unlock_mutex(duer_mutex_t mutex) {
    if (!mutex) {
        return DUER_ERR_FAILED;
    }

    hfthread_mutex_t _mutex;
    _mutex = (hfthread_mutex_t)mutex;

    hfthread_mutext_unlock(_mutex);

    return DUER_OK;

}

duer_status_t delete_mutex_lock(duer_mutex_t mutex) {
    if (!mutex) {
        return DUER_ERR_FAILED;
    }

    hfthread_mutex_t _mutex;
    _mutex = (hfthread_mutex_t)mutex;

    hfthread_mutext_free(_mutex);

    return DUER_OK;
}
