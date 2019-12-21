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
 * File: baidu_ca_adapter_internal.h
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Adapt the IoT CA to different platform.
 */

#ifndef BAIDU_DUER_IOT_CA_ADAPTER_BAIDU_CA_ADAPTER_INTERNAL_H
#define BAIDU_DUER_IOT_CA_ADAPTER_BAIDU_CA_ADAPTER_INTERNAL_H

#include "lightduer_types.h"
#include "lightduer_net_transport.h"
#include "lightduer_mutex.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Memory adapter
 */

extern void *bcamem_alloc(duer_context ctx, duer_size_t size);

extern void *bcamem_realloc(duer_context ctx, void *ptr, duer_size_t size);

extern void bcamem_free(duer_context ctx, void *ptr);

/*
 * Socket adapter
 */

extern void bcasoc_initialize(void);

extern duer_socket_t bcasoc_create(duer_transevt_func context);

extern duer_status_t bcasoc_connect(duer_socket_t ctx, const duer_addr_t *addr);

extern duer_status_t bcasoc_send(duer_socket_t ctx, const void *data, duer_size_t size, const duer_addr_t *addr);

extern duer_status_t bcasoc_recv(duer_socket_t ctx, void *data, duer_size_t size, duer_addr_t *addr);

extern duer_status_t bcasoc_recv_timeout(duer_socket_t ctx, void *data, duer_size_t size, duer_u32_t timeout, duer_addr_t *addr);

extern duer_status_t bcasoc_close(duer_socket_t ctx);

extern duer_status_t bcasoc_destroy(duer_socket_t ctx);

/*
 * Debug adapter
 */

extern void bcadbg(duer_context ctx, duer_u32_t level, const char *file, duer_u32_t line, const char *msg);

/*
 * Mutex adapter
 */

extern duer_mutex_t create_mutex(void);

extern duer_status_t lock_mutex(duer_mutex_t mutex);

extern duer_status_t unlock_mutex(duer_mutex_t mutex);

extern duer_status_t delete_mutex_lock(duer_mutex_t mutex);

#ifdef __cplusplus
}
#endif

#endif/*BAIDU_DUER_IOT_CA_ADAPTER_BAIDU_CA_ADAPTER_INTERNAL_H*/
