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
//
// File: lightduer_timers.c
// Auth: Zhang leliang (zhangleliang @baidu.com)
// Desc: Provide the timer APIs.

#include "lightduer_timers.h"

#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>

#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"

#define DUER_TIMER_DELAY	(100)

typedef struct _duer_timers_s {
    timer_t             _timer;
    int                 _type;
    duer_timer_callback _callback;
    void *              _param;
} duer_timers_t;

static void duer_timer_callback_wrapper(union sigval param)
{
    pthread_setname_np(pthread_self(), "duer_timer");
    assert(param.sival_ptr);

    // Which timer expired?
    duer_timers_t *timer = (duer_timers_t *)(param.sival_ptr);

    if (timer != NULL && timer->_callback != NULL) {
        timer->_callback(timer->_param);
    }
}

duer_timer_handler duer_timer_acquire(duer_timer_callback callback, void *param, int type)
{
    duer_timers_t *handle = NULL;
    int rs = DUER_ERR_FAILED;
    int status;
    DUER_LOGD("duer_timer_acquire, type:%d", type);
    do {
        handle = (duer_timers_t *)DUER_MALLOC(sizeof(duer_timers_t)); // ~= 16

        if (handle == NULL) {
            DUER_LOGE("Memory Overflow!!!");
            break;
        }

        handle->_callback = callback;
        handle->_param = param;
        handle->_type = type;

        struct sigevent se;
        se.sigev_signo = 0; // not used
        se.sigev_notify = SIGEV_THREAD;
        se.sigev_value.sival_ptr = handle;
        se.sigev_notify_function = duer_timer_callback_wrapper;
        se.sigev_notify_attributes = NULL;

        status = timer_create(CLOCK_REALTIME, &se, &(handle->_timer));

        if (status) {
            DUER_LOGE("Timer Create Failed!!!, status:%d", status);
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

int duer_timer_start(duer_timer_handler handle, size_t delay)
{
    duer_timers_t *timer = (duer_timers_t *)handle;
    int rs = DUER_ERR_FAILED;

    do {
        if (timer == NULL || timer->_timer == NULL) {
            rs = DUER_ERR_INVALID_PARAMETER;
            break;
        }

        long long freq_nanosecs = delay * 1000000LL;
        struct itimerspec ts;
        ts.it_value.tv_sec = freq_nanosecs / 1000000000;
        ts.it_value.tv_nsec = freq_nanosecs % 1000000000;
        if (DUER_TIMER_PERIODIC == timer->_type) {
            ts.it_interval.tv_sec = ts.it_value.tv_sec;
            ts.it_interval.tv_nsec = ts.it_value.tv_nsec;
        } else {
            ts.it_interval.tv_sec = 0;
            ts.it_interval.tv_nsec = 0;
        }

        int status = timer_settime(timer->_timer, 0, &ts, NULL);
        if (status) {
            DUER_LOGW("timer start failed!error:%d", status);
        }


        rs = DUER_OK;
    } while (0);

    return rs;
}

int duer_timer_is_valid(duer_timer_handler handle)
{
    duer_timers_t *timer = (duer_timers_t *)handle;
    return ((timer != NULL) && (timer->_timer != NULL));
}

int duer_timer_stop(duer_timer_handler handle)
{
    duer_timers_t *timer = (duer_timers_t *)handle;
    if (timer != NULL && timer->_timer != NULL) {
        struct itimerspec ts;
        ts.it_value.tv_sec = 0;
        ts.it_value.tv_nsec = 0;
        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;

        int status = timer_settime(timer->_timer, 0, &ts, NULL);
        if (status) {
            DUER_LOGW("timer stop failed!error:%d", status);
        }
    }
    return 0;
}

void duer_timer_release(duer_timer_handler handle)
{
    duer_timers_t *timer = (duer_timers_t *)handle;
    if (timer != NULL) {
        if (timer->_timer != NULL) {
            duer_timer_stop(timer);
            int status = timer_delete(timer->_timer);
            if (status) {
                DUER_LOGW("timer delete failed!error:%d", status);
            }
            timer->_timer = NULL;
        }
        DUER_FREE(timer);
    }
}
