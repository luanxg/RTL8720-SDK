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
 * File: config.h
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Speex port configuration.
 */

#include "config.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"

void speex_error(const char *str)
{
    DUER_LOGE("Fatal (internal) error: %s\n", str);
}

void speex_warning(const char *str)
{
#ifndef DISABLE_WARNINGS
    DUER_LOGW("warning: %s\n", str);
#endif
}

void speex_warning_int(const char *str, int val)
{
#ifndef DISABLE_WARNINGS
    DUER_LOGW("warning: %s %d\n", str, val);
#endif
}

void speex_notify(const char *str)
{
#ifndef DISABLE_NOTIFICATIONS
    DUER_LOGI("notification: %s\n", str);
#endif
}

#ifdef OVERRIDE_SPEEX_ALLOC
void *speex_alloc (int size)
{
   return DUER_CALLOC(size, 1);
}
#endif

#ifdef OVERRIDE_SPEEX_ALLOC_SCRATCH
void *speex_alloc_scratch (int size)
{
   return DUER_CALLOC(size, 1);
}
#endif

#ifdef OVERRIDE_SPEEX_REALLOC
void *speex_realloc (void *ptr, int size)
{
   return DUER_REALLOC(ptr, size);
}
#endif

#ifdef OVERRIDE_SPEEX_FREE
void speex_free (void *ptr)
{
   DUER_FREE(ptr);
}
#endif

#ifdef OVERRIDE_SPEEX_FREE_SCRATCH
void speex_free_scratch (void *ptr)
{
   DUER_FREE(ptr);
}
#endif
