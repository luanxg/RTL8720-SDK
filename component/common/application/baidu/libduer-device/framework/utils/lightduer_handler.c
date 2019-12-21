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
// Author: Su Hao(suhao@baidu.com)
//
// Description: A common events handler for internal call in the task.

#ifdef DUER_EVENT_HANDLER

#include "lightduer_handler.h"

#ifndef DUER_HANDLER_QUEUE_LENGTH
#define DUER_HANDLER_QUEUE_LENGTH    (50)
#endif

#ifndef DUER_HANDLER_STACK_SIZE
#define DUER_HANDLER_STACK_SIZE      (1024 * 6)
#endif

static duer_events_handler s_handler = NULL;

void duer_handler_create(void)
{
   if (s_handler == NULL) {
        s_handler = duer_events_create("lightduer_handler",
                                        DUER_HANDLER_STACK_SIZE, DUER_HANDLER_QUEUE_LENGTH);
    }
}

int duer_handler_handle(duer_events_func func, int what, void *object)
{
    if (s_handler) {
        return duer_events_call(s_handler, func, what, object);
    }
    return DUER_ERR_FAILED;
}

void duer_handler_destroy(void)
{
    if (s_handler) {
        duer_events_destroy(s_handler);
        s_handler = NULL;
    }
}

#endif // DUER_EVENT_HANDLER
