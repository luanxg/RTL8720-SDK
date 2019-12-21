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
 * File: lightduer_data_cache.c
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Light duer stored the send data.
 */

#include "lightduer_types.h"
#include "lightduer_data_cache.h"
#include "lightduer_queue_cache.h"
#include "lightduer_log.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"

static duer_qcache_handler  s_handler = NULL;
static duer_mutex_t         s_mutex = NULL;

static duer_dcache_item *duer_dcache_item_create(const void *data, size_t size, duer_bool need_copy) {
    duer_dcache_item *cache = (duer_dcache_item *)DUER_MALLOC(sizeof(duer_dcache_item)); // ~= 8

    if (cache) {
        if (need_copy == DUER_FALSE) {
            cache->data = (void*)data;
            cache->size = size;
        } else {
            cache->data = DUER_MALLOC(size);
            if (cache->data) {
                DUER_MEMCPY(cache->data, data, size);
                cache->size = size;
            } else {
                DUER_FREE(cache);
                cache = NULL;
            }
        }
    } else {
        DUER_LOGE("malloc fail!");
        if (need_copy == DUER_FALSE) { // responsibility for the memory if error happen
            DUER_FREE((void*)data);
        }
    }

    return cache;
}

void duer_dcache_item_destory(void *data) {
    duer_dcache_item *cache = (duer_dcache_item *)data;
    if (cache) {
        if (cache->data) {
            DUER_FREE(cache->data);
        }
        DUER_FREE(cache);
    }
}

void duer_dcache_initialize(void) {
    if (s_handler == NULL) {
        s_handler = duer_qcache_create();
    }

    if (s_mutex == NULL) {
        s_mutex = duer_mutex_create();
    }
}

duer_status_t duer_dcache_push(const void *data, size_t size, duer_bool need_copy) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_dcache_item *cache = NULL;

    do {
        if (s_handler == NULL) {
            break;
        }

        cache = duer_dcache_item_create(data, size, need_copy);
        if (cache == NULL) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            break;
        }

        duer_mutex_lock(s_mutex);
        rs = duer_qcache_push(s_handler, cache);
        if (rs == DUER_OK) {
            rs = (duer_status_t)duer_qcache_length(s_handler);
        }
        duer_mutex_unlock(s_mutex);
    } while (0);

    return rs;
}

duer_dcache_item *duer_dcache_top(void) {
    duer_dcache_item *cache = NULL;

    duer_mutex_lock(s_mutex);
    cache = (duer_dcache_item *)duer_qcache_top(s_handler);
    duer_mutex_unlock(s_mutex);

    return cache;
}

void *duer_dcache_create_iterator(void) {
    void *head_iterator = NULL;

    duer_mutex_lock(s_mutex);
    head_iterator = (void *)duer_qcache_head(s_handler);
    duer_mutex_unlock(s_mutex);

    return head_iterator;
}

void *duer_dcache_get_next(void *iterator) {
    if (!iterator) {
        return NULL;
    }
    void *iterator_next = NULL;

    duer_mutex_lock(s_mutex);
    iterator_next = (void *)duer_qcache_next(iterator);
    duer_mutex_unlock(s_mutex);

    return iterator_next;
}

duer_dcache_item *duer_dcache_get_data(void *iterator) {
    if (!iterator) {
        return NULL;
    }
    duer_dcache_item *cache = NULL;

    duer_mutex_lock(s_mutex);
    cache = (duer_dcache_item *)duer_qcache_data(iterator);
    duer_mutex_unlock(s_mutex);

    return cache;
}

void duer_dcache_pop(void) {
    duer_dcache_item *cache = NULL;

    duer_mutex_lock(s_mutex);
    cache = (duer_dcache_item *)duer_qcache_pop(s_handler);
    duer_mutex_unlock(s_mutex);

    duer_dcache_item_destory(cache);
}

size_t duer_dcache_length(void) {
    size_t length = 0;

    duer_mutex_lock(s_mutex);
    length = duer_qcache_length(s_handler);
    duer_mutex_unlock(s_mutex);

    return length;
}

void duer_dcache_clear(void) {
    duer_dcache_item *cache = NULL;

    duer_mutex_lock(s_mutex);
    while ((cache = (duer_dcache_item *)duer_qcache_pop(s_handler)) != NULL) {
        duer_dcache_item_destory(cache);
    }
    duer_mutex_unlock(s_mutex);
}

void duer_dcache_finalize(void) {
    if (s_handler != NULL) {
        duer_qcache_destroy_traverse(s_handler, duer_dcache_item_destory);
        s_handler = NULL;
    }

    if (s_mutex != NULL) {
        duer_mutex_destroy(s_mutex);
        s_mutex = NULL;
    }
}
