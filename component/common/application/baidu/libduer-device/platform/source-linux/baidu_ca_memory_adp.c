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
// File: baidu_ca_memory_adp.c
// Auth: Zhang Leliang (zhangleliang @baidu.com)
// Desc: Adapt the memory function to linux.


#include "baidu_ca_adapter_internal.h"

void *bcamem_alloc(duer_context ctx, duer_size_t size)
{
    return malloc(size);
}

void *bcamem_realloc(duer_context ctx, void *ptr, duer_size_t size)
{
    return realloc(ptr, size);
}

void bcamem_free(duer_context ctx, void *ptr)
{
    free(ptr);
}
