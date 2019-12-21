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
 * Auth: Chen Hanwen (chenhanwen@baidu.com)
 * Desc: Adapt the memory function to LPT230 VER1.0
 * babeband:RDA5981A:total internal sram:352KB,flash size:1MB,system:mbed
 */

#include "baidu_ca_adapter_internal.h"
#include "hsf.h"

void* bcamem_alloc(duer_context ctx, duer_size_t size) {
    return hfmem_malloc(size);
}

void* bcamem_realloc(duer_context ctx, void* ptr, duer_size_t size) {
    return hfmem_realloc(ptr, size);
}

void bcamem_free(duer_context ctx, void* ptr) {
    hfmem_free(ptr);
}
