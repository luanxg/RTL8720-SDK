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
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Provide the timer APIs.
 */

#include <mbed.h>
#include "lightduer_timers.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_connagent.h"
#include "heap_monitor.h"

class DuerTimer
{
public:

    DuerTimer(duer_timer_callback callback, void *param, os_timer_type type)
        : _timer(mbed::Callback<void ()>(this, &DuerTimer::time_expired), type)
        , _callback(callback)
        , _param(param)
    {
    }

    void time_expired(void)
    {
        DUER_LOGV("time_expired %p", _callback);
        if (_callback) {
            _callback(_param);
        }
    }

    int start(int delay)
    {
        DUER_LOGV("start %p", _callback);
        return _timer.start(delay);
    }

    int stop()
    {
        DUER_LOGV("stop %p", _callback);
        return _timer.stop();
    }

private:

    rtos::RtosTimer         _timer;
    duer_timer_callback     _callback;
    void *                  _param;
};

duer_timer_handler duer_timer_acquire(duer_timer_callback callback, void *param, int type)
{
    return NEW (CA) DuerTimer(callback, param, type == DUER_TIMER_PERIODIC ? osTimerPeriodic : osTimerOnce);
}

int duer_timer_start(duer_timer_handler handle, size_t delay)
{
    DuerTimer *timer = (DuerTimer *)handle;
    int rs = DUER_ERR_FAILED;

    do {
        if (timer == NULL) {
            rs = DUER_ERR_INVALID_PARAMETER;
            DUER_LOGE("Invalid paramiter!!!");
            break;
        }

        if ((rs = timer->start(delay)) != osOK) {
            DUER_LOGE("Start timer failed!!! rs = %d", rs);
            break;
        }

        rs = DUER_OK;
    } while (0);

    return rs;
}

int duer_timer_is_valid(duer_timer_handler handle)
{
    return (handle != NULL);
}

int duer_timer_stop(duer_timer_handler handle)
{
    DuerTimer *timer = (DuerTimer *)handle;
    int rs = DUER_ERR_FAILED;

    do {
        if (timer == NULL) {
            rs = DUER_ERR_INVALID_PARAMETER;
            DUER_LOGE("Invalid paramiter!!!");
            break;
        }

        if ((rs = timer->stop()) != osOK) {
            if (rs != osErrorResource) {
                DUER_LOGE("Stop timer failed!!! rs = %d", rs);
                break;
            } else {
                DUER_LOGD("timer is not start!! rs = %d", rs);
            }

        }

        rs = DUER_OK;
    } while (0);

    return rs;
}

void duer_timer_release(duer_timer_handler handle)
{
    DuerTimer *timer = (DuerTimer *)handle;
    if (timer != NULL) {
        delete timer;
    }

}
