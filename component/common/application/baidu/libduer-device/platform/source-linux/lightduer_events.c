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
//
// File: lightduer_events.c
// Auth: Zhang Leliang (zhangleliang@baidu.com)
// Desc: Light duer events looper.

#include "lightduer_events.h"

#include <pthread.h>
#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#include "lightduer_connagent.h"
#include "lightduer_lib.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_priority_conf.h"

static const int TERMINATE_MSG = 0xFF;

typedef struct _duer_queue_message_s {
    duer_events_func              _callback;
    int                           _what;
    void                          *_data;
    struct _duer_queue_message_s  *_pre;
    struct _duer_queue_message_s  *_next;
} duer_eq_message;

typedef struct _duer_events_struct {
    pthread_t        _task;
    pthread_mutex_t  _mutex;
    pthread_cond_t   _cond;
    duer_eq_message *_head;
    duer_eq_message *_tail;
    char             *_name;
    bool             _shutdown;
} duer_events_t;

static unsigned long long duer_events_timestamp()
{
    struct timeval tv;
    int ret = gettimeofday(&tv, NULL);
    if (ret) {
        DUER_LOGE("gettimeofday error");
        return 0;
    }
    unsigned long long time_in_microseconds = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    return time_in_microseconds;
}

static void* duer_events_task(void* context)
{
    duer_events_t *events = (duer_events_t *)(context);
    unsigned long long runtime = 0;
    duer_eq_message *message;
    char *name = NULL;
    const int NAME_LEN = 16;
    DUER_LOGD("duer_events_task");

    if (events == NULL) {
        DUER_LOGE("duer_events_task events is NULL");
        return NULL;
    }

    name = (char *)DUER_MALLOC(NAME_LEN);
    if (name) {
        snprintf(name, NAME_LEN, "%s", events->_name);
        pthread_setname_np(events->_task, name);
        DUER_FREE(name);
        name = NULL;
    }
#if DUER_ENABLE_TASK_PRIORITY
    struct sched_param pri_param = {-1};
    int policy;
    pthread_getschedparam(pthread_self(), &policy, &pri_param);
    DUER_LOGI("duer_events_create: %s, pri:%d, policy:%d", events->_name, pri_param.sched_priority, policy);
#endif

    do {
        pthread_mutex_lock(&events->_mutex);
        while (events->_tail == NULL) {
            pthread_cond_wait(&events->_cond, &events->_mutex);
        }

        message = events->_tail;
        events->_tail = events->_tail->_pre;
        if (events->_tail == NULL) {
            events->_head = NULL;
        }
        pthread_mutex_unlock(&events->_mutex);

        if (message->_callback) {
            runtime = duer_events_timestamp();
            DUER_LOGD("[%s] <== event begin = %p", events->_name, message->_callback);
            message->_callback(message->_what, message->_data);
            runtime = duer_events_timestamp() - runtime;
            if (runtime > 50) {
                DUER_LOGW("[%s] <== event end = %p, timespent = %ld, message:%p", events->_name, message->_callback, runtime, message);
            }
        } else if (events->_shutdown && message->_what == TERMINATE_MSG) {
            pthread_mutex_destroy(&events->_mutex);
            pthread_cond_destroy(&events->_cond);

            if (events->_name) {
                DUER_FREE(events->_name);
                events->_name = NULL;
            }
            DUER_FREE(message);
            DUER_FREE(events);
            break;
        } else {
            //do nothing
        }
        DUER_FREE(message);
        message = NULL;

    } while (1);
    return NULL;
}

duer_events_handler duer_events_create(const char *name, size_t stack_size, size_t queue_length)
{
    duer_events_t *events = NULL;
    int rs = DUER_ERR_FAILED;
    DUER_LOGD("duer_events_create: %s", name);
    do {
        if (name == NULL) {
            name = "default";
        }

        events = (duer_events_t *)DUER_MALLOC(sizeof(duer_events_t)); // ~= 92

        if (!events) {
            DUER_LOGE("[%s] Alloc the events resource failed!", name);
            break;
        }

        DUER_MEMSET(events, 0, sizeof(duer_events_t));

        events->_name = (char *)DUER_MALLOC(DUER_STRLEN(name) + 1); // > 10

        if (events->_name) {
            DUER_MEMCPY(events->_name, name, DUER_STRLEN(name) + 1);
        }

        pthread_mutex_init(&events->_mutex, NULL);
        pthread_cond_init(&events->_cond, NULL);
        events->_head = NULL;
        events->_tail = NULL;

        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (stack_size > 0) {
            pthread_attr_setstacksize(&attr, stack_size);
        }
#if DUER_ENABLE_TASK_PRIORITY // enable this, execution should have the root permission
        struct sched_param pri_param;
        pthread_attr_getschedparam (&attr, &pri_param);
        if (DUER_STRNCMP(name, "lightduer_ca", DUER_STRLEN(name)) == 0) {
            DUER_LOGI("pri min:%d, max:%d", sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO));

            pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
            pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
            pri_param.sched_priority = duer_priority_get(duer_priority_get_task_id(name));
            pthread_attr_setschedparam(&attr, &pri_param);
        }
        if (DUER_STRNCMP(name, "lightduer_handler", DUER_STRLEN(name)) == 0) {
            DUER_LOGI("pri min:%d, max:%d", sched_get_priority_min(SCHED_FIFO), sched_get_priority_max(SCHED_FIFO));

            pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
            pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
            pri_param.sched_priority = duer_priority_get(duer_priority_get_task_id(name));
            pthread_attr_setschedparam(&attr, &pri_param);
        }

        DUER_LOGI("duer_events_create: %s, pri:%d", name, pri_param.sched_priority);
#endif
        int status = pthread_create(&events->_task, &attr, duer_events_task, (void*)events);
        pthread_attr_destroy(&attr);
        if (status) {
            DUER_LOGE("pthread_create failed! status:%d,%d", status, EINVAL);
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
        if (events == NULL) {
            break;
        }
        DUER_LOGD("duer_events_call, name:%s, what:%d, obj:%p", events->_name, what, object);
        duer_eq_message *message = DUER_MALLOC(sizeof(duer_eq_message)); // ~= 20

        if (message == NULL) {
            DUER_LOGE("malloc duer_eq_message fail!!");
            break;
        }
        message->_callback = func;
        message->_what = what;
        message->_data = object;
        message->_pre = NULL;

        pthread_mutex_lock(&events->_mutex);
        if (!events->_shutdown || what == TERMINATE_MSG) {
            if (events->_head) {
                events->_head->_pre = message;
            }
            message->_next = events->_head;
            events->_head = message;
            if (events->_tail == NULL) {
                events->_tail = message;
            }
        } else {
            DUER_FREE(message);
            pthread_mutex_unlock(&events->_mutex);
            break;
        }

        if (events->_tail == message) {
            pthread_cond_signal(&events->_cond);
        }
        pthread_mutex_unlock(&events->_mutex);

        rs = DUER_OK;
    } while (0);

    if (rs < 0 && events != NULL && events->_name != NULL) {
        DUER_LOGE("[%s,%d] Queue send failed!", events->_name, events->_shutdown);
    }

    return rs;
}

void duer_events_destroy(duer_events_handler handler)
{
    duer_events_t *events = (duer_events_t *)handler;

    if (events) {
        if (!events->_shutdown) {
            pthread_mutex_lock(&events->_mutex);
            events->_shutdown = true;
            pthread_mutex_unlock(&events->_mutex);
        } else {
            DUER_LOGI("already destroy!");
            return;
        }

        duer_events_call(events, NULL, TERMINATE_MSG, NULL);
    }
}
