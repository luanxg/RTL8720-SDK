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
 * File: lightduer_events.c
 * Auth: Chen Hanwen (chenhanwen@baidu.com)
 * Desc: Adapt the Message function to LPT230 VER1.0
 * babeband:RDA5981A:total internal sram:352KB,flash size:1MB,system:mbed
 */

#include "lightduer_events.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_memory.h"
#include "lightduer_priority_conf.h"

#include "hsf.h"
#include "hfmsgq.h"

#define QUEUE_WAIT_FOREVER  (0x7fffffff)

typedef struct _duer_events_struct {
    hfthread_hande_t task;
    HFMSGQ_HANDLE   queue;
    char*           name;
} duer_events_t;

typedef struct _duer_queue_message_s {
    duer_events_func    callback;
    int                 what;
    void*               data;
} duer_eq_message;

static long duer_events_timestamp() {
    return hfsys_get_time();
}

static void duer_events_task(void* context) {
    duer_events_t* events = (duer_events_t*)context;
    long runtime = 0;
    duer_eq_message message;

    do {
        if (HF_SUCCESS == hfmsgq_recv(events->queue, &message, QUEUE_WAIT_FOREVER, 0)) {
            if (message.callback) {
                runtime = duer_events_timestamp();
                DUER_LOGD("[%s] <== event begin = %p", events->name, message.callback);
                message.callback(message.what, message.data);
                runtime = duer_events_timestamp() - runtime;

                if (runtime > 50) {
                    DUER_LOGW("[%s] <== event end = %p", events->name, message.callback);
                }
            }
        } else {
            DUER_LOGE("[%s] Queue receive failed!", events->name);
        }
    } while (1);
}

duer_events_handler duer_events_create(const char* name, size_t stack_size, size_t queue_length) {
    duer_events_t* events = NULL;
    int rs = DUER_ERR_FAILED;

    do {
        if (name == NULL) {
            name = "default";
        }

        events = (duer_events_t*)DUER_MALLOC(sizeof(duer_events_t));

        if (!events) {
            DUER_LOGE("[%s] Alloc the events resource failed!", name);
            break;
        }

        DUER_MEMSET(events, 0, sizeof(duer_events_t));

        events->name = (char*)DUER_MALLOC(DUER_STRLEN(name) + 1);

        if (events->name) {
            DUER_MEMCPY(events->name, name, DUER_STRLEN(name) + 1);
        }

        events->queue = hfmsgq_create(queue_length, sizeof(duer_eq_message));

        if (events->queue == NULL) {
            DUER_LOGE("[%s] Create the queue failed!", events->name);
            break;
        }

        /* for lpb100, lightduer_ca task stack size is 4K */
        //if (strncmp(name, "lightduer_ca", strlen("lightduer_ca")) == 0) {
        //stack_size = 4096;
        //}
        hfthread_create(&duer_events_task, name, stack_size / 4, events,
                        duer_priority_get(duer_priority_get_task_id(name)), &(events->task), NULL);

        if (events->task == NULL) {
            DUER_LOGE("[%s] Create the task failed!", events->name);
            break;
        }

        rs = DUER_OK;
    } while (0);

    if (rs != DUER_OK) {
        duer_events_destroy(events);
        events = NULL;
    }

    return events;
}

int duer_events_call(duer_events_handler handler, duer_events_func func, int what, void* object) {
    int rs = DUER_ERR_FAILED;
    duer_events_t* events = (duer_events_t*)handler;

    do {
        if (events == NULL || events->queue == NULL) {
            break;
        }

        duer_eq_message message;
        message.callback = func;
        message.what = what;
        message.data = object;

        if (HF_SUCCESS != hfmsgq_send(events->queue, &message, 0, 0)) {
            break;
        }

        rs = DUER_OK;
    } while (0);

    if (rs < 0 && events != NULL && events->name != NULL) {
        DUER_LOGE("[%s] Queue send failed!", events->name);
    }

    return rs;
}

void duer_events_destroy(duer_events_handler handler) {
    duer_events_t* events = (duer_events_t*)handler;

    if (events) {
        if (events->task) {
            hfthread_destroy(events->task);
            events->task = NULL;
        }

        if (events->queue) {
            hfmsgq_destroy(events->queue);
            events->queue = NULL;
        }

        if (events->name) {
            DUER_FREE(events->name);
            events->name = NULL;
        }

        DUER_FREE(events);
    }
}
