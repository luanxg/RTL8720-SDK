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
/*
 * Auth: Leliang Zhang(zhangleliang@baidu.com)
 * Desc: device status ca report
 */

#include "lightduer_ds_report_ca.h"
#include "lightduer_log.h"

typedef struct duer_ds_report_ca_status_type {

    duer_u32_t avg_latency;
    duer_u32_t latency_count;
    duer_u32_t s_max_packet;

    duer_u32_t valid_recv;
    duer_u32_t recv_count;
    duer_u32_t r_max_packet;

    duer_u32_t mbedtls;
    duer_u32_t tcp_header;
    duer_u32_t coap;
    duer_u32_t connect_fail;
} duer_ds_report_ca_status_t;

static duer_ds_report_ca_status_t s_ca_status;

duer_status_t duer_ds_report_ca_update_avg_latency(duer_u32_t al)
{
    duer_status_t result = DUER_OK;
    if (!al) {
        return result;
    }
    DUER_LOGV("count:%u, latency:%u, al:%u", s_ca_status.latency_count, s_ca_status.avg_latency, al);
    if (s_ca_status.latency_count) {
        al = s_ca_status.avg_latency * s_ca_status.latency_count + al;
        s_ca_status.avg_latency = al / (s_ca_status.latency_count + 1);
    } else {
        s_ca_status.avg_latency = al;
    }

    ++s_ca_status.latency_count;
    return result;
}

duer_status_t duer_ds_report_ca_update_send_max_packet(duer_u32_t smp)
{
    duer_status_t result = DUER_OK;
    if (smp > s_ca_status.s_max_packet) {
        s_ca_status.s_max_packet = smp;
    }
    return result;
}

duer_status_t duer_ds_report_ca_inc_valid_recv()
{
    duer_status_t result = DUER_OK;
    s_ca_status.valid_recv += 1;
    return result;
}

duer_status_t duer_ds_report_ca_inc_recv_count()
{
    duer_status_t result = DUER_OK;
    s_ca_status.recv_count += 1;
    return result;
}

duer_status_t duer_ds_report_ca_update_recv_max_packet(duer_u32_t rmp)
{
    duer_status_t result = DUER_OK;
    if (rmp > s_ca_status.r_max_packet) {
        s_ca_status.r_max_packet = rmp;
    }
    return result;
}

duer_status_t duer_ds_report_ca_inc_error_mbedtls()
{
    duer_status_t result = DUER_OK;
    s_ca_status.mbedtls += 1;
    return result;
}

duer_status_t duer_ds_report_ca_inc_error_tcpheader()
{
    duer_status_t result = DUER_OK;
    s_ca_status.tcp_header += 1;
    return result;
}

duer_status_t duer_ds_report_ca_inc_error_coap()
{
    duer_status_t result = DUER_OK;
    s_ca_status.coap += 1;
    return result;
}

duer_status_t duer_ds_report_ca_inc_error_connect()
{
    duer_status_t result = DUER_OK;
    s_ca_status.connect_fail += 1;
    return result;
}

baidu_json* duer_ds_report_ca_status(void)
{
    baidu_json *status = baidu_json_CreateObject();
    if (status == NULL) {
        DUER_LOGE("status json create fail");
        return NULL;
    }

    baidu_json *content = baidu_json_CreateObject();
    if (content == NULL) {
        DUER_LOGE("content json create fail");
        goto exit;
    }

    baidu_json *send = baidu_json_CreateObject();
    if (send == NULL) {
        DUER_LOGE("send json create fail");
        goto exit;
    }
    baidu_json_AddNumberToObjectCS(send, "avg_latency", s_ca_status.avg_latency);
    baidu_json_AddNumberToObjectCS(send, "max_packet", s_ca_status.s_max_packet);

    baidu_json_AddItemToObjectCS(content, "send", send);

    baidu_json *recv = baidu_json_CreateObject();
    if (recv == NULL) {
        DUER_LOGE("recv json create fail");
        goto exit;
    }
    baidu_json_AddNumberToObjectCS(recv, "valid_recv", s_ca_status.valid_recv);
    baidu_json_AddNumberToObjectCS(recv, "recv_count", s_ca_status.recv_count);
    baidu_json_AddNumberToObjectCS(recv, "max_packet", s_ca_status.r_max_packet);

    baidu_json_AddItemToObject(content, "recv", recv);
    if (s_ca_status.mbedtls
            || s_ca_status.tcp_header
            || s_ca_status.coap) {
        baidu_json *error = baidu_json_CreateObject();
        if (error == NULL) {
            DUER_LOGE("error json create fail");
            goto exit;
        }
        if (s_ca_status.mbedtls) {
            baidu_json_AddNumberToObjectCS(error, "mbedtls", s_ca_status.mbedtls);
        }
        if (s_ca_status.tcp_header) {
            baidu_json_AddNumberToObjectCS(error, "tcp_header", s_ca_status.tcp_header);
        }
        if (s_ca_status.coap) {
            baidu_json_AddNumberToObjectCS(error, "coap", s_ca_status.coap);
        }

        baidu_json_AddItemToObjectCS(content, "error", error);
    }

exit:
    if (content) {
        baidu_json_AddItemToObjectCS(status, "ca_status", content);
    }
    return status;
}
