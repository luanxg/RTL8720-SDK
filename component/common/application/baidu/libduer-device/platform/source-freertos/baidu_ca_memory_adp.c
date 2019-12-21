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
 * File: baidu_ca_memory_adp.c
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Adapt the memory function to esp32.
 */

#include "baidu_ca_adapter_internal.h"

#if defined(DUER_PLATFORM_ESPRESSIF)
#include "esp_system.h"
#include "esp_heap_caps.h"
#endif

void *bcamem_alloc(duer_context ctx, duer_size_t size)
{
#if defined(DUER_PLATFORM_ESPRESSIF)
#if IDF_3_0
    return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    return pvPortMallocCaps(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
#else
//Edit by Realtek
#if 0
	return malloc(size);
#else
   void *ret = pvPortMalloc(size);
   if(ret)
      memset(ret, 0, size);
   return ret;
 #endif
#endif
}

void *bcamem_realloc(duer_context ctx, void *ptr, duer_size_t size)
{
#if defined(DUER_PLATFORM_ESPRESSIF)
#if IDF_3_0
    return heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    void *p = NULL;

    p = pvPortMallocCaps(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) {
        return NULL;
    } else {
        memcpy(p, ptr, size);
    }

    free(ptr);
    return p;
#endif
#else
//Edit by Realtek
#if 0
   return realloc(ptr, size);
#else
    void *p = NULL;

    p = pvPortMalloc(size);
    if (!p) {
        return NULL;
    } else {
        memcpy(p, ptr, size);
    }

    vPortFree(ptr);
    return p;
#endif
#endif
}

void bcamem_free(duer_context ctx, void *ptr)
{
#if defined(DUER_PLATFORM_ESPRESSIF)
#if IDF_3_0
    heap_caps_free(ptr);
#else
    free(ptr);
#endif
#else
//Edit by Realtek
#if 0
	free(ptr);
#else
	vPortFree(ptr);
#endif
#endif
}
