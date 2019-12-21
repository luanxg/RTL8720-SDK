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
#include "heap_monitor.h"

void *bcamem_alloc(duer_context ctx, duer_size_t size)
{
    return MALLOC(size, CA);
}

void *bcamem_realloc(duer_context ctx, void *ptr, duer_size_t size)
{
    return REALLOC(ptr, size, CA);
}

void bcamem_free(duer_context ctx, void *ptr)
{
    return FREE(ptr);
}
