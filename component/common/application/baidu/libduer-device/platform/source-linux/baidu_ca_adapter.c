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
// File: baidu_ca_adapter.c
// Auth: Zhang Leliang (zhangleliang@baidu.com)
// Desc: Adapt the IoT CA to different platform.


#include "lightduer_adapter.h"

#include <sys/time.h>  // gettimeofday  get time in micro-second
#include <unistd.h>

#include "baidu_ca_adapter_internal.h"

#include "lightduer_debug.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_net_transport.h"
#include "lightduer_sleep.h"
#include "lightduer_timestamp.h"
#include "lightduer_random.h"
#include "lightduer_random_impl.h"
#ifndef DISABLE_REPORT_SYSTEM_INFO
#include "lightduer_statistics.h"
#endif

static volatile int s_initialized = 0;

static duer_u32_t duer_timestamp_obtain()
{
    //DUER_LOGV("duer_timestamp_obtain");
    struct timeval tv;
    int ret = gettimeofday(&tv, NULL);
    if (ret) {
        perror("gettimeofday error");
        return 0;
    }
    unsigned long long time_in_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return (duer_u32_t)time_in_ms;
}

static void duer_sleep_impl(duer_u32_t ms) {
    usleep(ms*1000);
}

void baidu_ca_adapter_initialize()
{
    if (s_initialized) {
        return;
    }
    s_initialized = 1;

    baidu_ca_memory_init(
        NULL,
        bcamem_alloc,
        bcamem_realloc,
        bcamem_free);

    baidu_ca_mutex_init(
          create_mutex,
          lock_mutex,
          unlock_mutex,
          delete_mutex_lock);

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
    duer_random_init(duer_random_impl);
#ifndef DISABLE_REPORT_SYSTEM_INFO
    duer_statistics_initialize();
#endif
#ifdef DUER_HEAP_MONITOR
    baidu_ca_memory_hm_init();
#endif // DUER_HEAP_MONITOR
}

void baidu_ca_adapter_finalize(void)
{
    if (!s_initialized) {
        return;
    }
    s_initialized = 0;

    bcasoc_finalize();

    baidu_ca_memory_uninit();
#ifdef DUER_HEAP_MONITOR
    baidu_ca_memory_hm_destroy();
#endif // DUER_HEAP_MONITOR
}
