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

#include "baidu_ca_adapter_internal.h"
#include "lightduer_adapter.h"
#include "lightduer_adapter_config.h"
#include "lightduer_memory.h"
#include "lightduer_debug.h"
#include "lightduer_timestamp.h"
#include "lightduer_sleep.h"
#include "lightduer_net_transport.h"
#include "lightduer_statistics.h"
#include "FreeRTOS.h"   //Edit By Realtek
#include "task.h"       //Edit By Realtek
//Edit By Realtek
#define DISABLE_REPORT_SYSTEM_INFO 1

#ifdef DUER_PLATFORM_ESP8266
#include "adaptation.h"
#endif

static volatile int s_initialized = 0;

extern int duer_statistics_finalize(void);

static duer_u32_t duer_timestamp_obtain()
{
    int tick_period_ms = duer_get_port_tick_period_ms();
    return xTaskGetTickCount() * tick_period_ms;
}

static void duer_sleep_impl(duer_u32_t ms) {
    int tick_period_ms = duer_get_port_tick_period_ms();
    vTaskDelay(ms / tick_period_ms);
    //printf("%s: sleep %d ms.\n", __FUNCTION__, ms);
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
#ifndef DISABLE_REPORT_SYSTEM_INFO
    duer_statistics_initialize();
#endif
#ifdef DUER_HEAP_MONITOR
    baidu_ca_memory_hm_init();
#endif
}

void baidu_ca_adapter_finalize(void)
{
    if (!s_initialized) {
        return;
    }
    s_initialized = 0;

    bcasoc_finalize();
#ifndef DISABLE_REPORT_SYSTEM_INFO
    duer_statistics_finalize();
#endif

    baidu_ca_memory_uninit();
#ifdef DUER_HEAP_MONITOR
    baidu_ca_memory_hm_destroy();
#endif // DUER_HEAP_MONITOR
}
