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
 * File: lightduer_timers.c
 * Auth: Chen Hanwen (chenhanwen@baidu.com)
 * Desc: Adapt the timer function to LPT230 VER1.0
 * babeband:RDA5981A:total internal sram:352KB,flash size:1MB,system:mbed
 */

#include "lightduer_timers.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_connagent.h"

#include "hsf.h"
#define true 1
#define false 0
#define DUER_TIMER_DELAY    (100)

typedef struct _duer_timers_s {
    hftimer_handle_t    timer;
    duer_timer_callback callback;
    void*               param;
} duer_timers_t;

static void duer_timer_callback_wrapper(hftimer_handle_t pxTimer) {
    //unused int32_t lArrayIndex;
    //unused const int32_t xMaxExpiryCountBeforeStopping = 10;

    // Optionally do something if the pxTimer parameter is NULL.
    //configASSERT(pxTimer);
    if (!pxTimer) {
        DUER_LOGE("pxTimer is NULL!!\n");
    }

    // Which timer expired?
    duer_timers_t* timer = (duer_timers_t*)hftimer_get_timer_id(pxTimer);

    if (timer != NULL && timer->callback != NULL) {
        timer->callback(timer->param);
    }
}

duer_timer_handler duer_timer_acquire(duer_timer_callback callback, void* param, int type) {
    duer_timers_t* handle = NULL;
    int rs = DUER_ERR_FAILED;

    do {
        handle = (duer_timers_t*)DUER_MALLOC(sizeof(duer_timers_t));

        if (handle == NULL) {
            DUER_LOGE("Memory Overflow!!!");
            break;
        }

        handle->callback = callback;
        handle->param = param;

        handle->timer = hftimer_create("duer_timers", // not used by the kernel.
                                       (0x7fffffff), // The timer period in ticks.
                                       type == DUER_TIMER_PERIODIC ? true :
                                       false, // auto-reload themselves when expire.
                                       (uint32_t)handle, // Assign each timer
                                       duer_timer_callback_wrapper,
                                       0   //soft timer
                                      );

        if (handle->timer == NULL) {
            DUER_LOGE("Timer Create Failed!!!");
            break;
        }

        rs = DUER_OK;
    } while (0);

    if (rs < DUER_OK) {
        duer_timer_release(handle);
        handle = NULL;
    }

    return handle;
}

int duer_timer_start(duer_timer_handler handle, size_t delay) {
    duer_timers_t* timer = (duer_timers_t*)handle;
    int rs = DUER_ERR_FAILED;

    do {
        if (timer == NULL || timer->timer == NULL) {
            rs = DUER_ERR_INVALID_PARAMETER;
            break;
        }

        hftimer_change_period(timer->timer, delay);

        // Start the timer.  No block time is specified, and even if one was
        // it would be ignored because the scheduler has not yet been
        // started.
        if (hftimer_start(timer->timer) != HF_SUCCESS) {
            // The timer could not be set into the Active state.
            DUER_LOGE("Failed to start timer");
            break;
        }

        rs = DUER_OK;
    } while (0);

    return rs;
}

int duer_timer_is_valid(duer_timer_handler handle) {
    duer_timers_t* timer = (duer_timers_t*)handle;
    return ((timer != NULL) && (timer->timer != NULL));
}

int duer_timer_stop(duer_timer_handler handle) {
    duer_timers_t* timer = (duer_timers_t*)handle;

    if (timer != NULL && timer->timer != NULL) {
        hftimer_stop(timer->timer);
    }
}

void duer_timer_release(duer_timer_handler handle) {
    duer_timers_t* timer = (duer_timers_t*)handle;

    if (timer != NULL) {
        if (timer->timer != NULL) {
            duer_timer_stop(timer);
            hftimer_delete(timer->timer);
            timer->timer = NULL;
        }

        DUER_FREE(timer);
    }

}
