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
 * File: baidu_ca_adapter.c
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Adapt the IoT CA to different platform.
 */

#include "lightduer_adapter.h"
#include "baidu_ca_adapter_internal.h"
#include "lightduer_memory.h"
#include "lightduer_debug.h"
#include "lightduer_timestamp.h"
#include "lightduer_sleep.h"
#include "lightduer_net_transport.h"
#include "lightduer_thread.h"
#include "lightduer_thread_impl.h"
#include "lightduer_random.h"
#include "lightduer_random_impl.h"
#include "lightduer_statistics.h"

#include "timer.h"
#include "rtos.h"

static mbed::Timer s_timer;
static volatile bool s_isInitialized = false;
static volatile bool s_isTimerStarted = false;

static duer_u32_t duer_timestamp_obtain()
{
    if (!s_isTimerStarted) {
        s_timer.start();
        s_isTimerStarted = true;
    }

    duer_u32_t timestamp = s_timer.read_us();
    return timestamp / 1000;
}

using rtos::Thread;

static void duer_sleep_impl(duer_u32_t ms)
{
    Thread::wait(ms);
}

void baidu_ca_adapter_initialize(void)
{
    if (s_isInitialized) {
        return ;
    }
    s_isInitialized = true;

    baidu_ca_memory_init(
        NULL,
        bcamem_alloc,
        bcamem_realloc,
        bcamem_free);

    baidu_ca_mutex_init(
        bcamutex_create,
        bcamutex_lock,
        bcamutex_unlock,
        bcamutex_destroy);

    baidu_ca_debug_init(NULL, bcadbg);

    bcasoc_initialize();

    baidu_ca_transport_init(
        bcasoc_create,
        bcasoc_connect,
        bcasoc_send,
        bcasoc_recv,
        bcasoc_recv_timeout,
        bcasoc_close,
        bcasoc_destroy);

    baidu_ca_timestamp_init(duer_timestamp_obtain);

    baidu_ca_sleep_init(duer_sleep_impl);

    duer_thread_init(duer_get_thread_id_impl);

    duer_random_init(duer_random_impl);

    duer_statistics_initialize();

#ifdef DUER_HEAP_MONITOR
    baidu_ca_memory_hm_init();
#endif // DUER_HEAP_MONITOR
}

void baidu_ca_adapter_finalize(void)
{
    if (!s_isInitialized) {
        return;
    }
    s_isInitialized = false;

    bcasoc_finalize();

    baidu_ca_memory_uninit();
#ifdef DUER_HEAP_MONITOR
    baidu_ca_memory_hm_destroy();
#endif // DUER_HEAP_MONITOR
}
