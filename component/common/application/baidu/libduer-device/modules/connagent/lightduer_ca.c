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
// Description: Provide the API for external applications.

#include "lightduer_ca.h"
#include "lightduer_timestamp.h"
#include "lightduer_lib.h"
#include "lightduer_coap_trace.h"
#include "lightduer_coap.h"
#include "lightduer_ca_conf.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"
#include "lightduer_ds_log_ca.h"
#include "lightduer_timestamp.h"
#include "baidu_json.h"

#define DUER_CONF_KEY_UUID       "uuid"
#define DUER_CONF_KEY_TOKEN      "token"
#define DUER_CONF_KEY_BINDTOKEN  "bindToken"
#define DUER_CONF_KEY_HOST       "serverAddr"
#define DUER_CONF_KEY_PORT_REG   "lwm2mPort"
#define DUER_CONF_KEY_PORT_RPT   "coapPort"
#define DUER_CONF_KEY_CERT       "rsaCaCrt"
#define DUER_CONF_KEY_CUID       "cuid"

#define DUER_COAP_LIFETIME       "3600"

#define DUER_SR_REG_PATH         "reg_for_control"
#define DUER_REPORT_PATH         "v1/device/data"

#define DUER_RPT_QUERY_UUID      "deviceUuid="
#define DUER_RPT_QUERY_TOKEN     "token="

#define SR_TAG_UUID             "deviceUUID"
#define SR_TAG_TOKEN            "token"
#define SR_TAG_CUID             "cuid"

#if defined(DUER_UDP_REPORTER)
#define SR_HOST                 "117.185.17.43"
#define SR_PORT                 8899
#endif

#define DUER_READ_TIMEOUT        (0)

#define DUER_RES_INC_COUNT       (5)

typedef enum _baidu_ca_start_state_enum {
    DUER_ST_STOPED,
    DUER_ST_UNREGISTERED,
    DUER_ST_REGISTER_CONNECTED,
    DUER_ST_REGISTERED,
    DUER_ST_SR_CONNECTING,
    DUER_ST_SR_DATA_SENDING,
    DUER_ST_SR_DATA_RECEIVING,
    DUER_ST_SR_INITIALIZED,
    DUER_ST_RPT_CONNECTED,
    DUER_ST_RPT_INITIALIZED,
    DUER_ST_STARTED,
    DUER_ST_REPORTING,
    DUER_ST_REPORTED,
    DUER_ST_REGISTER_FAIL
} duer_state_e;

typedef struct _baidu_ca_s {
    // Start state
    volatile duer_u8_t      state;

    // register message id
#if defined(DUER_LWM2M_REGISTER)
    duer_u16_t              msg_id_reg;
    duer_u16_t              msg_id_unreg;
#endif

    // Configuation
    duer_conf_handler       conf;

    // mutex
    duer_mutex_t            mutex;

    // Configuration
    char*                   uuid;
    char*                   token;
    char*                   bindToken;
    // If cuid is not null, deviceUuid will be binded to cuid on IoT cloud
    char*                   cuid;
    char*                   serverAddr;
    char*                   report_query;
    duer_size_t             report_query_len;

    const char*             cert;
    duer_size_t             cert_length;

    // Token generator
    duer_u16_t              gen_token;

    // Coap handler
#if defined(DUER_LWM2M_REGISTER)
    duer_coap_handler       reg;
#endif
    duer_coap_handler       sr;
#if defined(DUER_UDP_REPORTER)
    duer_coap_handler       rpt;
#endif

    // Resource list
    duer_res_t*             res_list;
    duer_u32_t              res_size;
    duer_u32_t              res_capa;

    // Response callback
    duer_notify_f           f_response;
    duer_context            ctx_response;

    // Response callback
    duer_transmit_f         f_transmit;

    // Socket context
    duer_transevt_func      ctx_socket;

    // Remote address
#if defined(DUER_LWM2M_REGISTER)
    duer_addr_t             addr_reg;
#endif
    duer_addr_t             addr_sr;
#if defined(DUER_UDP_REPORTER)
    duer_addr_t             addr_rpt;
#endif
    duer_u32_t              start_connect_time;
} baidu_ca_t;

static volatile duer_u32_t s_max_connect_time_ms = 0;
static volatile duer_u32_t s_count_connect = 0;
static volatile duer_u32_t s_total_connect_time_ms = 0;

void baidu_ca_get_statistic(duer_u32_t* connect_avg_time,
                            duer_u32_t* max_connect_time,
                            duer_u32_t* count_connect)
{
    *max_connect_time = s_max_connect_time_ms / 1000; // unit is ms
    *count_connect = s_count_connect;

    if (s_count_connect > 0) {
        *connect_avg_time =
            (s_total_connect_time_ms * 0.001) / s_count_connect; // unit is ms
    } else {
        *connect_avg_time = 0;
    }
}

DUER_EXT_IMPL const char *baidu_ca_get_uuid(duer_handler hdlr)
{
    baidu_ca_t *ctx = (baidu_ca_t *)hdlr;

    if (ctx) {
        return (char *)ctx->uuid;
    }

    return NULL;
}

DUER_EXT_IMPL const char *baidu_ca_get_bind_token(duer_handler hdlr)
{
    baidu_ca_t *ctx = (baidu_ca_t *)hdlr;

    if (ctx) {
        return ctx->bindToken;
    }

    return NULL;
}

DUER_EXT_IMPL const char *baidu_ca_get_rsa_cacrt(duer_handler hdlr)
{
    baidu_ca_t *ctx = (baidu_ca_t *)hdlr;

    if (ctx) {
        return ctx->cert;
    }

    return NULL;
}

DUER_LOC_IMPL duer_status_t duer_engine_coap_handler(duer_context ctx,
                                                     duer_coap_handler coap,
                                                     const duer_msg_t* msg,
                                                     const duer_addr_t* addr)
{
    duer_status_t rs = DUER_ERR_FAILED;
    baidu_ca_t* ptr = (baidu_ca_t*)ctx;

    if (ptr && msg && addr) {
        rs = DUER_OK;
        DUER_LOGV("==> duer_engine_coap_handler: state = %d", ptr->state);
#if defined(DUER_LWM2M_REGISTER)
        if (ptr->reg == coap) {
            if (ptr->state == DUER_ST_REGISTER_CONNECTED && ptr->msg_id_reg == msg->msg_id) {
                ptr->msg_id_reg = 0;

                if (msg->msg_code == DUER_MSG_RSP_CREATED) {
                    ptr->state = DUER_ST_REGISTERED;
                }
            } else if (ptr->state == DUER_ST_STOPED && ptr->msg_id_unreg == msg->msg_id) {
                ptr->msg_id_unreg = 0;
            }
        } else
#endif
        if (ptr->sr == coap) {
            if (ptr->state == DUER_ST_SR_DATA_RECEIVING) {
                if (msg->msg_code == DUER_MSG_RSP_CONTENT) {
                    DUER_LOGD("ca status: DUER_ST_SR_INITIALIZED");
                    ptr->state = DUER_ST_SR_INITIALIZED;
                    duer_ds_log_ca_start_state_change(DUER_ST_SR_DATA_RECEIVING, DUER_ST_SR_INITIALIZED);
                    // for debug
                    // char *payload = (char *)DUER_MALLOC(msg->payload_len + 1);
                    // DUER_MEMCPY(payload, msg->payload, msg->payload_len);
                    // payload[msg->payload_len] = '\0';
                    // DUER_LOGI("register ack payload: %s", payload);
                    // DUER_LOGI("register ack msg_id: %d", msg->msg_id);
                } else {
                    ptr->state = DUER_ST_REGISTER_FAIL;
                    duer_ds_log_ca_start_state_change(DUER_ST_SR_DATA_RECEIVING, DUER_ST_REGISTER_FAIL);
                    DUER_LOGE("register fail, code: %d", msg->msg_code);
                    duer_ds_log_ca_cache_error(DUER_DS_LOG_CA_START_STATE_REG_FAIL);
                }
            }
            if (ptr->f_response) {
                ptr->f_response(ptr->ctx_response, (duer_msg_t*)msg, NULL);
            }
        }
#if defined(DUER_UDP_REPORTER)
        else if (ptr->rpt == coap) {
            if (ptr->f_response) {
                ptr->f_response(ptr->ctx_response, (duer_msg_t*)msg, NULL);
            }
        }
#endif

        DUER_LOGV("<== duer_engine_coap_handler: rs = %d", rs);
    }

    return rs;
}

#if defined(DUER_UDP_REPORTER) || defined(DUER_LWM2M_REGISTER)
DUER_LOC_IMPL duer_bool baidu_ca_match_address(const duer_addr_t* l,
        const duer_addr_t* r) {
    if (l) {
        DUER_LOGV("baidu_ca_match_address: l = {ip:%s, port:%d, type:%d, length:%d}",
                 l->host, l->port, l->type, l->host_size);
    }

    if (r) {
        DUER_LOGV("baidu_ca_match_address: r = {ip:%s, port:%d, type:%d, length:%d}",
                 r->host, r->port, r->type, r->host_size);
    }

    if (l == r || (l && r && l->port == r->port && l->type == r->type
                   && l->host_size == r->host_size
                   && DUER_MEMCMP(r->host, l->host, r->host_size) == 0)) {
        return DUER_TRUE;
    }

    return DUER_FALSE;
}
#endif

#if defined(DUER_LWM2M_REGISTER)
DUER_LOC_IMPL duer_status_t baidu_ca_register(baidu_ca_t* ctx)
{
    duer_status_t rs = DUER_ERR_FAILED;
    duer_coap_ep_t ep;
    DUER_LOGV("==> baidu_ca_register");

    if (!ctx) {
        goto exit;
    }

    if (!ctx->reg) {
        if (!(ctx->reg = duer_coap_acquire(duer_engine_coap_handler, ctx,
                                          ctx->ctx_socket, ctx->uuid))) {
            goto exit;
        }
        duer_coap_set_tx_callback(ctx->reg, ctx->f_transmit);
    }

    if (ctx->state == DUER_ST_UNREGISTERED) {
        rs = duer_coap_connect(ctx->reg, &ctx->addr_reg, ctx->cert, ctx->cert_length);

        if (rs == DUER_OK) {
            ctx->state = DUER_ST_REGISTER_CONNECTED;
        }
    }

    if (ctx->state != DUER_ST_REGISTER_CONNECTED) {
        goto exit;
    }

    // Send register message
    if (ctx->msg_id_reg == 0) {
        DUER_MEMSET(&ep, 0, sizeof(ep));
        ep.name_ptr = (char*)ctx->uuid;
        ep.name_len = DUER_STRLEN(ctx->uuid);
        ep.type_ptr = (char*)ctx->token;
        ep.type_len = DUER_STRLEN(ctx->token);
        ep.lifetime_ptr = DUER_COAP_LIFETIME;
        ep.lifetime_len = DUER_STRLEN(DUER_COAP_LIFETIME);
        rs = duer_coap_register(ctx->reg, &ep);

        if (rs == 0) {
            rs = DUER_ERR_FAILED;
        }

        if (rs > 0) {
            ctx->msg_id_reg = (duer_u16_t)rs;
        } else {
            goto exit;
        }
    }

    rs = duer_coap_data_available(ctx->reg);
exit:
    DUER_LOGV("<== baidu_ca_register: rs = %d", rs);
    return rs;
}
#endif

DUER_LOC_IMPL char* baidu_ca_get_service_router_payload(
        const char* uuid, const char* token, const char* cuid)
{
    char* rs = NULL;
    baidu_json* item = baidu_json_CreateObject();

    if (item) {
        baidu_json_AddStringToObject(item, SR_TAG_UUID, uuid);
        baidu_json_AddStringToObject(item, SR_TAG_TOKEN, token);
        if (cuid != NULL) {
            baidu_json_AddStringToObject(item, SR_TAG_CUID, cuid);
        }
        rs = baidu_json_PrintUnformatted(item);
        baidu_json_Delete(item);
    }

    return rs;
}

DUER_LOC_IMPL duer_status_t baidu_ca_create_service_router(baidu_ca_t* ctx)
{
    duer_status_t rs = DUER_ERR_FAILED;
    duer_msg_t msg_reg;
    DUER_LOGV("==> baidu_ca_create_service_router");

    if (!ctx) {
        goto exit;
    }

    if (!ctx->sr) {
        ctx->sr = duer_coap_acquire(duer_engine_coap_handler, ctx, ctx->ctx_socket, ctx->uuid);
        if (!ctx->sr) {
            goto exit;
        }
        duer_coap_set_tx_callback(ctx->sr, ctx->f_transmit);
    }

    if (ctx->state == DUER_ST_SR_CONNECTING) {
        //record the start connect time
        if (ctx->start_connect_time == 0) {
            ctx->start_connect_time = duer_timestamp();
        }
        duer_ds_log_ca_start_connect(ctx->addr_sr.host, ctx->addr_sr.port);
#ifndef NET_TRANS_ENCRYPTED_BY_AES_CBC
        rs = duer_coap_connect(ctx->sr, &ctx->addr_sr, ctx->cert, ctx->cert_length);
#else
        // bindToken is the key for AES encryption
        rs = duer_coap_connect(ctx->sr, &ctx->addr_sr, ctx->bindToken, DUER_STRLEN(ctx->bindToken));
#endif // NET_TRANS_ENCRYPTED_BY_AES_CBC

        // baidu_ca_unload_configuration(ctx); //release the bjson format of the profile to save memory

        DUER_LOGD("ca state: DUER_ST_SR_CONNECTING, rs:%d", rs);
        if (rs == DUER_OK) {
            ctx->state = DUER_ST_SR_DATA_SENDING;
            //after success connect, calculate the connect time
            int time_end = duer_timestamp();
            int time_cost = time_end - ctx->start_connect_time;

            if (time_cost > s_max_connect_time_ms) {
                s_max_connect_time_ms = time_cost;
            }

            ++s_count_connect;
            s_total_connect_time_ms += time_cost;
            ctx->start_connect_time = 0;
        }
    }

    if (ctx->state == DUER_ST_SR_DATA_SENDING) {
        DUER_MEMSET(&msg_reg, 0, sizeof(msg_reg));
        msg_reg.msg_type = DUER_MSG_TYPE_CONFIRMABLE;
        msg_reg.msg_code = DUER_MSG_REQ_POST;
        msg_reg.path = (duer_u8_t *)DUER_SR_REG_PATH;
        msg_reg.path_len = DUER_STRLEN(DUER_SR_REG_PATH);
        msg_reg.payload = (duer_u8_t*)baidu_ca_get_service_router_payload(
                                        ctx->uuid, ctx->token, ctx->cuid);
        msg_reg.payload_len = DUER_STRLEN((char*)msg_reg.payload);
        rs = duer_coap_send(ctx->sr, &msg_reg);
        baidu_json_release(msg_reg.payload);
        DUER_LOGD("ca state: DUER_ST_SR_DATA_SENDING, rs:%d", rs);

        if (rs < DUER_OK) {
            goto exit;
        }
        ctx->state = DUER_ST_SR_DATA_RECEIVING;
        duer_ds_log_ca_start_state_change(DUER_ST_SR_DATA_SENDING, DUER_ST_SR_DATA_RECEIVING);
    }

    if (ctx->state == DUER_ST_SR_DATA_RECEIVING) {
        rs = duer_coap_data_available(ctx->sr);
        DUER_LOGD("ca state: DUER_ST_SR_DATA_RECEIVING, rs:%d", rs);
        if (rs < DUER_OK) {
            goto exit;
        }
    }

    if (ctx->state == DUER_ST_SR_INITIALIZED) {
        DUER_LOGD("ca state: DUER_ST_SR_INITIALIZED");
        baidu_ca_add_resources(ctx, ctx->res_list, ctx->res_size);
        if (ctx->res_list) {
            DUER_FREE(ctx->res_list);
            ctx->res_list = NULL;
            ctx->res_size = 0;
            ctx->res_capa = 0;
        }
    }

    if (ctx->state == DUER_ST_REGISTER_FAIL) {
        rs = DUER_ERR_REG_FAIL;
    }

exit:
    DUER_LOGV("<== baidu_ca_create_service_router: rs = %d", rs);
    return rs;
}

#if defined(DUER_UDP_REPORTER)
DUER_LOC_IMPL duer_status_t baidu_ca_create_reporter(baidu_ca_t* ctx)
{
    duer_status_t rs = DUER_ERR_FAILED;
    DUER_LOGV("==> baidu_ca_create_reporter");

    if (!ctx) {
        goto exit;
    }

    if (!ctx->rpt) {
        ctx->rpt = duer_coap_acquire(duer_engine_coap_handler, ctx, ctx->ctx_socket, ctx->uuid);
        if (!ctx->rpt) {
            goto exit;
        }
        duer_coap_set_tx_callback(ctx->rpt, ctx->f_transmit);
    }

    if (ctx->state == DUER_ST_SR_INITIALIZED) {
        rs = duer_coap_connect(ctx->rpt, &ctx->addr_rpt, ctx->cert, ctx->cert_length);

        if (rs == DUER_OK) {
            ctx->state = DUER_ST_RPT_CONNECTED;
        }
    }

    if (ctx->state == DUER_ST_RPT_CONNECTED) {
#if defined(DUER_READ_TIMEOUT) && (DUER_READ_TIMEOUT > 0)
        duer_coap_set_read_timeout(ctx->rpt, DUER_READ_TIMEOUT);
#endif
        ctx->state = DUER_ST_RPT_INITIALIZED;
        rs = DUER_OK;
    }

exit:
    DUER_LOGV("<== baidu_ca_create_reporter: rs = %d", rs);
    return rs;
}
#endif

DUER_LOC_IMPL duer_status_t baidu_ca_run(baidu_ca_t* ctx)
{
    duer_status_t rs = DUER_ERR_FAILED;

    if (!ctx) {
        goto exit;
    }

    DUER_LOGV("==> baidu_ca_run: ctx = %p, state = %d", ctx, ctx->state);

    switch (ctx->state) {
    case DUER_ST_UNREGISTERED:
        DUER_LOGD("ca status: %d", ctx->state);
        ctx->state = DUER_ST_REGISTER_CONNECTED;
        rs = DUER_OK;
        break;
    case DUER_ST_REGISTER_CONNECTED:
#if defined(DUER_LWM2M_REGISTER)
        rs = baidu_ca_register(ctx);
        break;
#else
        DUER_LOGD("ca status: %d", ctx->state);
        ctx->state = DUER_ST_REGISTERED;
        rs = DUER_OK;
        break;
#endif

    case DUER_ST_REGISTERED:
        DUER_LOGD("ca status: %d", ctx->state);
        ctx->state = DUER_ST_SR_CONNECTING;
        rs = DUER_OK;
        break;

    case DUER_ST_SR_CONNECTING:
    case DUER_ST_SR_DATA_SENDING:
    case DUER_ST_SR_DATA_RECEIVING:
        DUER_LOGD("ca status: %d", ctx->state);
        rs = baidu_ca_create_service_router(ctx);
        break;

    case DUER_ST_SR_INITIALIZED:
#if defined(DUER_UDP_REPORTER)
        rs = baidu_ca_create_reporter(ctx);
        break;
#endif

    case DUER_ST_RPT_INITIALIZED:
        DUER_LOGD("ca status: DUER_ST_RPT_INITIALIZED -> DUER_ST_STARTED");
        ctx->state = DUER_ST_STARTED;
        duer_ds_log_ca_start_state_change(DUER_ST_RPT_INITIALIZED, DUER_ST_STARTED);
        rs = DUER_OK;
        break;
    case DUER_ST_STOPED:
        rs = DUER_ERR_FAILED;
        break;
    case DUER_ST_STARTED:
        rs = DUER_OK;
        break;

    default:
        // do noting
        break;
    }

    DUER_LOGV("<== baidu_ca_run: rs = %d", rs);
exit:
    return rs;
}

DUER_EXT_IMPL duer_handler baidu_ca_acquire(duer_transevt_func soc_ctx)
{
    baidu_ca_t* hdlr = NULL;
    DUER_LOGV("==> baidu_ca_acquire");
    DUER_MEMDBG_USAGE();
    hdlr = DUER_MALLOC(sizeof(baidu_ca_t));

    if (!hdlr) {
        DUER_LOGE("alloc failed...");
        DUER_DS_LOG_CA_MEMORY_OVERFLOW();
        baidu_ca_release(hdlr);
        return NULL;
    }

    duer_coap_trace_enable();
    DUER_MEMSET(hdlr, 0, sizeof(baidu_ca_t));

    hdlr->state = DUER_ST_STOPED;
    hdlr->mutex = duer_mutex_create();
    hdlr->gen_token = 0xC000;
    hdlr->ctx_socket = soc_ctx;

    DUER_LOGV("<== baidu_ca_acquire: hdlr = %p", hdlr);
    return hdlr;
}

DUER_EXT duer_status_t baidu_ca_add_resources(duer_handler hdlr,
                                              const duer_res_t list_res[], duer_size_t list_res_size)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_status_t rs = DUER_ERR_FAILED;

    if (ctx->state >= DUER_ST_SR_INITIALIZED) {
        duer_u8_t i = 0;

        while (i < list_res_size) {
            duer_coap_resource_add(ctx->sr, list_res + i);
            i++;
        }

        rs = DUER_OK;
        goto exit;
    }
    //Note: if exccute here, the path/static data should be something with long-time life
    //      e.g. static chars; or try to add resource after the CA started.
    DUER_LOGW("add_resource at CA not started, all the content(static_data, path)should be static!");
    if (ctx->res_list == NULL) {
        ctx->res_capa = (list_res_size / DUER_RES_INC_COUNT + 1) * DUER_RES_INC_COUNT;
        ctx->res_list = DUER_MALLOC(sizeof(duer_res_t) * ctx->res_capa);
        ctx->res_size = 0;
    } else if (ctx->res_capa < ctx->res_size + list_res_size) {
        ctx->res_capa =
            ((ctx->res_size + list_res_size) / DUER_RES_INC_COUNT + 1) * DUER_RES_INC_COUNT;
        ctx->res_list = (duer_res_t*)DUER_REALLOC(ctx->res_list,
                                                  sizeof(duer_res_t) * ctx->res_capa);
    }

    if (ctx->res_list == NULL) {
        DUER_LOGE("    stored the resource failed...");
        DUER_DS_LOG_CA_MEMORY_OVERFLOW();
        goto exit;
    }

    DUER_MEMCPY(ctx->res_list + ctx->res_size, list_res,
                sizeof(duer_res_t) * list_res_size);
    ctx->res_size += list_res_size;
    rs = DUER_OK;
exit:
    return rs;
}

duer_status_t baidu_ca_unload_configuration(duer_handler hdlr)
{
    baidu_ca_t *ctx = (baidu_ca_t *)hdlr;
    ctx->cert = NULL;
    ctx->cert_length = 0;

    if (ctx->conf) {
        duer_conf_destroy(ctx->conf);
        ctx->conf = NULL;
    }
    return DUER_OK;
}

DUER_EXT_IMPL duer_status_t baidu_ca_load_configuration(duer_handler hdlr, const void *data, duer_size_t size)
{
    baidu_ca_t *ctx = (baidu_ca_t *)hdlr;
    duer_status_t rs = DUER_ERR_FAILED;
    const char *string_addr = NULL;

    if (!ctx) {
        DUER_LOGE("invalid handler with null");
        goto error;
    }

    if (ctx->conf) {
        duer_conf_destroy(ctx->conf);
        ctx->conf = NULL;
    }

    ctx->conf = duer_conf_create(data, size);
    if (!ctx->conf) {
        DUER_LOGE("    create the configuation failed...");
        goto error;
    }

    string_addr = duer_conf_get_string(ctx->conf, DUER_CONF_KEY_UUID);
    if (string_addr) {
        ctx->uuid = DUER_MALLOC(DUER_STRLEN(string_addr) + 1);
    }
    if (!string_addr || !ctx->uuid) {
        DUER_LOGE("    obtain the uuid failed...");
        goto error;
    } else {
        DUER_MEMCPY(ctx->uuid, string_addr, DUER_STRLEN(string_addr) + 1);
    }

    string_addr = duer_conf_get_string(ctx->conf, DUER_CONF_KEY_TOKEN);
    if (string_addr) {
        ctx->token = DUER_MALLOC(DUER_STRLEN(string_addr) + 1);
    }
    if (!string_addr || !ctx->token) {
        DUER_LOGE("    obtain the token failed...");
        goto error;
    } else {
        DUER_MEMCPY(ctx->token, string_addr, DUER_STRLEN(string_addr) + 1);
    }

    string_addr = duer_conf_get_string(ctx->conf, DUER_CONF_KEY_BINDTOKEN);
    if (string_addr) {
        ctx->bindToken = DUER_MALLOC(DUER_STRLEN(string_addr) + 1);
    }
    if (!string_addr || !ctx->bindToken) {
        DUER_LOGE("    obtain the bindToken failed...");
        goto error;
    } else {
        DUER_MEMCPY(ctx->bindToken, string_addr, DUER_STRLEN(string_addr) + 1);
    }

    ctx->cert = duer_conf_get_string(ctx->conf, DUER_CONF_KEY_CERT);
    if (!ctx->cert) {
        DUER_LOGE("    obtain the cert failed...");
        goto error;
    }
    ctx->cert_length = DUER_STRLEN(ctx->cert) + 1;


    string_addr = duer_conf_get_string(ctx->conf, DUER_CONF_KEY_HOST);
    if (string_addr) {
        ctx->serverAddr = DUER_MALLOC(DUER_STRLEN(string_addr) + 1);
    }
    if (!string_addr || !ctx->serverAddr) {
        DUER_LOGE("    obtain the hostname failed...");
        goto error;
    } else {
        DUER_MEMCPY(ctx->serverAddr, string_addr, DUER_STRLEN(string_addr) + 1);
    }

    // cuid may be not exist
    string_addr = (char *)duer_conf_get_string(ctx->conf, DUER_CONF_KEY_CUID);
    if (string_addr) {
        ctx->cuid = DUER_MALLOC(DUER_STRLEN(string_addr) + 1);
    } else {
        ctx->cuid = NULL;
    }
    if (string_addr && !ctx->cuid) {
        DUER_LOGE("    malloc the cuid failed...");
        goto error;
    } else {
        if (ctx->cuid) {
            DUER_MEMCPY(ctx->cuid, string_addr, DUER_STRLEN(string_addr) + 1);
            DUER_LOGI("profile include cuid:%s", ctx->cuid);
        }
    }

#if defined(DUER_LWM2M_REGISTER)
    ctx->addr_reg.host = ctx->serverAddr;
    ctx->addr_reg.host_size = DUER_STRLEN(ctx->serverAddr);
    ctx->addr_reg.port = duer_conf_get_ushort(ctx->conf, DUER_CONF_KEY_PORT_REG);
    ctx->addr_reg.type = DUER_PROTO_UDP;
#endif

#if defined(DUER_UDP_REPORTER)
    ctx->addr_sr.host = SR_HOST;
    ctx->addr_sr.host_size = DUER_STRLEN(SR_HOST);
    ctx->addr_sr.port = SR_PORT;
#else
    ctx->addr_sr.host = ctx->serverAddr;
    ctx->addr_sr.host_size = DUER_STRLEN(ctx->serverAddr);
    ctx->addr_sr.port = duer_conf_get_ushort(ctx->conf, DUER_CONF_KEY_PORT_RPT);
#endif
    ctx->addr_sr.type = DUER_PROTO_TCP;

#if defined(DUER_UDP_REPORTER)
    ctx->addr_rpt.host = ctx->serverAddr;
    ctx->addr_rpt.host_size = DUER_STRLEN(ctx->serverAddr);
    ctx->addr_rpt.port = duer_conf_get_ushort(ctx->conf, DUER_CONF_KEY_PORT_RPT);
    ctx->addr_rpt.type = DUER_PROTO_UDP;
#endif

    if (ctx->report_query) {
        DUER_FREE(ctx->report_query);
        ctx->report_query = NULL;
    }

    ctx->report_query_len = DUER_STRLEN(DUER_RPT_QUERY_UUID)
                            + DUER_STRLEN(ctx->uuid)
                            + 1  // '&'
                            + DUER_STRLEN(DUER_RPT_QUERY_TOKEN)
                            + DUER_STRLEN(ctx->token);
    ctx->report_query = DUER_MALLOC(ctx->report_query_len + 1);
    if (!ctx->report_query) {
        DUER_DS_LOG_CA_MEMORY_OVERFLOW();
        goto error;
    }
    DUER_SNPRINTF(
        ctx->report_query,
        ctx->report_query_len + 1,
        DUER_RPT_QUERY_UUID"%s&"DUER_RPT_QUERY_TOKEN"%s",
        ctx->uuid,
        ctx->token);

    rs = DUER_OK;

error:

    return rs;
}

DUER_EXT_IMPL duer_status_t baidu_ca_do_start(duer_handler hdlr)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_status_t rs = DUER_ERR_FAILED;
    DUER_LOGV("==> baidu_ca_do_start");

    if (!ctx) {
        goto exit;
    }

    duer_mutex_lock(ctx->mutex);
    rs = DUER_OK;

    while (ctx->state != DUER_ST_STARTED && rs == DUER_OK) {
        DUER_LOGD("ca status: %d", ctx->state);
        rs = baidu_ca_run(ctx);
    }

    duer_mutex_unlock(ctx->mutex);
exit:
    DUER_LOGV("<== baidu_ca_do_start: rs = %d", rs);
    return rs;
}

DUER_EXT_IMPL duer_status_t baidu_ca_start(duer_handler hdlr)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_status_t rs = DUER_ERR_FAILED;
    DUER_LOGV("==> baidu_ca_start");

    if (!ctx) {
        goto exit;
    }

    if (ctx->state == DUER_ST_STOPED) {
        DUER_LOGD("ca status: DUER_ST_STOPED -> DUER_ST_UNREGISTERED");
        ctx->state = DUER_ST_UNREGISTERED;
    }

    if (ctx->state == DUER_ST_REGISTER_FAIL) {
        rs = DUER_ERR_REG_FAIL;
    } else {
        rs = baidu_ca_do_start(hdlr);
    }

exit:
    DUER_LOGV("<== baidu_ca_start: rs = %d", rs);
    return rs;
}

DUER_EXT_IMPL duer_bool baidu_ca_is_started(duer_handler hdlr)
{
    baidu_ca_t *ctx = (baidu_ca_t *)hdlr;
    return (ctx != NULL && ctx->state == DUER_ST_STARTED) ? DUER_TRUE : DUER_FALSE;
}

DUER_EXT_IMPL duer_bool baidu_ca_is_stopped(duer_handler hdlr)
{
    baidu_ca_t *ctx = (baidu_ca_t *)hdlr;
    return (ctx == NULL || ctx->state == DUER_ST_STOPED) ? DUER_TRUE : DUER_FALSE;
}

DUER_EXT_IMPL duer_status_t baidu_ca_report_set_response_callback(
    duer_handler hdlr, duer_notify_f f_response, duer_context context)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;

    if (ctx) {
        ctx->f_response = f_response;
        ctx->ctx_response = context;
        return DUER_OK;
    }

    return DUER_ERR_FAILED;
}

DUER_EXT duer_status_t baidu_ca_report_set_data_tx_callback(duer_handler hdlr, duer_transmit_f f_transmit) {

    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;

    if (ctx) {
        ctx->f_transmit = f_transmit;
        return DUER_OK;
    }

    return DUER_ERR_FAILED;
}

static duer_msg_t* baidu_ca_build_coap_message()
{
    duer_msg_t* msg = DUER_MALLOC(sizeof(duer_msg_t)); // ~= 36

    if (msg) {
        DUER_MEMSET(msg, 0, sizeof(duer_msg_t));

        msg->create_timestamp = duer_timestamp();
    }
    return msg;
}

DUER_EXT duer_msg_t* baidu_ca_build_report_message(duer_handler hdlr,
                                                   duer_bool confirmable)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_msg_t* msg = NULL;

    do {
        if (!ctx) {
            break;
        }
        msg = baidu_ca_build_coap_message();

        if (!msg) {
            DUER_DS_LOG_CA_MEMORY_OVERFLOW();
            break;
        }

        msg->msg_type =
            confirmable == DUER_FALSE ? DUER_MSG_TYPE_NON_CONFIRMABLE : DUER_MSG_TYPE_CONFIRMABLE;
        msg->msg_code = DUER_MSG_REQ_POST;
        msg->path = (duer_u8_t *)DUER_REPORT_PATH;
        msg->path_len = DUER_STRLEN(DUER_REPORT_PATH);
        msg->query = (duer_u8_t*)ctx->report_query;
        msg->query_len = ctx->report_query_len;

        msg->token_len = sizeof(ctx->gen_token);
        msg->token = DUER_MALLOC(msg->token_len);

        if (msg->token) {
            DUER_MEMCPY(msg->token, &ctx->gen_token, msg->token_len);
        } else {
            DUER_DS_LOG_CA_MEMORY_OVERFLOW();
        }

        ctx->gen_token++;

    } while (0);

    return msg;
}

DUER_EXT duer_msg_t* baidu_ca_build_response_message(duer_handler hdlr,
                                                     const duer_msg_t* msg, duer_u8_t msg_code)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_msg_t* rs = NULL;
    const char *token = NULL;

    do {
        if (!ctx || !msg) {
            DUER_LOGE("paramter error: ctx = %p, msg = %p", ctx, msg);
            break;
        }

        if (msg_code != DUER_MSG_EMPTY_MESSAGE) { // empty message don't need token
            if (msg->token_len > 0 && msg->token) {
                token = DUER_MALLOC(msg->token_len);
                if (!token) {
                    DUER_DS_LOG_CA_MEMORY_OVERFLOW();
                    break;
                }
            }
        }

        rs = baidu_ca_build_coap_message();
        if (!rs) {
            DUER_DS_LOG_CA_MEMORY_OVERFLOW();
            break;
        }

        if (msg->msg_type == DUER_MSG_TYPE_CONFIRMABLE) {
            rs->msg_type = DUER_MSG_TYPE_ACKNOWLEDGEMENT;
            rs->msg_id = msg->msg_id;
        } else {
            rs->msg_type = msg->msg_type;
        }

        // DUER_MSG_RSP_INVALID means a seperate response
        if (msg_code != DUER_MSG_RSP_INVALID) {
            rs->msg_code = msg_code;

            if (token) {
                rs->token_len = msg->token_len;
                rs->token = (duer_u8_t*)token;

                DUER_MEMCPY(rs->token, msg->token, rs->token_len);
            }
        }

    } while (0);

    if (!rs) {
        if (token) {
            DUER_FREE((char*)token);
            token = NULL;
        }
    }

    return rs;
}

DUER_EXT duer_msg_t* baidu_ca_build_seperate_response_message(duer_handler hdlr,
                                                              const char *ptoken,
                                                              duer_size_t token_len,
                                                              int msg_code,
                                                              duer_bool confirmable)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_msg_t* rs = NULL;
    const char *token = NULL;

    do {
        if (!ctx) {
            DUER_LOGE("paramter error: ctx = %p", ctx);
            break;
        }

        if (token_len > 0 && ptoken) {
            token = DUER_MALLOC(token_len);
            if (!token) {
                DUER_DS_LOG_CA_MEMORY_OVERFLOW();
                break;
            }
        } else {
            DUER_LOGI("token should not be null!!");
            break;
        }

        rs = baidu_ca_build_coap_message();
        if (!rs) {
            DUER_DS_LOG_CA_MEMORY_OVERFLOW();
            break;
        }

        rs->msg_type =
            confirmable == DUER_FALSE ? DUER_MSG_TYPE_NON_CONFIRMABLE : DUER_MSG_TYPE_CONFIRMABLE;

        rs->msg_code = msg_code;

        if (token) {
            rs->token_len = token_len;
            rs->token = (duer_u8_t*)token;

            DUER_MEMCPY(rs->token, ptoken, rs->token_len);
        }

    } while (0);

    if (!rs) {
        if (token) {
            DUER_FREE((char*)token);
            token = NULL;
        }
    }

    return rs;
}

/*
 * Release the message that generated by baidu_ca_build_XXXX_message.
 *
 * @Param hdlr, in, the handler will be operated
 * @Param msg, in, the message that remote requested
 */
DUER_EXT void baidu_ca_release_message(duer_handler hdlr, duer_msg_t* msg)
{
    if (msg) {
        if (msg->token) {
            DUER_FREE(msg->token);
            msg->token = NULL;
        }

        DUER_FREE(msg);
    }
}

DUER_EXT duer_status_t baidu_ca_send_data(duer_handler hdlr, const duer_msg_t* msg,
                                          const duer_addr_t* addr)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_status_t rs = DUER_ERR_FAILED;
    DUER_LOGV("==> baidu_ca_send_data");
#if defined(DUER_UDP_REPORTER)

    if (ctx->state >= DUER_ST_SR_INITIALIZED
        && baidu_ca_match_address(&(ctx->addr_sr), addr))
#endif
    {
        DUER_LOGV("    ctx->sr matched: %p", addr);
        rs = duer_coap_send(ctx->sr, msg);
        goto exit;
    }

#if defined(DUER_UDP_REPORTER)

    if (ctx->state >= DUER_ST_RPT_INITIALIZED
        && (!addr || baidu_ca_match_address(&(ctx->addr_rpt), addr))) {
        duer_mutex_lock(ctx->mutex);
        DUER_LOGV("    ctx->rpt matched: %p", addr);
        rs = duer_coap_send(ctx->rpt, msg);
        duer_mutex_unlock(ctx->mutex);
        goto exit;
    }


    rs = baidu_ca_do_start(hdlr);

    if (rs < DUER_OK) {
        DUER_LOGD("    start failed...");
        goto exit;
    }
#endif
exit:
    DUER_LOGV("<== baidu_ca_send_data: rs = %d", rs);
    DUER_MEMDBG_USAGE();
    return rs;
}

DUER_EXT duer_status_t baidu_ca_send_data_directly(duer_handler hdlr, const void *data,
                                                   duer_size_t size, const duer_addr_t* addr)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_status_t rs = DUER_ERR_FAILED;
    DUER_LOGV("==> baidu_ca_send_data");
#if defined(DUER_UDP_REPORTER)

    if (ctx->state >= DUER_ST_SR_INITIALIZED
        && baidu_ca_match_address(&(ctx->addr_sr), addr))
#endif
    {
        DUER_LOGV("    ctx->sr matched: %p", addr);
        rs = duer_coap_send_data(ctx->sr, data, size);
        goto exit;
    }

#if defined(DUER_UDP_REPORTER)

    if (ctx->state >= DUER_ST_RPT_INITIALIZED
        && (!addr || baidu_ca_match_address(&(ctx->addr_rpt), addr))) {
        duer_mutex_lock(ctx->mutex);
        DUER_LOGV("    ctx->rpt matched: %p", addr);
        rs = duer_coap_send_data(ctx->rpt, data, size);
        duer_mutex_unlock(ctx->mutex);
        goto exit;
    }

    rs = baidu_ca_do_start(hdlr);

    if (rs < DUER_OK) {
        DUER_LOGD("    start failed...");
        goto exit;
    }
#endif
exit:
    DUER_LOGV("<== baidu_ca_send_data: rs = %d", rs);
    //DUER_MEMDBG_USAGE();
    return rs;
}

DUER_EXT_IMPL duer_status_t baidu_ca_data_available(duer_handler hdlr,
                                                    const duer_addr_t* addr)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_status_t rs = DUER_ERR_FAILED;
    DUER_LOGV("==> baidu_ca_data_available");

    if (ctx->state >= DUER_ST_SR_INITIALIZED
#if defined(DUER_UDP_REPORTER)
        && baidu_ca_match_address(&(ctx->addr_sr), addr)
#endif
       ) {
        DUER_LOGV("    ctx->sr matched: %p", addr);
        rs = duer_coap_data_available(ctx->sr);
    }

#if defined(DUER_UDP_REPORTER)

    if (ctx->state >= DUER_ST_RPT_INITIALIZED && (!addr
                                                  || baidu_ca_match_address(&(ctx->addr_rpt), addr))) {
        duer_mutex_lock(ctx->mutex);
        DUER_LOGV("    ctx->rpt matched: %p", addr);
        rs = duer_coap_data_available(ctx->rpt);
        duer_mutex_unlock(ctx->mutex);
    }

#endif

    if (ctx->state < DUER_ST_STARTED) {
        rs = baidu_ca_do_start(hdlr);

        if (rs < DUER_OK && rs != DUER_ERR_TRANS_WOULD_BLOCK) {
            DUER_LOGW("    start failed...");
        }
    }

    DUER_LOGV("<== baidu_ca_data_available: rs = %d", rs);
    return rs;
}

DUER_EXT duer_status_t baidu_ca_exec(duer_handler hdlr)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_status_t rs = DUER_OK;
    duer_u32_t timestamp = duer_timestamp() / 1000;
    DUER_LOGV("==> baidu_ca_exec");

    if (ctx) {
#if defined(DUER_LWM2M_REGISTER)
        duer_coap_exec(ctx->reg, timestamp);
#endif
#if defined(DUER_UDP_REPORTER)
        duer_coap_exec(ctx->rpt, timestamp);
#endif
        duer_coap_exec(ctx->sr, timestamp);
    }

    DUER_LOGV("<== baidu_ca_exec: rs = %d", rs);
    return rs;
}

DUER_EXT_IMPL duer_status_t baidu_ca_stop(duer_handler hdlr)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    duer_status_t rs = DUER_OK;
    DUER_LOGV("==> baidu_ca_stop");

    if (ctx->state > DUER_ST_STOPED) {
        ctx->state = DUER_ST_STOPED;

        if (ctx->sr) {
            duer_coap_release(ctx->sr);
            ctx->sr = NULL;
        }

#if defined(DUER_UDP_REPORTER)

        if (ctx->rpt) {
            duer_coap_release(ctx->rpt);
            ctx->rpt = NULL;
        }

#endif
#if defined(DUER_LWM2M_REGISTER)

        if (ctx->reg) {
            duer_coap_unregister(ctx->reg);
            duer_coap_release(ctx->reg);
            ctx->reg = NULL;
        }

#endif
    }

    DUER_LOGV("<== baidu_ca_stop: rs = %d", rs);
    return rs;
}

DUER_EXT_IMPL duer_status_t baidu_ca_release(duer_handler hdlr)
{
    baidu_ca_t* ctx = (baidu_ca_t*)hdlr;
    DUER_LOGV("==> baidu_ca_release");

    if (ctx) {
        if (ctx->conf) {
            duer_conf_destroy(ctx->conf);
            ctx->conf = NULL;
        }

        baidu_ca_stop(ctx);

        if (ctx->report_query) {
            DUER_FREE(ctx->report_query);
            ctx->report_query = NULL;
        }

        if (ctx->uuid) {
            DUER_FREE(ctx->uuid);
            ctx->uuid = NULL;
        }

        if (ctx->token) {
            DUER_FREE(ctx->token);
            ctx->token = NULL;
        }

        if (ctx->bindToken) {
            DUER_FREE(ctx->bindToken);
            ctx->bindToken = NULL;
        }

        if (ctx->cuid) {
            DUER_FREE(ctx->cuid);
            ctx->cuid = NULL;
        }

        if (ctx->serverAddr) {
            DUER_FREE(ctx->serverAddr);
            ctx->serverAddr = NULL;
        }

        if (ctx->res_list) {
            DUER_FREE(ctx->res_list);
            ctx->res_list = NULL;
        }

        duer_coap_trace_disable();
        duer_mutex_destroy(ctx->mutex);
        ctx->mutex = NULL;
        DUER_FREE(ctx);
    }

    DUER_MEMDBG_USAGE();
    DUER_LOGV("<== baidu_ca_release");
    return DUER_OK;
}
