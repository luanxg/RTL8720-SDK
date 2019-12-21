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
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Light duer events looper.
 */

#include "lightduer_events.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_memory.h"
#include "lightduer_mutex.h"
#include "lightduer_priority_conf.h"

//Edit by Realtek
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#if defined(DUER_PLATFORM_MARVELL)
#define QueueHandle_t       xQueueHandle
#define TaskHandle_t        xTaskHandle
#endif

#ifdef DUER_PLATFORM_ESP8266
#include "adaptation.h"
#endif

#define QUEUE_WAIT_FOREVER  (0x7fffffff)

static const int TERMINATE_MSG = 0xff;

typedef struct _duer_events_struct {
    TaskHandle_t    _task;
    QueueHandle_t   _queue;
    char *          _name;
    int             _terminate;
    duer_mutex_t    _terminate_mutex;
} duer_events_t;

typedef struct _duer_queue_message_s {
    duer_events_func    _callback;
    int                 _what;
    void *              _data;
} duer_eq_message;

static long duer_events_timestamp()
{
    return xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
}

static void duer_events_task(void *context)
{
    duer_events_t *events = (duer_events_t *)context;
    long runtime = 0;
    duer_eq_message message;

    do {
        if (pdTRUE == xQueueReceive(events->_queue, &message, QUEUE_WAIT_FOREVER)) {
            if (message._callback) {
                runtime = duer_events_timestamp();
                DUER_LOGD("[%s] <== event begin = %p", events->_name, message._callback);
                message._callback(message._what, message._data);
                runtime = duer_events_timestamp() - runtime;
                if (runtime > 50) {
                    DUER_LOGW("[%s] <== event end = %p, timespent = %ld", events->_name, message._callback, runtime);
                }
            } else if (message._what == TERMINATE_MSG) {
                DUER_LOGI("terminate task: %s", events->_name);
                break;
            }
        } else {
            DUER_LOGE("[%s] Queue receive failed!", events->_name);
        }
    } while (1);

    if (events->_queue) {
        vQueueDelete(events->_queue);
        events->_queue = NULL;
    }

    if (events->_name) {
        DUER_FREE(events->_name);
        events->_name = NULL;
    }

    if (events->_terminate_mutex) {
        duer_mutex_destroy(events->_terminate_mutex);
        events->_terminate_mutex = NULL;
    }

    DUER_FREE(events);
    events = NULL;

    vTaskDelete(NULL);
}

duer_events_handler duer_events_create(const char *name, size_t stack_size, size_t queue_length)
{
    return duer_events_create_with_priority(name, stack_size, queue_length,
            duer_priority_get(duer_priority_get_task_id(name)));
}

duer_events_handler duer_events_create_with_priority(const char *name, size_t stack_size,
                                                     size_t queue_length, int priority)
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

        events->_queue = xQueueCreate(queue_length, sizeof(duer_eq_message));
        if (events->_queue == NULL) {
            DUER_LOGE("[%s] Create the queue failed!", events->_name);
            break;
        }

        events->_terminate = 0;
        events->_terminate_mutex = duer_mutex_create();
        if (events->_terminate_mutex == NULL) {
            DUER_LOGE("[%s] Create the mutex failed!", events->_name);
            break;
        }

        xTaskCreate(&duer_events_task, name, stack_size / sizeof(portSTACK_TYPE),
                    events, priority, &(events->_task));
        if (events->_task == NULL) {
            DUER_LOGE("[%s] Create the queue failed!", events->_name);
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
//Add by Realtek
//begin
#if defined(DUER_PLATFORM_RTKAMEBA)
static int inHandlerMode (void)
{

#if defined ( __GNUC__ )
	uint32_t result;
	__asm volatile ("MRS %0, ipsr" : "=r" (result) );
	return(result) != 0;
#else
    return __get_IPSR() != 0;
#endif
}

BaseType_t duer_osMailPut (QueueHandle_t handle, void *mail)
{
    portBASE_TYPE taskWoken = pdFALSE;

    if (inHandlerMode()) {
        if (xQueueSendFromISR(handle, mail, &taskWoken) != pdTRUE) {
            return pdFALSE;
        }
        portEND_SWITCHING_ISR(taskWoken);
    }
    else {
        if (xQueueSend(handle, mail, 0) != pdTRUE) {
            return pdFALSE;
        }
    }

    return pdTRUE;
}
#endif

int duer_events_call(duer_events_handler handler, duer_events_func func, int what, void *object)
{
    int rs = DUER_ERR_FAILED;
    duer_events_t *events = (duer_events_t *)handler;

    do {
        if (events == NULL || events->_queue == NULL) {
            break;
        }

        duer_mutex_lock(events->_terminate_mutex);
        if (events->_terminate) {
            duer_mutex_unlock(events->_terminate_mutex);
            break;
        }

        if (func == NULL && what == TERMINATE_MSG) {
            events->_terminate = 1;
        }
        duer_mutex_unlock(events->_terminate_mutex);

        duer_eq_message message;
        message._callback = func;
        message._what = what;
        message._data = object;
//Edit by Realtek
#if defined(DUER_PLATFORM_RTKAMEBA)
	if (pdTRUE != duer_osMailPut(events->_queue, &message)) {
            break;
        }
#else
        if (pdTRUE != xQueueSend(events->_queue, &message, 0)) {
            break;
        }
#endif

        rs = DUER_OK;
    } while (0);

    if (rs < 0 && events != NULL && events->_name != NULL) {
        DUER_LOGE("[%s] Queue send failed!", events->_name);
    }

    return rs;
}

void duer_events_destroy(duer_events_handler handler)
{
    duer_events_call(handler, NULL, TERMINATE_MSG, NULL);
}
