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
 * File: lightduer_dcs_router.c
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Light duer dcs directive router.
 */

#include "lightduer_dcs_router.h"
#include "lightduer_dcs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lightduer_dcs_local.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_connagent.h"
#include "lightduer_lib.h"
#include "lightduer_ca.h"
#include "lightduer_mutex.h"
#include "lightduer_ds_log_dcs.h"
#include "lightduer_random.h"
#include "lightduer_queue_cache.h"
#ifdef DUER_EVENT_HANDLER
#include "lightduer_handler.h"
#else
#include "lightduer_event_emitter.h"
#endif // DUER_EVENT_HANDLER
#include "lightduer_ds_log_e2e.h"
#include "lightduer_voice.h"

typedef struct {
    char *name_space;
    duer_directive_list *list;
    size_t count;
} directive_namespace_list_t;

typedef enum {
    STANDARD_DCS_DIRECTIVE,
    PRIVATE_PROTOCAL_DIRECTIVE,
} directive_source_t;

typedef struct {
    baidu_json *directive;
    int index;
    directive_source_t source;
} directive_node_t;

static volatile size_t s_namespace_count;
static volatile directive_namespace_list_t *s_directive_namespace_list;
static volatile size_t s_client_context_cb_count;
static volatile dcs_client_context_handler *s_dcs_client_context_cb;
static duer_qcache_handler s_directive_queue;
static duer_mutex_t s_directive_queue_lock;
static const char *PRIVATE_NAMESPACE = "ai.dueros.private.protocol";
static volatile duer_bool s_wait_dialog_finish;
static volatile duer_bool s_is_initialized;
static volatile duer_bool s_is_speak_pending;
static volatile int s_speaking_index;
static const char *IOT_CLOUD_EXTRA_KEY = "iot_cloud_extra";
static const char *INDEX_KEY = "index";
static const int DUMMY_INDEX = 0;
#define DUER_PRINT_BJSON_ESTIMATED_SIZE_FOR_REPORT_EXCEPTION (1024)

duer_status_t duer_reg_client_context_cb_internal(dcs_client_context_handler cb)
{
    size_t size = 0;
    duer_status_t ret = DUER_OK;
    dcs_client_context_handler *list = NULL;

    if (!cb) {
        DUER_LOGE("Failed to add client context handler: param error\n");
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        ret = DUER_ERR_INVALID_PARAMETER;
        return ret;
    }

    if (!s_dcs_client_context_cb) {
        s_dcs_client_context_cb = DUER_MALLOC(sizeof(dcs_client_context_handler));
        if (!s_dcs_client_context_cb) {
            DUER_LOGE("Failed to add dcs client context cb: no memory\n");
            DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
            ret = DUER_ERR_MEMORY_OVERLOW;
            return ret;
        }
    } else {
        size = sizeof(dcs_client_context_handler) * (s_client_context_cb_count + 1);
        list = (dcs_client_context_handler *)DUER_REALLOC((void *)s_dcs_client_context_cb, size);
        if (list) {
            s_dcs_client_context_cb = list;
        } else {
            DUER_LOGE("Failed to add dcs client context cb: no memory\n");
            DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
            ret = DUER_ERR_MEMORY_OVERLOW;
            return ret;
        }
    }

    s_dcs_client_context_cb[s_client_context_cb_count] = cb;
    s_client_context_cb_count++;

    return ret;
}

duer_status_t duer_reg_client_context_cb(dcs_client_context_handler cb)
{
    duer_status_t ret = DUER_OK;

    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        ret = duer_reg_client_context_cb_internal(cb);
    } else {
        DUER_LOGW("DCS has not been initialed");
        ret = DUER_ERR_FAILED;
    }
    DUER_DCS_CRITICAL_EXIT();

    return ret;
}

baidu_json *duer_create_dcs_event(const char *name_space, const char *name, duer_bool need_msg_id)
{
    baidu_json *event = NULL;
    baidu_json *header = NULL;
    baidu_json *payload = NULL;
    const char *msg_id = NULL;

    event = baidu_json_CreateObject();
    if (event == NULL) {
        goto error_out;
    }

    header = baidu_json_CreateObject();
    if (header == NULL) {
        goto error_out;
    }

    baidu_json_AddStringToObjectCS(header, DCS_NAMESPACE_KEY, name_space);
    baidu_json_AddStringToObjectCS(header, DCS_NAME_KEY, name);

    if (need_msg_id) {
        msg_id = duer_generate_msg_id_internal();
        if (msg_id) {
            baidu_json_AddStringToObject(header, DCS_MESSAGE_ID_KEY, msg_id);
            DUER_LOGD("msg_id: %s", msg_id);
        }
    }

    baidu_json_AddItemToObjectCS(event, DCS_HEADER_KEY, header);

    payload = baidu_json_CreateObject();
    if (payload == NULL) {
        goto error_out;
    }

    baidu_json_AddItemToObjectCS(event, DCS_PAYLOAD_KEY, payload);

    return event;

error_out:
    DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
    if (event) {
        baidu_json_Delete(event);
    }
    return NULL;
}

baidu_json* duer_get_client_context_internal(void)
{
    baidu_json *client_context = NULL;
    baidu_json *state = NULL;
    int i = 0;

    client_context = baidu_json_CreateArray();
    if (!client_context) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        DUER_LOGE("Memory overlow");
        return NULL;
    }

    while ((i < s_client_context_cb_count) && (s_dcs_client_context_cb[i])) {
        state = s_dcs_client_context_cb[i]();
        if (state) {
            baidu_json_AddItemToArray(client_context, state);
        }
        i++;
    }

    return client_context;
}

static directive_namespace_list_t *duer_get_namespace_node(const char *name_space)
{
    int i = 0;
    size_t size = 0;
    char *new_namespace = NULL;
    directive_namespace_list_t *list = NULL;

    if (!s_directive_namespace_list) {
        s_directive_namespace_list = DUER_MALLOC(sizeof(*s_directive_namespace_list));
        if (!s_directive_namespace_list) {
            return NULL;
        }

        DUER_MEMSET((void *)s_directive_namespace_list, 0, sizeof(*s_directive_namespace_list));
        s_directive_namespace_list->name_space = duer_strdup_internal(name_space);
        if (!s_directive_namespace_list->name_space) {
            DUER_FREE((void *)s_directive_namespace_list);
            s_directive_namespace_list = NULL;
            return NULL;
        }

        s_namespace_count++;
        return (directive_namespace_list_t *)s_directive_namespace_list;
    }

    for (i = 0; i < s_namespace_count; i++) {
        if (DUER_STRCMP(name_space, s_directive_namespace_list[i].name_space) == 0) {
            return (directive_namespace_list_t *)&s_directive_namespace_list[i];
        }
    }

    new_namespace = duer_strdup_internal(name_space);
    if (!new_namespace) {
        return NULL;
    }

    size = sizeof(*s_directive_namespace_list) * (s_namespace_count + 1);
    list = DUER_REALLOC((void *)s_directive_namespace_list, size);
    if (!list) {
        DUER_FREE(new_namespace);
        return NULL;
    }

    s_directive_namespace_list = list;
    DUER_MEMSET((void *)&s_directive_namespace_list[s_namespace_count],
            0,
            sizeof(*s_directive_namespace_list));
    s_directive_namespace_list[s_namespace_count].name_space = new_namespace;

    return (directive_namespace_list_t *)&s_directive_namespace_list[s_namespace_count++];
}

static duer_status_t duer_directive_duplicate(directive_namespace_list_t *node,
                                              const duer_directive_list *directive)
{
    int i = 0;
    char *name = NULL;
    duer_directive_list *list = NULL;

    for (i = 0; i < node->count; i++) {
        if (DUER_STRCMP(node->list[i].directive_name, directive->directive_name) == 0) {
            node->list[i].cb = directive->cb;
            return DUER_OK;
        }
    }

    name = duer_strdup_internal(directive->directive_name);
    if (!name) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return DUER_ERR_MEMORY_OVERLOW;
    }

    if (node->list) {
        list = (duer_directive_list *)DUER_REALLOC(node->list,
                                                   sizeof(*directive) * (node->count + 1));
    } else {
        list = (duer_directive_list *)DUER_MALLOC(sizeof(*directive));
    }

    if (!list) {
        DUER_FREE(name);
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return DUER_ERR_MEMORY_OVERLOW;
    }

    list[node->count].cb = directive->cb;
    list[node->count].directive_name = name;
    node->list = list;
    node->count++;

    return DUER_OK;
}

duer_status_t duer_add_dcs_directive_internal(const duer_directive_list *directive,
                                              size_t count,
                                              const char *name_space)
{
    int i = 0;
    duer_status_t ret = DUER_OK;
    directive_namespace_list_t *node;
    DUER_LOGI("namespace: %s", name_space);

    if (!directive || (count == 0) || !name_space) {
        DUER_LOGE("Failed to add dcs directive: param error");
        DUER_DS_LOG_REPORT_DCS_PARAM_ERROR();
        ret = DUER_ERR_INVALID_PARAMETER;
        goto exit;
    }

    node = duer_get_namespace_node(name_space);
    if (!node) {
        ret = DUER_ERR_MEMORY_OVERLOW;
        DUER_LOGE("Failed to add dcs directive: no memory");
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        goto exit;
    }

    for (i = 0; i < count; i++) {
        ret = duer_directive_duplicate(node, &directive[i]);
        if (ret != DUER_OK) {
            DUER_LOGE("Failed to add dcs directive: %s, errcode: %d",
                      directive[i].directive_name,
                      ret);
            break;
        }
    }

exit:
    if ((ret != DUER_OK) && (ret != DUER_ERR_INVALID_PARAMETER)) {
        while (i < count) {
            duer_ds_log_dcs_add_directive_fail(directive[i++].directive_name);
        }
    }

    return ret;
}

static duer_status_t duer_directive_enqueue(baidu_json *directive,
                                            int index,
                                            directive_source_t source)
{
    duer_status_t rs = DUER_OK;
    directive_node_t *node = NULL;
    directive_node_t *tmp_node = NULL;
    duer_qcache_iterator *p = NULL; // used to find the location to insert the directive
    duer_qcache_iterator *target = NULL; // insert the directive after q

    node = (directive_node_t *)DUER_MALLOC(sizeof(directive_node_t));
    if (!node) {
        DUER_LOGE("Failed to cache directive: memory overlow");
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }

    node->directive = baidu_json_Duplicate(directive, 0);
    if (!node->directive) {
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto RET;
    }
    node->index = index;
    node->source = source;

    p = duer_qcache_head(s_directive_queue);
    while (p) {
        tmp_node = (directive_node_t *)duer_qcache_data(p);
        if (!tmp_node) {
            DUER_LOGE("Failed to get directive node");
            rs = DUER_ERR_FAILED;
            goto RET;
        }

        if (tmp_node->index > index) {
            break;
        }

        target = p;
        p = duer_qcache_next(p);
    }

    rs = duer_qcache_insert(s_directive_queue, target, node);
    if (rs != DUER_OK) {
        DUER_LOGE("Failed to insert directive to queue");
    }

RET:
    if (rs != DUER_OK) {
        if (node) {
            baidu_json_release(node->directive);
            DUER_FREE(node);
        }
    }

    return rs;
}

duer_status_t duer_add_dcs_directive(const duer_directive_list *directive,
                                     size_t count,
                                     const char *name_space)
{
    duer_status_t ret = DUER_OK;

    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        ret = duer_add_dcs_directive_internal(directive, count, name_space);
    } else {
        DUER_LOGW("DCS has not been initialed");
        ret = DUER_ERR_FAILED;
    }
    DUER_DCS_CRITICAL_EXIT();

    return ret;
}

static duer_status_t duer_dialog_finish_cb(const baidu_json *directive)
{
    DUER_DCS_CRITICAL_ENTER();
    s_wait_dialog_finish = DUER_FALSE;
    duer_play_channel_control_internal(DCS_DIALOG_FINISHED);
    DUER_DCS_CRITICAL_EXIT();

    return DUER_OK;
}

static duer_status_t duer_execute_directive(baidu_json *directive)
{
    int i = 0;
    duer_status_t rs = DUER_OK;
    baidu_json *name = NULL;
    baidu_json *name_space = NULL;
    baidu_json *header = NULL;
    dcs_directive_handler handler = NULL;
    directive_namespace_list_t *node = NULL;

    header = baidu_json_GetObjectItem(directive, DCS_HEADER_KEY);
    if (!header) {
        DUER_LOGE("Failed to parse directive header");
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    name_space = baidu_json_GetObjectItem(header, DCS_NAMESPACE_KEY);
    if (!name_space) {
        DUER_LOGE("Failed to parse directive namespace");
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    name = baidu_json_GetObjectItem(header, DCS_NAME_KEY);
    if (!name) {
        DUER_LOGE("Failed to parse directive name");
        rs = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    DUER_LOGI("Directive name: %s", name->valuestring);

    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        for (i = 0; i < s_namespace_count; i++) {
            if (DUER_STRCMP(s_directive_namespace_list[i].name_space,
                            name_space->valuestring) == 0) {
                node = (directive_namespace_list_t *)&s_directive_namespace_list[i];
            }
        }

        if (node) {
            for (i = 0; i < node->count; i++) {
                if (DUER_STRCMP(node->list[i].directive_name, name->valuestring) == 0) {
                    handler = node->list[i].cb;
                }
            }
        }
    }
    DUER_DCS_CRITICAL_EXIT();

    if (handler) {
        rs = handler(directive);
    } else {
        DUER_LOGE("unrecognized directive: %s, namespace: %s\n",
                  name->valuestring,
                  name_space->valuestring);
        rs = DUER_MSG_RSP_NOT_FOUND;
    }

RET:
    return rs;
}

static duer_bool duer_is_directive_equal(const char *namespace1,
                                         const char *name1,
                                         const char *namespace2,
                                         const char *name2)
{
    if (namespace1 && name1 && namespace2 && name2) {
        if ((DUER_STRCMP(name1, name2) == 0) && (DUER_STRCMP(namespace1, namespace2) == 0)) {
            return DUER_TRUE;
        }
    }

    return DUER_FALSE;
}

static duer_bool duer_is_speak_directive(baidu_json *directive)
{
    baidu_json *name = NULL;
    baidu_json *name_space = NULL;
    baidu_json *header = NULL;
    duer_bool is_speak_directive = DUER_FALSE;

    header = baidu_json_GetObjectItem(directive, DCS_HEADER_KEY);
    if (header) {
        name = baidu_json_GetObjectItem(header, DCS_NAME_KEY);
        name_space = baidu_json_GetObjectItem(header, DCS_NAMESPACE_KEY);
        if (name && name_space) {
            is_speak_directive = duer_is_directive_equal(name_space->valuestring,
                                                         name->valuestring,
                                                         DCS_VOICE_OUTPUT_NAMESPACE,
                                                         DCS_SPEAK_NAME);
        }
    }

    return is_speak_directive;
}

static void duer_dcs_check_and_report_exception(directive_node_t *node, duer_status_t rs) {
    baidu_json *value = NULL;
    char *payload = NULL;

    if (rs == DUER_OK) {
        return;
    }

    if (!node || !node->directive) {
        return;
    }

    value = baidu_json_CreateObject();
    if (!value) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return;
    }

    baidu_json_AddItemReferenceToObject(value, DCS_DIRECTIVE_KEY, node->directive);
#ifdef DUER_BJSON_PRINT_WITH_ESTIMATED_SIZE
    payload = baidu_json_PrintBuffered(value,
                                       DUER_PRINT_BJSON_ESTIMATED_SIZE_FOR_REPORT_EXCEPTION, 0);
#else
    payload = baidu_json_PrintUnformatted(value);
#endif // DUER_BJSON_PRINT_WITH_ESTIMATED_SIZE
    baidu_json_Delete(value);
    value = NULL;
    if (!payload) {
        DUER_DS_LOG_REPORT_DCS_MEMORY_ERROR();
        return;
    }
    baidu_json_Delete(node->directive);
    node->directive = NULL;

    if (rs == DUER_MSG_RSP_NOT_FOUND) {
        value = duer_get_exception_internal(payload, 0,
                                       DCS_UNSUPPORTED_OPERATION_TYPE, "Unknow directive");
    } else if (rs == DUER_MSG_RSP_BAD_REQUEST) {
        value = duer_get_exception_internal(payload, 0,
                                       DCS_UNEXPECTED_INFORMATION_RECEIVED_TYPE,
                                       "Bad directive");
    } else {
        value = duer_get_exception_internal(payload, 0, DCS_INTERNAL_ERROR_TYPE, "Internal error");
    }

    if (payload) {
        baidu_json_release(payload);
    }
    rs = DUER_ERR_FAILED;
    if (value) {
        rs = duer_dcs_data_report_internal(value, DUER_TRUE);
        baidu_json_Delete(value);
        value = NULL;
    }
    if (rs != DUER_OK) {
        duer_ds_log_dcs_event_report_fail(DCS_EXCEPTION_ENCOUNTERED_NAME);
    }
}

static void duer_execute_caching_directive(int what, void *data)
{
    duer_bool is_speak_directive = DUER_FALSE;
    duer_status_t rs = DUER_OK;
    directive_node_t *node = NULL;

    duer_mutex_lock(s_directive_queue_lock);
    do {
        if (!s_directive_queue) {
            DUER_LOGW("s_directive_queue = null");
            break;
        }

        if (duer_qcache_length(s_directive_queue) == 0) {
            break;
        }

        node = duer_qcache_pop(s_directive_queue);
        if (!node || !node->directive) {
            DUER_LOGE("Failed to get directive");
            break;
        }

        is_speak_directive = duer_is_speak_directive(node->directive);
        if (is_speak_directive) {
            s_wait_dialog_finish = DUER_TRUE;
            s_is_speak_pending = DUER_TRUE;
            s_speaking_index = node->index;
        }

        rs = duer_execute_directive(node->directive);
        if (node->source == STANDARD_DCS_DIRECTIVE) {
            duer_dcs_check_and_report_exception(node, rs);
        }

        if (node->directive) {
            baidu_json_Delete(node->directive);
            node->directive = NULL;
        }
        DUER_FREE(node);

        if (!s_is_speak_pending) {
            if (duer_qcache_length(s_directive_queue) > 0) {
#ifdef DUER_EVENT_HANDLER
                duer_handler_handle(duer_execute_caching_directive, 0, NULL);
#else
                duer_emitter_emit(duer_execute_caching_directive, 0, NULL);
#endif // DUER_EVENT_HANDLER
            }
        }
    } while(0);

    duer_mutex_unlock(s_directive_queue_lock);
}

static void duer_clear_directive_queue(int what, void *data)
{
    directive_node_t *node = NULL;

    duer_mutex_lock(s_directive_queue_lock);

    s_speaking_index = DUMMY_INDEX;
    if (s_is_speak_pending) {
        duer_dcs_stop_speak_handler();
        s_is_speak_pending = DUER_FALSE;
    }
    s_wait_dialog_finish = DUER_FALSE;
    duer_speech_on_stop_internal();

    if (!s_directive_queue) {
        duer_mutex_unlock(s_directive_queue_lock);
        return;
    }

    do {
        node = duer_qcache_pop(s_directive_queue);
        if (node) {
            baidu_json_Delete(node->directive);
            DUER_FREE(node);
        }
    } while (duer_qcache_length(s_directive_queue) > 0);

    duer_mutex_unlock(s_directive_queue_lock);
}

void duer_cancel_caching_directive_internal()
{
#ifdef DUER_EVENT_HANDLER
    duer_handler_handle(duer_clear_directive_queue, 0, NULL);
#else
    duer_emitter_emit(duer_clear_directive_queue, 0, NULL);
#endif // DUER_EVENT_HANDLER
}

duer_bool duer_wait_dialog_finished_internal(void)
{
    return s_wait_dialog_finish;
}

void duer_speak_directive_finished_internal(void)
{
    duer_mutex_lock(s_directive_queue_lock);

    s_speaking_index = DUMMY_INDEX;
    s_is_speak_pending = DUER_FALSE;
    do {
        if (!s_directive_queue) {
            DUER_LOGW("s_directive_queue = null");
            break;
        }

        if (duer_qcache_length(s_directive_queue) > 0) {
#ifdef DUER_EVENT_HANDLER
            duer_handler_handle(duer_execute_caching_directive, 0, NULL);
#else
            duer_emitter_emit(duer_execute_caching_directive, 0, NULL);
#endif // DUER_EVENT_HANDLER
        }
    } while (0);
    duer_mutex_unlock(s_directive_queue_lock);
}

#ifdef DUER_STATISTICS_E2E
static void duer_e2e_response(const char *name_space, const char *name)
{
    duer_bool is_e2e_response_directive = DUER_FALSE;

    if (!duer_ds_e2e_wait_response()) {
        return;
    }

    is_e2e_response_directive = duer_is_directive_equal(name_space,
                                                        name,
                                                        DCS_VOICE_OUTPUT_NAMESPACE,
                                                        DCS_SPEAK_NAME);
    if (!is_e2e_response_directive) {
        is_e2e_response_directive = duer_is_directive_equal(name_space,
                                                            name,
                                                            DCS_AUDIO_PLAYER_NAMESPACE,
                                                            DCS_PLAY_NAME);
    }

    if (is_e2e_response_directive) {
        duer_ds_e2e_event(DUER_E2E_RESPONSE);
    }
}
#endif

static duer_status_t duer_handle_directive(baidu_json *directive,
                                           int index,
                                           directive_source_t source)
{
    baidu_json *dialog_id = NULL;
    baidu_json *name = NULL;
    baidu_json *header = NULL;
    baidu_json *name_space = NULL;
    const char *current_dialog_id = NULL;
    duer_status_t rs = DUER_OK;
    duer_bool is_unblock_directive = DUER_FALSE;

    header = baidu_json_GetObjectItem(directive, DCS_HEADER_KEY);
    if (!header) {
        DUER_LOGE("Failed to parse header");
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    dialog_id = baidu_json_GetObjectItem(header, DCS_DIALOG_REQUEST_ID_KEY);
    // execute the directive immediately if no dialog_id item
    if (!dialog_id) {
        rs = duer_execute_directive(directive);
        return rs;
    }

    if (!dialog_id->valuestring) {
        DUER_LOGE("Failed to parse dialog_id");
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    current_dialog_id = duer_get_request_id_internal();
    if (!current_dialog_id) {
        DUER_LOGE("Failed to get current dialog id");
        return DUER_ERR_FAILED;
    }

    name = baidu_json_GetObjectItem(header, DCS_NAME_KEY);
    if (!name || !name->valuestring) {
        DUER_LOGE("Failed to parse name");
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    if (DUER_STRCMP(current_dialog_id, dialog_id->valuestring)) {
        DUER_LOGI("Drop the directive for old dialog_Id: %s, current dialog_Id: %s",
                  dialog_id->valuestring, current_dialog_id);
        duer_ds_log_dcs_directive_drop(current_dialog_id,
                                       dialog_id->valuestring,
                                       name->valuestring);
        return DUER_OK;
    }

    name_space = baidu_json_GetObjectItem(header, DCS_NAMESPACE_KEY);
    if (!name_space || !name_space->valuestring) {
        DUER_LOGE("Failed to parse namespace");
        return DUER_MSG_RSP_BAD_REQUEST;
    }

#ifdef DUER_STATISTICS_E2E
    duer_e2e_response(name_space->valuestring, name->valuestring);
#endif

    is_unblock_directive = duer_is_directive_equal(name_space->valuestring,
                                                   name->valuestring,
                                                   DCS_VOICE_INPUT_NAMESPACE,
                                                   DCS_STOP_LISTEN_NAME);
    if (!is_unblock_directive) {
        is_unblock_directive = duer_is_directive_equal(name_space->valuestring,
                                                       name->valuestring,
                                                       DCS_SCREEN_NAMESPACE,
                                                       DCS_RENDER_VOICE_INPUT_TEXT_NAME);
    }

    if (is_unblock_directive) {
        DUER_LOGD("Unblocked directive");
        rs = duer_execute_directive(directive);
        return rs;
    }

    duer_mutex_lock(s_directive_queue_lock);

    if (s_directive_queue) {
        rs = duer_directive_enqueue(directive, index, source);

        if (rs == DUER_OK) {
            if ((duer_qcache_length(s_directive_queue) == 1 && !s_is_speak_pending)
                || (s_is_speak_pending && index < s_speaking_index)){
#ifdef DUER_EVENT_HANDLER
                rs = duer_handler_handle(duer_execute_caching_directive, 0, NULL);
#else
                rs = duer_emitter_emit(duer_execute_caching_directive, 0, NULL);
#endif // DUER_EVENT_HANDLER
            }
        } else {
            DUER_LOGE("Failed to enqueue directive");
        }

        DUER_LOGD("queue len: %d", duer_qcache_length(s_directive_queue));
    }
    duer_mutex_unlock(s_directive_queue_lock);

    return rs;
}

// now the last parameter payload is not used
static duer_status_t duer_dcs_directive_parse(duer_msg_t *msg,
                                              baidu_json **value,
                                              baidu_json **directive,
                                              baidu_json **header,
                                              baidu_json **name_space,
                                              baidu_json **name,
                                              char **payload)
{
    duer_status_t rs = DUER_OK;
    baidu_json *directive_header = NULL;

    do {
        if (!s_is_initialized) {
            DUER_LOGW("DCS has been uninitialized");
            rs = DUER_MSG_RSP_NOT_FOUND;
            break;
        }

        if (!msg) {
            rs = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        if (!value || !directive) {
            rs = DUER_ERR_INVALID_PARAMETER;
            break;
        }

        if (!msg->payload || msg->payload_len <= 0) {
            rs = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        *value = baidu_json_Parse(msg->payload);

        if (*value == NULL) {
            DUER_LOGE("Failed to parse payload");
            rs = DUER_ERR_FAILED;
            break;
        }

        *directive = baidu_json_GetObjectItem(*value, DCS_DIRECTIVE_KEY);
        if (*directive == NULL) {
            DUER_LOGE("Failed to parse directive");
            rs = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        if (!header && !name_space && !name) {
            break;
        }

        directive_header = baidu_json_GetObjectItem(*directive, DCS_HEADER_KEY);
        if (!directive_header) {
            DUER_LOGE("Failed to parse header");
            rs = DUER_MSG_RSP_BAD_REQUEST;
            break;
        }

        if (header) {
            *header = directive_header;
        }

        if (name_space) {
            *name_space = baidu_json_GetObjectItem(directive_header, DCS_NAMESPACE_KEY);
            if (*name_space == NULL) {
                DUER_LOGE("Failed to parse directive namespace");
                rs = DUER_MSG_RSP_BAD_REQUEST;
                break;
            }
        }

        if (name) {
            *name = baidu_json_GetObjectItem(directive_header, DCS_NAME_KEY);
            if (*name == NULL) {
                DUER_LOGE("Failed to parse directive namespace");
                rs = DUER_MSG_RSP_BAD_REQUEST;
                break;
            }
        }
    } while (0);

    return rs;
}

static int duer_get_directive_index(baidu_json *value, const char *name)
{
    baidu_json *index = NULL;
    baidu_json *extra_info = NULL;

    extra_info = baidu_json_GetObjectItem(value, IOT_CLOUD_EXTRA_KEY);
    if (extra_info) {
        index = baidu_json_GetObjectItem(extra_info, INDEX_KEY);
#if 0
        baidu_json *timestamp = baidu_json_GetObjectItem(extra_info, "timestamp");
        DUER_LOGI("directive name:%s, timestamp:%s, index:%d",
                  name,
                  (timestamp && timestamp->valuestring) ? timestamp->valuestring : "",
                  index ? index->valueint : DUMMY_INDEX);
#endif
    }

    return index ? index->valueint : DUMMY_INDEX;
}

static duer_status_t duer_dcs_router(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    char *payload = NULL;
    duer_status_t rs = DUER_OK;
    baidu_json *directive = NULL;
    baidu_json *value = NULL;
    duer_msg_code_e msg_code = DUER_MSG_RSP_CHANGED;
    baidu_json *name = NULL;
    int index = DUMMY_INDEX;

    DUER_LOGV("Enter");

    do {
        rs = duer_dcs_directive_parse(msg, &value, &directive, NULL, NULL, &name, NULL);
        if (rs != DUER_OK) {
            break;
        }

        index = duer_get_directive_index(value, name->valuestring);
        rs = duer_handle_directive(directive, index, STANDARD_DCS_DIRECTIVE);
    } while (0);

    if (rs != DUER_OK) {
        if (rs == DUER_MSG_RSP_NOT_FOUND) {
            duer_report_exception_internal(msg->payload, msg->payload_len,
                                           DCS_UNSUPPORTED_OPERATION_TYPE,
                                           "Unknow directive");
            msg_code = DUER_MSG_RSP_NOT_FOUND;
        } else if (rs == DUER_MSG_RSP_BAD_REQUEST) {
            duer_report_exception_internal(msg->payload, msg->payload_len,
                                           DCS_UNEXPECTED_INFORMATION_RECEIVED_TYPE,
                                           "Bad directive");
            msg_code = DUER_MSG_RSP_BAD_REQUEST;
        } else {
            duer_report_exception_internal(msg->payload, msg->payload_len,
                                           DCS_INTERNAL_ERROR_TYPE, "Internal error");
            msg_code = DUER_MSG_RSP_INTERNAL_SERVER_ERROR;
        }
    }

    duer_response(msg, msg_code, NULL, 0);

    if (payload) {
        DUER_FREE(payload);
    }

    if (value) {
        baidu_json_Delete(value);
    }

    return rs;
}

static duer_status_t duer_private_protocal_cb(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    duer_status_t rs = DUER_OK;
    baidu_json *value = NULL;
    baidu_json *name = NULL;
    baidu_json *name_space = NULL;
    baidu_json *header = NULL;
    baidu_json *directive = NULL;
    baidu_json *client_context = NULL;
    baidu_json *response_json = NULL;
    char *response_data = NULL;
    size_t response_data_size = 0;
    duer_bool seperated_response = DUER_FALSE;
    duer_msg_code_e msg_code = DUER_MSG_RSP_CHANGED;
    int index = DUMMY_INDEX;

    DUER_LOGV("Enter");

    rs = duer_dcs_directive_parse(msg, &value, &directive, &header, &name_space, &name, NULL);
    if (rs != DUER_OK) {
        goto RET;
    }

    if (DUER_STRCMP(DCS_GET_STATUS_NAME, name->valuestring) == 0
        && DUER_STRCMP(PRIVATE_NAMESPACE, name_space->valuestring)) {
        if (msg->token_len > 0) {
            DUER_LOGI("response, mid:%d", msg->msg_id);
            duer_response(msg, DUER_MSG_EMPTY_MESSAGE, NULL, 0);// ack in empty message first!
            seperated_response = DUER_TRUE;
        }
        DUER_DCS_CRITICAL_ENTER();
        client_context = duer_get_client_context_internal();
        DUER_DCS_CRITICAL_EXIT();
        if (client_context) {
            response_json = baidu_json_CreateObject();
            if (response_json) {
                baidu_json_AddItemToObjectCS(response_json, DCS_CLIENT_CONTEXT_KEY, client_context);
                //response_data = baidu_json_PrintUnformatted(response_json);
                //response_data_size = response_data ? DUER_STRLEN(response_data) : 0;
            } else {
                DUER_LOGE("Failed to create response msg: memory too low");
                baidu_json_Delete(client_context);
                rs = DUER_ERR_MEMORY_OVERLOW;
            }
        }
    }  else {
        index = duer_get_directive_index(value, name->valuestring);
        rs = duer_handle_directive(directive, index, PRIVATE_PROTOCAL_DIRECTIVE);
    }

RET:

    if (rs != DUER_OK) {
        if (rs == DUER_MSG_RSP_BAD_REQUEST) {
            msg_code = DUER_MSG_RSP_BAD_REQUEST;
        } else {
            msg_code = DUER_MSG_RSP_INTERNAL_SERVER_ERROR;
        }
    }

    if (seperated_response == DUER_FALSE) {
        duer_response(msg, msg_code, response_data, response_data_size);
    } else {
        DUER_LOGI("seperated response, token_len:%d", msg->token_len);
        duer_seperate_response((const char *)msg->token, msg->token_len, msg_code, response_json);
    }

    if (response_json) {
        baidu_json_Delete(response_json);
    }

    if (response_data) {
        DUER_FREE(response_data);
    }

    if (value) {
        baidu_json_Delete(value);
    }

    return rs;
}

static duer_status_t duer_iot_cloud_cb(duer_context ctx, duer_msg_t* msg, duer_addr_t* addr)
{
    duer_status_t rs = DUER_OK;
    baidu_json *value = NULL;
    baidu_json *directive = NULL;
    duer_msg_code_e msg_code = DUER_MSG_RSP_CHANGED;

    DUER_LOGV("Enter");

    rs = duer_dcs_directive_parse(msg, &value, &directive, NULL, NULL, NULL, NULL);
    if (rs == DUER_OK) {
        rs = duer_execute_directive(directive);
    }

    if (rs != DUER_OK) {
        if (rs == DUER_MSG_RSP_NOT_FOUND) {
            msg_code = DUER_MSG_RSP_NOT_FOUND;
        } else if (rs == DUER_MSG_RSP_BAD_REQUEST) {
            msg_code = DUER_MSG_RSP_BAD_REQUEST;
        } else {
            msg_code = DUER_MSG_RSP_INTERNAL_SERVER_ERROR;
        }
    }

    duer_response(msg, msg_code, NULL, 0);

    if (value) {
        baidu_json_Delete(value);
    }

    return rs;
}

void duer_dcs_dialog_cancel(void)
{
    DUER_DCS_CRITICAL_ENTER();
    if (s_is_initialized) {
        duer_create_request_id_internal();
    } else {
        DUER_LOGW("DCS has not been initialized");
    }
    DUER_DCS_CRITICAL_EXIT();
}

static duer_status_t dummy_directive_cb(const baidu_json *directive)
{
    return DUER_OK;
}

void duer_dcs_framework_init()
{
    duer_res_t res[] = {
        {
            DUER_RES_MODE_DYNAMIC,
            DUER_RES_OP_PUT,
            (char *)DCS_DUER_DIRECTIVE_PATH,
            {
                duer_dcs_router
            }
        },
        {
            DUER_RES_MODE_DYNAMIC,
            DUER_RES_OP_PUT,
            (char *)DCS_DUER_PRIVATE_PATH,
            {
                duer_private_protocal_cb
            }
        },
        {
            DUER_RES_MODE_DYNAMIC,
            DUER_RES_OP_PUT,
            (char *)DCS_IOTCLOUD_DIRECTIVE_PATH,
            {
                duer_iot_cloud_cb
            }
        },
    };

    duer_add_resources(res, sizeof(res) / sizeof(res[0]));
    duer_ds_e2e_set_dialog_id_cb(duer_get_request_id_internal);
    duer_reg_dialog_id_cb(duer_get_request_id_internal);

    duer_dcs_local_init_internal();
    DUER_DCS_CRITICAL_ENTER();

    if (s_is_initialized) {
        DUER_DCS_CRITICAL_EXIT();
        DUER_LOGW("DCS has been initialized");
        return;
    }

    duer_dcs_sys_exit_internal();

    if (!s_directive_queue) {
        s_directive_queue = duer_qcache_create();
        if (!s_directive_queue) {
            DUER_LOGE("Failed to create DCS directive queue");
        }
    }

    if (!s_directive_queue_lock) {
        s_directive_queue_lock = duer_mutex_create();
        if (!s_directive_queue_lock) {
            DUER_LOGE("Failed to create s_directive_queue_lock");
        }
    }

    duer_directive_list directive = {DCS_DIALOGUE_FINISHED_NAME, duer_dialog_finish_cb};
    duer_add_dcs_directive_internal(&directive, 1, PRIVATE_NAMESPACE);

    // Unnecessary directives might be received for some special reason, exception event will be
    // reported to cloud if these directives have no handler. Hence we register default callbacks
    // for them.
    duer_directive_list list[] = {
        {DCS_RENDER_PLAYER_INFO, dummy_directive_cb},
        {DCS_RENDER_AUDIO_LIST, dummy_directive_cb},
        {DCS_RENDER_ALBUM_LIST, dummy_directive_cb},
    };
    duer_add_dcs_directive_internal(list,
                                    sizeof(list) / sizeof(list[0]),
                                    DCS_SCREEN_EXTENDED_CARD_NAMESPACE);

    duer_directive_list screen_directive[] = {
        {DCS_RENDER_VOICE_INPUT_TEXT_NAME, dummy_directive_cb},
        {DCS_RENDER_CARD_NAME, dummy_directive_cb},
    };
    duer_add_dcs_directive_internal(screen_directive,
                                    sizeof(screen_directive) / sizeof(screen_directive[0]),
                                    DCS_SCREEN_NAMESPACE);

    s_is_initialized = DUER_TRUE;

    duer_declare_sys_interface_internal();
    duer_dcs_play_control_init_internal();
    DUER_DCS_CRITICAL_EXIT();
}

void duer_dcs_uninitialize(void)
{
    int i = 0;
    int j = 0;

    DUER_DCS_CRITICAL_ENTER();

    if (!s_is_initialized) {
        DUER_LOGW("DCS has not been initialized");
        DUER_DCS_CRITICAL_EXIT();
        return;
    }

    duer_dcs_sys_exit_internal();
    s_is_initialized = DUER_FALSE;

    while (i < s_namespace_count) {
        if (s_directive_namespace_list[i].name_space) {
            DUER_FREE(s_directive_namespace_list[i].name_space);
        }

        if (s_directive_namespace_list[i].list) {
            for (j = 0; j < s_directive_namespace_list[i].count; j++) {
                if (s_directive_namespace_list[i].list[j].directive_name) {
                    DUER_FREE((void *)s_directive_namespace_list[i].list[j].directive_name);
                }
            }
            DUER_FREE(s_directive_namespace_list[i].list);
        }

        i++;
    }

    s_namespace_count = 0;
    if (s_directive_namespace_list) {
        DUER_FREE((void *)s_directive_namespace_list);
        s_directive_namespace_list = NULL;
    }

    if (s_dcs_client_context_cb) {
        DUER_FREE((void *)s_dcs_client_context_cb);
        s_dcs_client_context_cb = NULL;
    }

    s_client_context_cb_count = 0;

    duer_mutex_lock(s_directive_queue_lock);
    if (s_directive_queue) {
        duer_qcache_destroy(s_directive_queue);
        s_directive_queue = NULL;
    }
    duer_mutex_unlock(s_directive_queue_lock);

    duer_sys_interface_uninitialize_internal();
    duer_dcs_alert_uninitialize_internal();
    duer_dcs_audio_player_uninitialize_internal();
    duer_dcs_device_control_uninitialize_internal();
    duer_dcs_play_control_uninitialize_internal();
    duer_dcs_voice_output_uninitialize_internal();
    duer_dcs_voice_input_uninitialize_internal();
    duer_dcs_screen_uninitialize_internal();
    duer_dcs_local_uninitialize_internal();

    DUER_DCS_CRITICAL_EXIT();
}
