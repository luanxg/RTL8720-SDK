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
 * File: lightduer.c
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Light duer APIS.
 */

#include <inttypes.h>
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_adapter.h"
#include "lightduer_memory.h"
#include "lightduer_events.h"
#include "lightduer_engine.h"
#include "lightduer_ds_report.h"
#include "lightduer_ds_report_ca.h"
#include "lightduer_ds_log_ca.h"
#include "lightduer_ds_log_cache.h"
#include "lightduer_timestamp.h"
#include "lightduer_event_emitter.h"
#include "lightduer_random.h"
#include "lightduer_mutex.h"
#ifdef DUER_EVENT_HANDLER
#include "lightduer_handler.h"
#endif // DUER_EVENT_HANDLER

extern void duer_voice_initialize(void);
extern void duer_voice_finalize(void);
extern int duer_voice_terminate(void);
extern int duer_data_send(void);
extern int duer_engine_enqueue_response(
        const duer_msg_t *req, int msg_code, const void *data, duer_size_t size);
extern int duer_engine_enqueue_seperate_response(
        const char *token, duer_size_t token_len, int msg_code, const baidu_json *data);
extern void duer_engine_send_directly(int what, void *object);
extern void duer_session_initialize(void);
extern void duer_session_finalize(void);

static duer_event_callback g_event_callback = NULL;

#ifndef DISABLE_SPLIT_SEND
static volatile duer_u32_t s_split_id;
//used to keep the s_split_id is not negative when adding to json as int
static const int SPLIT_ID_MASK = 0x7fffffff;
static duer_mutex_t s_split_mutex;
#endif

// begin device status report
static duer_u32_t s_connect_start_time = 0;
static duer_u32_t s_connect_end_time = 0;
static duer_u32_t s_connect_count = 0;
// end device status report

// MAX num message in CA queue
#ifndef DUER_MAX_MSG_NUM_IN_CA_QUEUE
#define DUER_MAX_MSG_NUM_IN_CA_QUEUE 20
#endif //DUER_MAX_MSG_NUM_IN_CA_QUEUE

// begin whether clear the message queue
#ifndef MAX_INTERVAL_KEEP_CA_QUEUE
#define MAX_INTERVAL_KEEP_CA_QUEUE 4000 // ms
#endif // MAX_INTERVAL_KEEP_CA_QUEUE
// begin whether clear the voice data
#ifndef MAX_INTERVAL_KEEP_VOICE_QUEUE
#define MAX_INTERVAL_KEEP_VOICE_QUEUE 1900 //ms when interval > 2s,the server will stop immediately.
#endif // MAX_INTERVAL_KEEP_VOICE_QUEUE
static duer_u32_t s_first_stopped_time = 0;

#ifdef DUER_BJSON_PRINT_WITH_ESTIMATED_SIZE
#define DUER_PRINT_BJSON_ESTIMATED_SIZE_FOR_REPORT_DATA (1024)
#endif

// invoke this after CA connected
static duer_status_t duer_ca_try_clear_queue()
{
    duer_u32_t current_time = duer_timestamp();
    if (s_first_stopped_time > 0
            && (current_time < s_first_stopped_time
                    || current_time - s_first_stopped_time > MAX_INTERVAL_KEEP_CA_QUEUE)) {
        duer_clear_data(duer_timestamp());
        duer_ds_log_ca_info(DUER_DS_LOG_CA_CLEAR_DATA, NULL);
    }
#if !defined(__TARGET_TRACKER__)
    if (s_first_stopped_time > 0
            && (current_time < s_first_stopped_time
                    || current_time - s_first_stopped_time > MAX_INTERVAL_KEEP_VOICE_QUEUE)) {
        duer_voice_terminate();
    }
#endif
    s_first_stopped_time = 0;
    return DUER_OK;
}
// end whether clear the message queue

static char* g_event_name[] = {
    "DUER_EVT_CREATE",
    "DUER_EVT_START",
    "DUER_EVT_ADD_RESOURCES",
    "DUER_EVT_SEND_DATA",
    "DUER_EVT_DATA_AVAILABLE",
    "DUER_EVT_STOP",
    "DUER_EVT_DESTROY",
    "DUER_EVENTS_TOTAL"
};

static void duer_event_notify(int event_id, int event_code, void *object)
{
    if (g_event_callback) {
        duer_event_t event;
        event._event = event_id;
        event._code = event_code;
        event._object = object;
        g_event_callback(&event);
    }
}

static void duer_set_coap_response_callback(void)
{
    //duer_emitter_emit(duer_engine_set_response_callback, 0, duer_coap_response_callback);
    duer_engine_set_response_callback(0, duer_coap_response_callback);
}

#ifndef DISABLE_SPLIT_SEND
static void duer_split_id_initialize()
{
    if (!s_split_mutex) {
        s_split_mutex = duer_mutex_create();
    }

    duer_mutex_lock(s_split_mutex);
    s_split_id = duer_random() & SPLIT_ID_MASK;
    duer_mutex_unlock(s_split_mutex);
}

static void duer_split_id_finalize()
{
    if (s_split_mutex) {
        duer_mutex_destroy(s_split_mutex);
        s_split_mutex = NULL;
    }
}
#endif

static void duer_engine_notify_callback(int event, int status, int what, void *object)
{
    static duer_status_t reason_to_start = DUER_OK;

    if (status != DUER_OK
            && status != DUER_ERR_TRANS_WOULD_BLOCK
            && status != DUER_ERR_CONNECT_TIMEOUT
            && status != DUER_ERR_HAS_STOPPED) {
        DUER_LOGI("=====> event: %d[%s], status: %d", event, g_event_name[event], status);
    }
    switch (event) {
    case DUER_EVT_CREATE:
        duer_session_initialize();
        duer_set_coap_response_callback();
#ifndef DISABLE_SPLIT_SEND
        duer_split_id_initialize();
#endif
        break;
    case DUER_EVT_START:
        if (object) {
            DUER_FREE(object);
        }
        if (status == DUER_OK) {
            duer_ds_log_init();
#if !defined(__TARGET_TRACKER__)
            duer_voice_initialize();
#endif

            DUER_LOGI("connect started!");
            duer_ca_try_clear_queue();

            s_connect_end_time = duer_timestamp();
            duer_u32_t connect_time_cost = (s_connect_end_time - s_connect_start_time);
            duer_ds_log_ca_started(reason_to_start, s_connect_count, connect_time_cost);
            s_connect_count = 0;
            reason_to_start = DUER_OK;

            duer_ds_log_cache_report();

            duer_event_notify(DUER_EVENT_STARTED, status, NULL);

            if (duer_engine_qcache_length() > 0) {
                duer_data_send();
            }
            duer_ds_report_start(DUER_DS_PERIODIC, 0);// use the default interval

        } else if (status == DUER_ERR_TRANS_WOULD_BLOCK) {
            DUER_LOGI("will start latter(DUER_ERR_TRANS_WOULD_BLOCK)");
        } else if (status == DUER_ERR_CONNECT_TIMEOUT) {
            int rs = duer_emitter_emit(duer_engine_start, 0, NULL);
            if (rs != DUER_OK) {
                DUER_LOGW("retry connect fail!!");
            }
        } else if (status == DUER_ERR_HAS_STARTED) {
            DUER_LOGW("Duer CA engine has started!");
        } else {
            DUER_LOGE("start fail! status:%d", status);
        }
        break;
    case DUER_EVT_ADD_RESOURCES:
        if (status == DUER_OK) {
            DUER_LOGI("add resource successfully!!");
        }
        if (object) {
            duer_res_t *resources = (duer_res_t *)object;
            for (int i = 0; i < what; ++i) {
                if (resources[i].path) {
                    DUER_FREE(resources[i].path);// more info in duer_add_resources
                    resources[i].path = NULL;
                }
            }
            DUER_FREE(object);
        }
        break;
    case DUER_EVT_SEND_DATA:
        if (object) {
            DUER_LOGI("data sent : %s", (const char *)object);
            baidu_json_release(object);
        }
        //DUER_LOGI("====status:%d, len:%d=", status, duer_engine_qcache_length());
        if (status >= 0 ) {
            if (duer_engine_qcache_length() > 0) {
                DUER_LOGD("###QCACHE_LEN====%d.", duer_engine_qcache_length());
                duer_data_send();
            }
        } else if (status == DUER_ERR_TRANS_WOULD_BLOCK) {
            DUER_LOGD("!!!Send WOULD_BLOCK");
        } else {
            //do nothing
        }
        break;
    case DUER_EVT_DATA_AVAILABLE:
        break;
    case DUER_EVT_STOP:
        if (status == DUER_ERR_TRANS_TIMEOUT) {
            duer_clear_data(duer_timestamp());
#if !defined(__TARGET_TRACKER__)
            duer_voice_terminate();
#endif
            duer_ds_log_ca_info(DUER_DS_LOG_CA_CLEAR_DATA, NULL);
        } else {
            if (s_first_stopped_time == 0) {
                s_first_stopped_time = duer_timestamp();
            }
        }
        duer_ds_log_ca_debug(DUER_DS_LOG_CA_STOPPED, NULL);
        DUER_LOGI("connect stopped, status:%d!!", status);
#if !defined(__TARGET_TRACKER__)
        duer_voice_finalize();
#endif

        duer_event_notify(DUER_EVENT_STOPPED, status, NULL);
        break;
    case DUER_EVT_DESTROY:
        duer_ds_report_destroy();
        duer_session_finalize();
        duer_split_id_finalize();
        duer_emitter_destroy();
        duer_ds_log_cache_finalize();
        baidu_ca_adapter_finalize();
#ifdef DUER_EVENT_HANDLER
        duer_handler_destroy();
#endif // DUER_EVENT_HANDLER
        break;
    default:
        DUER_LOGW("unknown event!!");
        break;
    }

    if (event < DUER_EVT_STOP
            && status != DUER_OK
            && status != DUER_ERR_TRANS_WOULD_BLOCK
            && status != DUER_ERR_CONNECT_TIMEOUT
            && status != DUER_ERR_HAS_STARTED
            && status != DUER_ERR_HAS_STOPPED) {
        duer_ds_report_stop();
        if (reason_to_start == DUER_OK) { // only record the first stop reason
            reason_to_start = status;
        }
        DUER_LOGE("Action failed: event: %d, status: %d", event, status);
        duer_engine_stop(status, NULL);
    }
}

void duer_initialize()
{
    baidu_ca_adapter_initialize();

    duer_emitter_create();

#ifdef DUER_EVENT_HANDLER
    duer_handler_create();
#endif // DUER_EVENT_HANDLER

    duer_engine_register_notify(duer_engine_notify_callback);

    int res = duer_emitter_emit(duer_engine_create, 0, NULL);
    if (res != DUER_OK) {
        DUER_LOGE("duer_initialize fail! res:%d", res);
    }

    int ret = duer_ds_register_report_function(duer_ds_report_ca_status);
    if (ret != DUER_OK) {
        DUER_LOGE("report ca status register failed ret: %d", ret);
    }

    duer_ds_log_cache_initialize();

    duer_ds_log_ca_debug(DUER_DS_LOG_CA_INIT, NULL);
}

void duer_set_event_callback(duer_event_callback callback)
{
    if (g_event_callback != callback) {
        g_event_callback = callback;
    }
}

int duer_start(const void *data, size_t size)
{
    if (data == NULL || size == 0) {
        DUER_LOGE("duer_start invalid arguments");
        return DUER_ERR_INVALID_PARAMETER;
    }

    duer_ds_log_ca_debug(DUER_DS_LOG_CA_START, NULL);
    ++s_connect_count;

    void *profile = DUER_MALLOC(size); // ~= 1439

    if (profile == NULL) {
        DUER_LOGE("memory alloc fail!");
        DUER_DS_LOG_CA_MEMORY_OVERFLOW();
        return DUER_ERR_MEMORY_OVERLOW;
    }
    DUER_MEMCPY(profile, data, size);
    DUER_LOGD("use the profile to start engine");

    int rs = duer_emitter_emit(duer_engine_start, (int)size, profile);

    if (rs != DUER_OK) {
        DUER_LOGE("emit duer_engine_start failed!");
        DUER_FREE(profile);
    }
    s_connect_start_time = duer_timestamp();
    return rs;
}

int duer_add_resources(const duer_res_t *res, size_t length)
{
    if (duer_engine_is_started() != DUER_TRUE) {
        DUER_LOGW("please add control point after the CA started");
        return DUER_ERR_FAILED;
    }
    if (length == 0 || res == NULL) {
        DUER_LOGW("no resource will be added, length is 0");
        return DUER_ERR_FAILED;
    }

    duer_bool malloc_failed = DUER_FALSE;
    duer_res_t *list = (duer_res_t *)DUER_MALLOC(length * sizeof(duer_res_t)); // ~= length * 16

    if (list == NULL) {
        malloc_failed = DUER_TRUE;
        goto malloc_fail;
    }
    DUER_MEMSET(list, 0, length * sizeof(duer_res_t));
    for (int i = 0; i < length; ++i) {
        list[i].mode = res[i].mode;
        list[i].allowed = res[i].allowed;
        size_t path_len = DUER_STRLEN(res[i].path);
        if (list[i].mode == DUER_RES_MODE_STATIC) {
            // malloc the path&static data once, Note only need free once
            list[i].path = DUER_MALLOC(path_len + res[i].res.s_res.size + 1);
            if (list[i].path == NULL) {
                malloc_failed = DUER_TRUE;
                goto malloc_fail;
            }
            DUER_MEMCPY(list[i].path, res[i].path, path_len);
            list[i].path[path_len] = '\0';
            list[i].res.s_res.size = res[i].res.s_res.size;
            list[i].res.s_res.data = list[i].path + 1 + path_len;
            DUER_MEMCPY(list[i].res.s_res.data, res[i].res.s_res.data, list[i].res.s_res.size);
            duer_ds_log_ca_register_cp(list[i].path, DUER_CA_CP_TYPE_STATIC);
        } else {
            list[i].path = DUER_MALLOC(path_len + 1);

            if (list[i].path == NULL) {
                malloc_failed = DUER_TRUE;
                goto malloc_fail;
            }
            DUER_MEMCPY(list[i].path, res[i].path, path_len);
            list[i].path[path_len] = '\0';
            list[i].res.f_res = res[i].res.f_res;
            duer_ds_log_ca_register_cp(list[i].path, DUER_CA_CP_TYPE_DYNAMIC);
        }
    }

    if (malloc_failed == DUER_FALSE) {
        int rs = duer_emitter_emit(duer_engine_add_resources, (int)length, (void *)list);
        if (rs == DUER_OK) {
            return DUER_OK;
        } else {
            DUER_LOGW("add resource fail!!");
            goto add_fail;
        }
    }
malloc_fail:
    DUER_LOGW("memory malloc fail!!");
    DUER_DS_LOG_CA_MEMORY_OVERFLOW();
add_fail:
    if (list) {
        for (int i = 0; i < length; ++i) {
            if (list[i].path) {
                DUER_FREE(list[i].path);
                list[i].path = NULL;
            }
        }
        DUER_FREE(list);
        list = NULL;
    }
    return DUER_ERR_FAILED;
}

int duer_data_send(void)
{
    //DUER_LOGI("===%s===", __func__);
    return duer_emitter_emit(duer_engine_send, 0, NULL);
}

static int duer_data_real_send(void)
{
    //DUER_LOGI("===%s===", __func__);
    return duer_emitter_emit(duer_engine_send_directly, 0, NULL);
}

static int duer_after_enqueue_handler(int rs)
{
    if (rs > 0) {
        if (rs == 1) {
            rs = duer_data_send();
        } else if (rs > 10) {
            DUER_LOGW("cached more than 10 message!!");
            rs = duer_data_real_send();
        } else {
            rs = DUER_OK;
        }
    }
    return rs;
}

#ifndef DISABLE_SPLIT_SEND
static int duer_copy_split_data(char *des, int size, char *source)
{
    int des_str_len = 0;
    int special_char_count = 0;

    if (!des || !source || size <= 0) {
        DUER_LOGE("Invalid param");
        return 0;
    }

    while (source[des_str_len] != '\0') {
        if ((source[des_str_len] == '\"') || (source[des_str_len] == '\\')) {
            special_char_count++;
        }
        if (des_str_len + special_char_count + 1 >= size - 1) {
            break;
        }
        des[des_str_len] = source[des_str_len];
        des_str_len++;
    }

    des[des_str_len] = '\0';

    return des_str_len;
}

static int duer_data_get_split_id()
{
    duer_u32_t ret = 0;

    duer_mutex_lock(s_split_mutex);
    s_split_id = (s_split_id + 1) & SPLIT_ID_MASK;
    ret = s_split_id;
    duer_mutex_unlock(s_split_mutex);
    return ret;
}
#endif

int duer_data_report_async(duer_context_t *context, const baidu_json *data)
{
#ifdef DISABLE_SPLIT_SEND
    int rs = duer_engine_enqueue_report_data(context, data);
    //DUER_LOGI("===%s===, context:%p, rs:%d", __func__, context, rs);
    return duer_after_enqueue_handler(rs);
#else
    int rs = DUER_OK;
    int block = 0;
    int content_len = 0;
    int send_len = 0;
    int remain_len = 0;
    duer_u32_t id = 0;
    const int MAX_SEG_LEN = 980;
    const int MAX_PAYLOAD_LEN = 900;
    char *buf = NULL;
    char *content = NULL;
    baidu_json *value = NULL;
    baidu_json *payload = NULL;

    do {
#ifdef DUER_BJSON_PRINT_WITH_ESTIMATED_SIZE
        content = baidu_json_PrintBuffered(data,
                DUER_PRINT_BJSON_ESTIMATED_SIZE_FOR_REPORT_DATA, 0);
#else
        content = baidu_json_PrintUnformatted(data);
#endif // DUER_BJSON_PRINT_WITH_ESTIMATED_SIZE
        id = duer_data_get_split_id();
        if (!content) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            break;
        }
        content_len = DUER_STRLEN(content);
        if (content_len <= MAX_SEG_LEN) {
            baidu_json_release(content);
            content = NULL;
            rs = duer_engine_enqueue_report_data(context, data);
            //DUER_LOGI("===%s===, context:%p, rs:%d", __func__, context, rs);
            rs = duer_after_enqueue_handler(rs);
            break;
        }

        DUER_LOGD("split send data, %d", content_len);

        buf = DUER_MALLOC(MAX_PAYLOAD_LEN + 1); // ~= 901
        if (!buf) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            break;
        }

        value = baidu_json_CreateObject();
        if (!value) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            break;
        }

        payload = baidu_json_CreateObject();
        if (!payload) {
            rs = DUER_ERR_MEMORY_OVERLOW;
            break;
        }
        baidu_json_AddItemToObjectCS(value, "duer_blockwise", payload);
        baidu_json_AddNumberToObjectCS(payload, "id", (int)id);
        baidu_json_AddNumberToObjectCS(payload, "eof", 0);
        remain_len = content_len;

        while (remain_len > 0) {
            baidu_json_DeleteItemFromObject(payload, "block");
            baidu_json_AddNumberToObjectCS(payload, "block", block++);
            send_len = duer_copy_split_data(buf,
                                            MAX_PAYLOAD_LEN + 1,
                                            content + content_len - remain_len);
            if (send_len == 0) {
                rs = DUER_ERR_FAILED;
                break;
            }

            DUER_LOGD("remain_len:%d, block:%d, send_len:%d", remain_len, block, send_len);

            remain_len -= send_len;
            if (remain_len == 0) {
                baidu_json_DeleteItemFromObject(payload, "eof");
                baidu_json_AddNumberToObjectCS(payload, "eof", 1);
            }
            baidu_json_DeleteItemFromObject(payload, "block_data");
            baidu_json_AddStringToObjectCS(payload, "block_data", buf);

            rs = duer_engine_enqueue_report_data(context, value);
            //DUER_LOGI("===%s===, context:%p, rs:%d", __func__, context, rs);
            rs = duer_after_enqueue_handler(rs);
            //sleep needed if the invoke thread has the same priority with lightduer_ca,
            //sleep can trigger lightduer_ca to do the real send work
            //#include "lightduer_sleep.h"
            //duer_sleep(1);
            if (rs != DUER_OK) {
                break;
            }
        }
    } while (0);

    if (content) {
        baidu_json_release(content);
    }

    if (buf) {
        DUER_FREE(buf);
    }

    if (value) {
        baidu_json_Delete(value);
    }

    return rs;
#endif
}

int duer_data_report(const baidu_json *data)
{
    if (duer_engine_qcache_length() > DUER_MAX_MSG_NUM_IN_CA_QUEUE) {
        DUER_LOGW("CA queue max num(%d) reached!", DUER_MAX_MSG_NUM_IN_CA_QUEUE);
        return DUER_ERR_FAILED;
    }
    return duer_data_report_async(NULL, data);
}

int duer_response(const duer_msg_t *msg, int msg_code, const void *data, duer_size_t size)
{
    int rs = duer_engine_enqueue_response(msg, msg_code, data, size);
    //DUER_LOGI("===%s=== rs:%d", __func__, rs);
    return duer_after_enqueue_handler(rs);
}

int duer_seperate_response(const char *token,
                           duer_size_t token_len,
                           int msg_code,
                           const baidu_json *data)
{
    int rs = duer_engine_enqueue_seperate_response(token, token_len, msg_code, data);

    return duer_after_enqueue_handler(rs);
}

int duer_data_available()
{
    return duer_emitter_emit(duer_engine_data_available, 0, NULL);
}

int duer_clear_data(duer_u32_t timestamp)
{
    return duer_emitter_emit((duer_events_func)duer_engine_clear_data, timestamp, NULL);
}

int duer_stop()
{
    return duer_emitter_emit(duer_engine_stop, 0, NULL);
}

int duer_stop_with_reason(int reason)
{
    return duer_emitter_emit(duer_engine_stop, reason, NULL);
}

void duer_coap_response_callback(duer_context ctx,
                                 duer_msg_t* msg,
                                 duer_addr_t* addr)
{
    // to handle response with unauthorized code 129 = 0x81(4.01)
    if (msg->msg_type == DUER_MSG_TYPE_ACKNOWLEDGEMENT
            && msg->msg_code == DUER_MSG_RSP_UNAUTHORIZED) {
        // try to stop CA
        DUER_LOGW("DUER_MSG_RSP_UNAUTHORIZED code for coap ACK message, try to stop");
        duer_stop_with_reason(DUER_MSG_RSP_UNAUTHORIZED);
    }
}

int duer_finalize(void)
{
    return duer_emitter_emit(duer_engine_destroy, 0, NULL);
}
