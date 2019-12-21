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
// Author: Su Hao (suhao@baidu.com)
//
// Description: The adapter for Memory Management.

#include "lightduer_memory.h"
#include "lightduer_lib.h"
#include "lightduer_log.h"
#include "lightduer_mutex.h"
#include "baidu_json.h"

typedef struct _baidu_ca_memory_s {
    duer_context     context;
    duer_malloc_f    f_malloc;
    duer_realloc_f   f_realloc;
    duer_free_f      f_free;
#ifdef DUER_MEMORY_USAGE
    duer_size_t      alloc_counts;
    duer_size_t      alloc_size;
    duer_size_t      max_size;
    duer_size_t      all_alloc_counts;
#endif/*DUER_MEMORY_USAGE*/
} duer_memory_t;

DUER_LOC_IMPL duer_memory_t s_duer_memory;


#ifdef DUER_HEAP_MONITOR

#undef HM_KEYWORD
#define HM_KEYWORD(symbol) #symbol,
static const char* const s_duer_memory_hm_name[] = {
    "DUER_MEMORY_HM_MIN",
#include "lightduer_memory_hm_keywords.h"
    "DUER_MEMORY_HM_MAX"
};

struct heap_info {
    duer_u32_t max_used_heap_size;
    duer_u32_t current_used_heap_size;
};

static struct heap_info s_hm_heap_info[DUER_MEMORY_HM_MAX];
static duer_mutex_t s_hm_mutex;

#ifndef SDCARD_PATH
#ifdef __MBED__
#define SDCARD_PATH     "/sd"  // refer to sdpath initialize g_sd
#elif defined ESP_PLATFORM
#define SDCARD_PATH     "/sdcard"
#else
#define SDCARD_PATH "."
#warning "default value for SDCARD_PATH is ."
#endif
#endif

static FILE *s_hm_collect_info;
static int s_line_num;
static void duer_memdbg_hm_to_file()
{
    if (s_hm_collect_info) {
        int i = DUER_MEMORY_HM_MIN;
        fprintf(s_hm_collect_info, "%d,", s_line_num++);
        for (i = DUER_MEMORY_HM_MIN + 1; i < DUER_MEMORY_HM_MAX; ++i) {
            fprintf(s_hm_collect_info, "%d,", s_hm_heap_info[i].current_used_heap_size);
        }
        fprintf(s_hm_collect_info, "%u", s_duer_memory.alloc_size);
        fprintf(s_hm_collect_info, "\n");
        fflush(s_hm_collect_info);
    }
}
#endif // DUER_HEAP_MONITOR

#ifdef DUER_MEMORY_USAGE

#define DUER_MEM_HDR_MASK        (0xFEDCBA98)

#ifdef DUER_HEAP_MONITOR
#define DUER_MEMORY_HEADER   \
    duer_u32_t   mask; \
    duer_memory_hm_module_e module; \
    duer_size_t  size;
#else
#define DUER_MEMORY_HEADER   \
    duer_u32_t   mask; \
    duer_size_t  size;
#endif // DUER_HEAP_MONITOR

#define DUER_MEM_CONVERT(ptr)  (duer_memdbg_t *)(ptr ? ((char *)ptr - sizeof(duer_memhdr_t)) : NULL)

typedef struct _baidu_ca_memory_header_s {
    DUER_MEMORY_HEADER
} duer_memhdr_t;

typedef struct _baidu_ca_memory_debug_s {
    DUER_MEMORY_HEADER
    char        data[];
} duer_memdbg_t;

DUER_LOC_IMPL void* duer_memdbg_acquire(duer_memdbg_t* ptr,
                                        duer_size_t size
#ifdef DUER_HEAP_MONITOR
                                        , duer_memory_hm_module_e module
#endif // DUER_HEAP_MONITOR
                                        )
{
    if (ptr) {
        ptr->mask = DUER_MEM_HDR_MASK;
        ptr->size = size - sizeof(duer_memhdr_t);

#ifdef DUER_HEAP_MONITOR
        if (s_hm_mutex) {
            duer_mutex_lock(s_hm_mutex);
        }
#endif // DUER_HEAP_MONITOR

        s_duer_memory.all_alloc_counts++;
        s_duer_memory.alloc_counts++;
        s_duer_memory.alloc_size += ptr->size;

        if (s_duer_memory.max_size < s_duer_memory.alloc_size) {
            s_duer_memory.max_size = s_duer_memory.alloc_size;
        }

#ifdef DUER_HEAP_MONITOR
        if (module <= DUER_MEMORY_HM_MIN || module >= DUER_MEMORY_HM_MAX) {
            DUER_LOGW("wrong module:%d", module);
        } else {
            ptr->module = module;
            s_hm_heap_info[module].current_used_heap_size += ptr->size;
            if (s_hm_heap_info[module].current_used_heap_size
                    > s_hm_heap_info[module].max_used_heap_size) {
                s_hm_heap_info[module].max_used_heap_size
                        = s_hm_heap_info[module].current_used_heap_size;
            }
        }

        duer_memdbg_hm_to_file();
        if (s_hm_mutex) {
            duer_mutex_unlock(s_hm_mutex);
        }
#endif // DUER_HEAP_MONITOR
    }

    return ptr ? ptr->data : NULL;
}

DUER_LOC_IMPL void duer_memdbg_release(duer_memdbg_t* p) {
    if (p) {
        if (p->mask != DUER_MEM_HDR_MASK) {
            DUER_LOGE("The memory <%x> has been trampled!!!", p->data);
        }

#ifdef DUER_HEAP_MONITOR
        if (s_hm_mutex) {
            duer_mutex_lock(s_hm_mutex);
        }
#endif // DUER_HEAP_MONITOR

        s_duer_memory.alloc_counts--;
        s_duer_memory.alloc_size -= p->size;

#ifdef DUER_HEAP_MONITOR
        duer_memory_hm_module_e module = p->module;
        if (module <= DUER_MEMORY_HM_MIN || module >= DUER_MEMORY_HM_MAX) {
            DUER_LOGW("wrong module:%d", module);
        } else {
            s_hm_heap_info[module].current_used_heap_size -= p->size;
        }

        duer_memdbg_hm_to_file();
        if (s_hm_mutex) {
            duer_mutex_unlock(s_hm_mutex);
        }
#endif // DUER_HEAP_MONITOR
    }

}

DUER_INT_IMPL void duer_memdbg_usage() {

#ifdef DUER_HEAP_MONITOR
    if (s_hm_mutex) {
       duer_mutex_lock(s_hm_mutex);
    }
#endif // DUER_HEAP_MONITOR

    DUER_LOGI("duer_memdbg_usage: alloc_counts:%d, all_counts:%d, alloc_size:%d, max_size:%d",
             s_duer_memory.alloc_counts, s_duer_memory.all_alloc_counts,
             s_duer_memory.alloc_size, s_duer_memory.max_size);

#ifdef DUER_HEAP_MONITOR_DEBUG
    int i = DUER_MEMORY_HM_MIN;
    int alloc_size = 0;
    for (i = DUER_MEMORY_HM_MIN + 1; i < DUER_MEMORY_HM_MAX; ++i) {
        DUER_LOGI("%s: current:%d, max:%d",
                s_duer_memory_hm_name[i],
                s_hm_heap_info[i].current_used_heap_size,
                s_hm_heap_info[i].max_used_heap_size);
        alloc_size += s_hm_heap_info[i].current_used_heap_size;
    }
    DUER_LOGI("=========:malloc_size:%d====s_line_num:%d======", alloc_size, s_line_num);
#endif // DUER_HEAP_MONITOR_DEBUG

#ifdef DUER_HEAP_MONITOR
    if (s_hm_mutex) {
        duer_mutex_unlock(s_hm_mutex);
    }
#endif // #ifdef DUER_HEAP_MONITOR

}

#endif // DUER_MEMORY_USAGE/

DUER_EXT_IMPL void baidu_ca_memory_init(duer_context context,
                                       duer_malloc_f f_malloc,
                                       duer_realloc_f f_realloc,
                                       duer_free_f f_free) {
    s_duer_memory.context = context;
    s_duer_memory.f_malloc = f_malloc;
    s_duer_memory.f_realloc = f_realloc;
    s_duer_memory.f_free = f_free;
    baidu_json_Hooks hooks = {
        duer_malloc_bjson,
        duer_free_bjson
    };
    baidu_json_InitHooks(&hooks);
}

DUER_EXT_IMPL void baidu_ca_memory_uninit()
{
#ifdef DUER_BJSON_PREALLOC_ITEM
    baidu_json_Uninit();
#endif // DUER_BJSON_PREALLOC_ITEM
}

#ifdef DUER_HEAP_MONITOR

/*
    init_hm(); TODO
*/
DUER_EXT_IMPL void baidu_ca_memory_hm_init()
{
    if (!s_hm_mutex) {
        s_hm_mutex = duer_mutex_create();
        if (!s_hm_mutex) {
            DUER_LOGE("create mutex error!!");
        }
    }
    if (!s_hm_collect_info) {
        const char *file_name = SDCARD_PATH"/hm_collect_info.txt";
        s_hm_collect_info = fopen(file_name, "w");
        if (s_hm_collect_info) {
            int i = DUER_MEMORY_HM_MIN;
            fwrite("INDEX,", 6, 1, s_hm_collect_info);
            for (i = DUER_MEMORY_HM_MIN + 1; i < DUER_MEMORY_HM_MAX; ++i) {
                fprintf(s_hm_collect_info, "%s", s_duer_memory_hm_name[i] + 8);
                fwrite(",", 1, 1, s_hm_collect_info);
            }
            fwrite("TOTAL", 5, 1, s_hm_collect_info);
            fwrite("\n", 1, 1, s_hm_collect_info);
            fflush(s_hm_collect_info);
        } else {
            DUER_LOGE("open s_hm_collect_info fail,%s!", file_name);
        }
    }
}

DUER_EXT_IMPL void baidu_ca_memory_hm_destroy()
{
    if (s_hm_mutex) {
       duer_mutex_lock(s_hm_mutex);
    }
    if (s_hm_collect_info) {
        // comment these here, because the file close too early
        fclose(s_hm_collect_info);
        s_hm_collect_info = NULL;
    }
    if (s_hm_mutex) {
        duer_mutex_unlock(s_hm_mutex);
    }

    if (s_hm_mutex) {
        //Note: don't release the mutex
        //duer_mutex_destroy(s_hm_mutex);
        //s_hm_mutex = NULL;
    }
}

static duer_memory_hm_module_e duer_memdbg_get_module(duer_memdbg_t* p)
{
    if (p) {
        if (p->mask != DUER_MEM_HDR_MASK) {
            DUER_LOGE("The memory <%x> has been trampled!!!", p->data);
            return DUER_MEMORY_HM_MIN;
        }
        return p->module;
    }
    return DUER_MEMORY_HM_MIN;
}

DUER_INT_IMPL void* duer_malloc_hm(duer_size_t size, duer_memory_hm_module_e module) {
    void *rs = NULL;
    char *head = NULL;
    struct mem_block_info *block_info = NULL;

    if (s_duer_memory.f_malloc) {
        size += sizeof(duer_memhdr_t);

        rs = s_duer_memory.f_malloc(s_duer_memory.context, size);
        if (rs) {

        }

        rs = duer_memdbg_acquire(rs, size, module);
    }

    return rs;
}

DUER_INT_IMPL void* duer_calloc_hm(duer_size_t size, duer_memory_hm_module_e module) {
    void* rs = NULL;

    if (s_duer_memory.f_malloc) {
        size += sizeof(duer_memhdr_t);

        rs = s_duer_memory.f_malloc(s_duer_memory.context, size);
        if (rs) {
            DUER_MEMSET(rs, 0, size);
        }

        rs = duer_memdbg_acquire(rs, size, module);
    }

    return rs;
}

DUER_INT_IMPL void* duer_realloc_hm(void* ptr, duer_size_t size, duer_memory_hm_module_e module) {
    void* rs = NULL;

    if (s_duer_memory.f_realloc || (s_duer_memory.f_malloc && s_duer_memory.f_free)) {

        ptr = DUER_MEM_CONVERT(ptr);
        size += sizeof(duer_memhdr_t);
        duer_memdbg_release(ptr);

        if (s_duer_memory.f_realloc) {
            rs = s_duer_memory.f_realloc(s_duer_memory.context, ptr, size);
        } else {
            DUER_LOGE("realloc_hm not set");
        }

        rs = duer_memdbg_acquire(rs, size, module);
    }

    return rs;
}

DUER_INT_IMPL void duer_free_hm(void* ptr)
{
    if (s_duer_memory.f_free) {
        ptr = DUER_MEM_CONVERT(ptr);
        duer_memdbg_release(ptr);

        s_duer_memory.f_free(s_duer_memory.context, ptr);
    }
}

DUER_INT_IMPL void *duer_malloc_bjson(duer_size_t size)
{
    return duer_malloc_hm(size, DUER_HM_CJSON);
}

DUER_INT_IMPL void duer_free_bjson(void *ptr)
{
    duer_free_hm(ptr);
}

#else

DUER_INT_IMPL void* duer_malloc_ext(duer_size_t size, const char* file,
                                  duer_u32_t line) {
    void* rs = duer_malloc(size);
    DUER_LOGI("duer_malloc_ext: file:%s, line:%d, addr = %x, size = %d", file, line, rs, size);

    return rs;
}

DUER_INT_IMPL void* duer_calloc_ext(duer_size_t size, const char* file,
                                  duer_u32_t line) {
    void* rs = duer_malloc(size);
    DUER_LOGI("duer_malloc_ext: file:%s, line:%d, addr = %x, size = %d", file, line, rs, size);

    if (rs) {
        DUER_MEMSET(rs, 0, size);
    }

    return rs;
}

DUER_INT_IMPL void* duer_realloc_ext(void* ptr, duer_size_t size, const char* file,
                                   duer_u32_t line) {
    void* rs = duer_realloc(ptr, size);
    DUER_LOGI("duer_realloc_ext: file:%s, line:%d, new_addr = %x, old_addr = %x, size = %d",
                                    file, line, rs, ptr, size);
    return rs;
}

DUER_INT_IMPL void duer_free_ext(void* ptr, const char* file, duer_u32_t line) {
    DUER_LOGI("duer_free_ext: file:%s, line:%d, addr = %x", file, line, ptr);
    duer_free(ptr);
}

DUER_INT_IMPL void* duer_malloc(duer_size_t size) {
    void* rs = NULL;

    if (s_duer_memory.f_malloc) {
#ifdef DUER_MEMORY_USAGE
        size += sizeof(duer_memhdr_t);
#endif
        rs = s_duer_memory.f_malloc(s_duer_memory.context, size);
#ifdef DUER_MEMORY_USAGE
        rs = duer_memdbg_acquire(rs, size);
#endif
    }

    return rs;
}

DUER_INT_IMPL void* duer_calloc(duer_size_t size) {
    void* rs = NULL;

    if (s_duer_memory.f_malloc) {
#ifdef DUER_MEMORY_USAGE
        size += sizeof(duer_memhdr_t);
#endif
        rs = s_duer_memory.f_malloc(s_duer_memory.context, size);
        if (rs) {
            DUER_MEMSET(rs, 0, size);
        }
#ifdef DUER_MEMORY_USAGE
        rs = duer_memdbg_acquire(rs, size);
#endif
    }

    return rs;
}

DUER_INT_IMPL void* duer_realloc(void* ptr, duer_size_t size) {
    void* rs = NULL;

    if (s_duer_memory.f_realloc || (s_duer_memory.f_malloc && s_duer_memory.f_free)) {
#ifdef DUER_MEMORY_USAGE
        ptr = DUER_MEM_CONVERT(ptr);
        size += sizeof(duer_memhdr_t);
        duer_memdbg_release(ptr);
#endif

        if (s_duer_memory.f_realloc) {
            rs = s_duer_memory.f_realloc(s_duer_memory.context, ptr, size);
        } else if (s_duer_memory.f_malloc && s_duer_memory.f_free) {
            DUER_LOGW("the impl here is not right!!");
            rs = duer_malloc(size);

            if (rs) {
                DUER_MEMCPY(rs, ptr, size); // where the ptr start and what's the right size???
            }

            duer_free(ptr);
        } else {
            // do nothing
        }

#ifdef DUER_MEMORY_USAGE
        rs = duer_memdbg_acquire(rs, size);
#endif
    }

    return rs;
}

DUER_INT_IMPL void duer_free(void* ptr) {
    if (s_duer_memory.f_free) {
#ifdef DUER_MEMORY_USAGE
        ptr = DUER_MEM_CONVERT(ptr);
        duer_memdbg_release(ptr);
#endif
        s_duer_memory.f_free(s_duer_memory.context, ptr);
    }
}

#ifdef DUER_MEMORY_USAGE
DUER_INT_IMPL void *duer_malloc_bjson(duer_size_t size)
{
    return duer_malloc_ext(size, __FILE__, __LINE__);
}

DUER_INT_IMPL void duer_free_bjson(void *ptr)
{
    return duer_free_ext(ptr, __FILE__, __LINE__);
}
#else // DUER_MEMORY_DEBUG
DUER_INT_IMPL void *duer_malloc_bjson(duer_size_t size)
{
    return duer_malloc(size);
}

DUER_INT_IMPL void duer_free_bjson(void *ptr)
{
    return duer_free(ptr);
}
#endif // DUER_MEMORY_USAGE

#endif //DUER_HEAP_MONITOR

