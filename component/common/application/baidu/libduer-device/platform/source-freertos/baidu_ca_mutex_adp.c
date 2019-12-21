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

#include "FreeRTOS.h"  //Edit By Realtek
#include "semphr.h"    //Edit By Realtek
#include "lightduer_mutex.h"

#ifdef DUER_PLATFORM_ESP8266
#include "adaptation.h"
#endif

duer_mutex_t create_mutex(void)
{
    duer_mutex_t mutex;

    mutex = (duer_mutex_t)xSemaphoreCreateMutex();

    return mutex;
}

duer_status_t lock_mutex(duer_mutex_t mutex)
{
    SemaphoreHandle_t semaphore;

    if (!mutex) {
        return DUER_ERR_FAILED;
    }

    semaphore = (SemaphoreHandle_t)mutex;

    if(xSemaphoreTake(semaphore, portMAX_DELAY) == pdTRUE) {
        return DUER_OK;
    } else {
        return DUER_ERR_FAILED;
    }
}

duer_status_t unlock_mutex(duer_mutex_t mutex)
{
    SemaphoreHandle_t semaphore;

    if (!mutex) {
        return DUER_ERR_FAILED;
    }

    semaphore = (SemaphoreHandle_t)mutex;

    if(xSemaphoreGive(semaphore) == pdTRUE) {
        return DUER_OK;
    } else {
        return DUER_ERR_FAILED;
    }
}

duer_status_t delete_mutex_lock(duer_mutex_t mutex)
{
    SemaphoreHandle_t semaphore;

    if (!mutex) {
        return DUER_ERR_FAILED;
    }

    semaphore = (SemaphoreHandle_t)mutex;

    vSemaphoreDelete(semaphore);

    return DUER_OK;
}
