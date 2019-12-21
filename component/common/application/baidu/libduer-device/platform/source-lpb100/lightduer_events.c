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
 * Auth: Chang Li (changli@baidu.com)
 * Desc: Light duer events looper.
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
    hfthread_hande_t _task;
    HFMSGQ_HANDLE   _queue;
    char *          _name;
} duer_events_t;

typedef struct _duer_queue_message_s {
    duer_events_func    _callback;
    int                 _what;
    void *              _data;
} duer_eq_message;

static long duer_events_timestamp()
{
    return hfsys_get_time();
}

static void duer_events_task(void *context)
{
    duer_events_t *events = (duer_events_t *)context;
    long runtime = 0;
    duer_eq_message message;

    do {
        if (HF_SUCCESS == hfmsgq_recv(events->_queue, &message, QUEUE_WAIT_FOREVER, 0)) {
            if (message._callback) {
                runtime = duer_events_timestamp();
                DUER_LOGD("[%s] <== event begin = %p", events->_name, message._callback);
                message._callback(message._what, message._data);
                runtime = duer_events_timestamp() - runtime;
                if (runtime > 50) {
                    DUER_LOGW("[%s] <== event end = %p, timespent = %ld", events->_name, message._callback, runtime);
                }
            }
        } else {
            DUER_LOGE("[%s] Queue receive failed!", events->_name);
        }
    } while (1);
}

duer_events_handler duer_events_create(const char *name, size_t stack_size, size_t queue_length)
{
    duer_events_t *events = NULL;
    int rs = DUER_ERR_FAILED;

    do {
        if (name == NULL) {
            name = "default";
        }

        events = (duer_events_t *)DUER_MALLOC(sizeof(duer_events_t));
        if (!events) {
            DUER_LOGE("[%s] Alloc the events resource failed!", name);
            break;
        }

        DUER_MEMSET(events, 0, sizeof(duer_events_t));

        events->_name = (char *)DUER_MALLOC(DUER_STRLEN(name) + 1);
        if (events->_name) {
            DUER_MEMCPY(events->_name, name, DUER_STRLEN(name) + 1);
        }

        events->_queue = hfmsgq_create(queue_length, sizeof(duer_eq_message));
        if (events->_queue == NULL) {
            DUER_LOGE("[%s] Create the queue failed!", events->_name);
            break;
        }

        /* for lpb100, lightduer_ca task stack size is 4K */
        if (strncmp(name, "lightduer_ca", strlen("lightduer_ca")) == 0) {
            stack_size = 4096;
        }

        hfthread_create(&duer_events_task, name, stack_size / 4, events, duer_priority_get(duer_priority_get_task_id(name)), &(events->_task), NULL);
        if (events->_task == NULL) {
            DUER_LOGE("[%s] Create the task failed!", events->_name);
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

int duer_events_call(duer_events_handler handler, duer_events_func func, int what, void *object)
{
    int rs = DUER_ERR_FAILED;
    duer_events_t *events = (duer_events_t *)handler;

    do {
        if (events == NULL || events->_queue == NULL) {
            break;
        }

        duer_eq_message message;
        message._callback = func;
        message._what = what;
        message._data = object;

        if (HF_SUCCESS != hfmsgq_send(events->_queue, &message, 0, 0)) {
            break;
        }

        rs = DUER_OK;
    } while (0);

    if (rs < 0 && events != NULL && events->_name != NULL) {
        DUER_LOGE("[%s] Queue send failed!", events->_name);
    }

    return rs;
}

void duer_events_destroy(duer_events_handler handler)
{
    duer_events_t *events = (duer_events_t *)handler;

    if (events) {
        if (events->_task) {
            hfthread_destroy(events->_task);
            events->_task = NULL;
        }

        if (events->_queue) {
            hfmsgq_destroy(events->_queue);
            events->_queue = NULL;
        }

        if (events->_name) {
            DUER_FREE(events->_name);
            events->_name = NULL;
        }

        DUER_FREE(events);
    }
}
