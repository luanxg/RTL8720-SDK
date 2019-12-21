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

#include "test.h"

#undef DUER_MEMORY_DEBUG
#include "lightduer_memory.h"
#include "lightduer_timers.h"
#include "lightduer_queue_cache.h"

DUER_INT void* duer_malloc(duer_size_t size)
{
        check_expected(size);
            return mock_ptr_type(void*);
}

DUER_INT void duer_free(void* ptr)
{
        check_expected(ptr);
}

int duer_timer_start(duer_timer_handler handle, size_t delay)
{
    check_expected(handle);
    check_expected(delay);
    return mock_type(int);
}

duer_timer_handler duer_timer_acquire(duer_timer_callback callback, void *param, int type)
{
    check_expected(callback);
    check_expected(param);
    check_expected(type);
    return (duer_timer_handler)mock();
}

int duer_timer_stop(duer_timer_handler handle)
{
    check_expected(handle);
    return mock_type(int);
}

void *duer_qcache_pop(duer_qcache_handler cache)
{
    check_expected(cache);
    return (void *)mock();
}

void *duer_qcache_top(duer_qcache_handler cache)
{
    check_expected(cache);
    return (void *)mock();
}

int duer_qcache_push(duer_qcache_handler cache, void *data)
{
    check_expected(cache);
    check_expected(data);
    return (void *)mock();
}

duer_qcache_handler duer_qcache_create(void)
{
    return (duer_qcache_handler)mock();
}

void duer_qcache_destroy(duer_qcache_handler cache)
{
    check_expected(cache);
}

