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
// Description: Adapter between nsdl and Baidu CA CoAP.

#include "lightduer_nsdl_adapter.h"
#include "sn_nsdl_lib.h"
#if MBED_CLIENT_C_VERSION > 29999
#include "sn_coap_protocol_internal.h"
#endif

DUER_INT_IMPL void duer_nsdl_address_set(sn_nsdl_addr_s* target,
                                       const duer_addr_t* source) {
    if (target && source) {
        target->addr_len = source->host_size;
        target->addr_ptr = source->host;
        target->port = source->port;
        target->type = SN_NSDL_ADDRESS_TYPE_IPV4;
    }
}

DUER_INT_IMPL void duer_nsdl_header_set(sn_coap_hdr_s* target,
                                      sn_coap_options_list_s* opt,
                                      const duer_msg_t* source) {
    if (target && source) {
        DUER_MEMSET(target, 0, sizeof(sn_coap_hdr_s));
        target->msg_code = (sn_coap_msg_code_e)source->msg_code;
        target->msg_id = source->msg_id;
        target->msg_type = (sn_coap_msg_type_e)source->msg_type;
        target->payload_len = source->payload_len;
        target->payload_ptr = source->payload;
        target->token_len = source->token_len;
        target->token_ptr = (uint8_t*)source->token;
        target->uri_path_len = source->path_len;
        target->uri_path_ptr = source->path;
#if MBED_CLIENT_C_VERSION > 29999
        target->content_format = COAP_CT_NONE;
#endif

        if (opt && source->query != NULL && source->query_len > 0) {
            DUER_MEMSET(opt, 0, sizeof(sn_coap_options_list_s));
#if MBED_CLIENT_C_VERSION > 29999
            opt->max_age = COAP_OPTION_MAX_AGE_DEFAULT;
            opt->uri_port = COAP_OPTION_URI_PORT_NONE;
            opt->observe = COAP_OBSERVE_NONE;
            opt->accept = COAP_CT_NONE;
            opt->block2 = COAP_OPTION_BLOCK_NONE;
            opt->block1 = COAP_OPTION_BLOCK_NONE;
#endif
            opt->uri_query_len = source->query_len;
            opt->uri_query_ptr = source->query;
            target->options_list_ptr = opt;
        }
    }
}

DUER_INT void duer_coap_address_set(duer_addr_t* target,
                                  const sn_nsdl_addr_s* source) {
    if (target && source) {
        target->host_size = source->addr_len;
        target->host = source->addr_ptr;
        target->port = source->port;
        target->type = (duer_u8_t) - 1;
    }
}

DUER_INT void duer_coap_header_set(duer_msg_t* target,
                                 const sn_coap_hdr_s* source) {
    if (target && source) {
        DUER_MEMSET(target, 0, sizeof(duer_msg_t));
        target->msg_code = source->msg_code;
        target->msg_id = source->msg_id;
        target->msg_type = source->msg_type;
        target->payload_len = source->payload_len;
        target->payload = source->payload_ptr;
        target->token = source->token_ptr;
        target->token_len = source->token_len;
        target->path_len = source->uri_path_len;
        target->path = source->uri_path_ptr;
        sn_coap_options_list_s* opt = source->options_list_ptr;

        if (opt != NULL && opt->uri_query_ptr != NULL && opt->uri_query_len > 0) {
            target->query_len = opt->uri_query_len;
            target->query = opt->uri_query_ptr;
        }
    }
}
