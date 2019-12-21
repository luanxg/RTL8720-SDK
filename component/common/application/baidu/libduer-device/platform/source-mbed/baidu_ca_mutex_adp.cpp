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
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: Adapt the mutex function to esp32.
 */

#include <mbed.h>
#include "baidu_ca_adapter_internal.h"
#include "lightduer_mutex.h"
#include "lightduer_log.h"

duer_mutex_t bcamutex_create(void)
{
    duer_mutex_t mutex = new rtos::Mutex;

    return mutex;
}

duer_status_t bcamutex_lock(duer_mutex_t mutex)
{
    rtos::Mutex *object = reinterpret_cast<rtos::Mutex *>(mutex);

    if (object == NULL || object->lock() != osOK) {
        DUER_LOGE("lock failed: %p", object);
        return DUER_ERR_FAILED;
    }

    return DUER_OK;
}

duer_status_t bcamutex_unlock(duer_mutex_t mutex)
{
    rtos::Mutex *object = reinterpret_cast<rtos::Mutex *>(mutex);

    if (object == NULL || object->unlock() != osOK) {
        DUER_LOGE("unlock failed: %p", object);
        return DUER_ERR_FAILED;
    }

    return DUER_OK;
}

duer_status_t bcamutex_destroy(duer_mutex_t mutex)
{
    rtos::Mutex *object = reinterpret_cast<rtos::Mutex *>(mutex);

    if (object) {
        delete object;
    }

    return DUER_OK;
}
