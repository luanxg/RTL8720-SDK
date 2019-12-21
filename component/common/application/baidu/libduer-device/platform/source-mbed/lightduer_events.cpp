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

#include <mbed.h>
#include "lightduer_events.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_memory.h"
#include "lightduer_timestamp.h"
#include "lightduer_priority_conf.h"
#include "heap_monitor.h"

static const int TERMINATE_MSG = 0xff;

typedef struct _duer_queue_message_s {
    duer_events_func    _callback;
    int                 _what;
    void *              _data;
} duer_eq_message;

class DuerHandler
{
public:

    DuerHandler(const char *name, size_t stack_size)
            : _thread(osPriorityNormal, stack_size)
            , _terminate(false)
            , _destory_timer(NULL)
    {
        if (name == NULL) {
            name = "default";
        }

        _name = NEW(CA) char[strlen(name) + 1];
        if (_name) {
            DUER_MEMCPY(_name, name, strlen(name) + 1);
        }

        _thread.start(this, &DuerHandler::loop);
    }

    ~DuerHandler()
    {
        if (_name) {
            delete [] _name;
            _name = NULL;
        }
        if (_destory_timer) {
            delete _destory_timer;
            _destory_timer = NULL;
        }
    }

    int post(duer_events_func func, int what, void *object)
    {
        int rs = DUER_ERR_FAILED;

        do {
            _terminate_mutex.lock();
            if (_terminate) {
                _terminate_mutex.unlock();
                break;
            }

            if (func == NULL && what == TERMINATE_MSG) {
                _terminate = true;
            }
            _terminate_mutex.unlock();

            duer_eq_message *message = (duer_eq_message *)DUER_MALLOC(sizeof(duer_eq_message));
            message->_callback = func;
            message->_what = what;
            message->_data = object;

            DUER_LOGV("post event: %p ==> %p(%d, %p)", message, func, what, object);

            rs = _queue.put(message, 1);
            if (osOK != rs) {
                DUER_LOGE("Queue send failed: %d, name:%s", rs, _name);
                DUER_FREE(message);
                rs = DUER_ERR_FAILED;
                break;
            }

            rs = DUER_OK;
        } while (0);

        return rs;
    }

    void loop()
    {
        long runtime = 0;
        duer_eq_message *message;
        osEvent evt;

        do {
            evt = _queue.get();

            if (evt.status != osEventMessage) {
                DUER_LOGE("[%s] Queue receive failed! evt.status: %d", _name, evt.status);
                continue;
            }

            message = reinterpret_cast<duer_eq_message *>(evt.value.p);

            if (message && message->_callback) {
                DUER_LOGV("recv event: %p <== %p(%d, %p)", message, message->_callback, message->_what, message->_data);
                DUER_LOGV("stack size: %d, free: %d, used: %d, max: %d",
                          _thread.stack_size(), _thread.free_stack(),
                          _thread.used_stack(), _thread.max_stack());
                runtime = duer_timestamp();
                DUER_LOGD("[%s] <== event begin = %p", _name, message->_callback);
                message->_callback(message->_what, message->_data);
                runtime = duer_timestamp() - runtime;
                if (runtime > 50) {
                    DUER_LOGW("[%s] <== event end = %p, timespent = %ld", _name, message->_callback, runtime);
                }
                DUER_LOGV("processed: %p <== %p(%d, %p)", message, message->_callback, message->_what, message->_data);
            } else if (message && message->_what == TERMINATE_MSG) {
                DUER_LOGI("terminate thread: %s", _name);
                DUER_FREE(message);
                break;
            }

            DUER_FREE(message);
        } while (1);

        _destory_timer = NEW(CA) rtos::RtosTimer(
                mbed::Callback<void ()>(this, &DuerHandler::delete_self), osTimerOnce);
        _destory_timer->start(10);
    }

    void delete_self()
    {
        delete this;
    }

private:

    rtos::Thread                    _thread;
    rtos::Queue<duer_eq_message, 20>    _queue;
    char *                          _name;
    bool                            _terminate;
    rtos::Mutex                     _terminate_mutex;
    rtos::RtosTimer *               _destory_timer;
};

duer_events_handler duer_events_create(const char *name, size_t stack_size, size_t queue_length)
{
    return NEW (CA) DuerHandler(name, stack_size);
}

int duer_events_call(duer_events_handler handler, duer_events_func func, int what, void *object)
{
    int rs = DUER_ERR_FAILED;
    DuerHandler *events = (DuerHandler *)handler;

    do {
        if (events == NULL) {
            break;
        }

        rs = events->post(func, what, object);
    } while (0);

    return rs;
}

void duer_events_destroy(duer_events_handler handler)
{
    duer_events_call(handler, NULL, TERMINATE_MSG, NULL);
}
