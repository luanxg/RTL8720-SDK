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
// Description: The wrapper for user's transport.

#include "lightduer_net_transport.h"
#include "lightduer_lib.h"

typedef struct _baidu_ca_transport_t {
    duer_soc_create_f        f_create;
    duer_soc_connect_f       f_connect;
    duer_soc_recv_f          f_recv;
    duer_soc_recv_timeout_f  f_recv_timeout;
    duer_soc_send_f          f_send;
    duer_soc_close_f         f_close;
    duer_soc_destroy_f       f_destroy;
} duer_transport_t;

DUER_LOC_IMPL duer_transport_t s_duer_transport = {NULL};

DUER_EXT_IMPL void baidu_ca_transport_init(duer_soc_create_f f_create,
                                          duer_soc_connect_f f_conn,
                                          duer_soc_send_f f_send,
                                          duer_soc_recv_f f_recv,
                                          duer_soc_recv_timeout_f f_recv_timeout,
                                          duer_soc_close_f f_close,
                                          duer_soc_destroy_f f_destroy) {
    s_duer_transport.f_create = f_create;
    s_duer_transport.f_connect = f_conn;
    s_duer_transport.f_send = f_send;
    s_duer_transport.f_recv = f_recv;
    s_duer_transport.f_recv_timeout = f_recv_timeout;
    s_duer_transport.f_close = f_close;
    s_duer_transport.f_destroy = f_destroy;
}

DUER_INT_IMPL duer_status_t duer_trans_wrapper_create(duer_trans_ptr trans) {
    if (trans && s_duer_transport.f_create) {
        trans->ctx = s_duer_transport.f_create(trans->transevt_callback);
    }

    return trans && trans->ctx ? DUER_OK : DUER_ERR_FAILED;
}

DUER_INT_IMPL duer_status_t duer_trans_wrapper_connect(duer_trans_ptr trans,
        const duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;

    if (trans && s_duer_transport.f_connect) {
        rs = s_duer_transport.f_connect(trans->ctx, addr);
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_wrapper_send(duer_trans_ptr trans,
                                                 const void* data,
                                                 duer_size_t size,
                                                 const duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;

    if (trans && s_duer_transport.f_send) {
        rs = s_duer_transport.f_send(trans->ctx, data, size, addr);
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_wrapper_recv(duer_trans_ptr trans,
                                                 void* data,
                                                 duer_size_t size,
                                                 duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;

    if (trans) {
        if (s_duer_transport.f_recv) {
            rs = s_duer_transport.f_recv(trans->ctx, data, size, addr);
        } 
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_wrapper_recv_timeout(duer_trans_ptr trans,
                                                         void* data,
                                                         duer_size_t size,
                                                         duer_u32_t timeout,
                                                         duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;

    if (trans) {
        if (s_duer_transport.f_recv_timeout) {
            rs = s_duer_transport.f_recv_timeout(trans->ctx, data, size, timeout, addr);
        }
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_wrapper_close(duer_trans_ptr trans) {
    duer_status_t rs = DUER_ERR_FAILED;

    if (trans && s_duer_transport.f_close) {
        rs = s_duer_transport.f_close(trans->ctx);
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_wrapper_destroy(duer_trans_ptr trans) {
    duer_status_t rs = DUER_ERR_FAILED;

    if (trans && s_duer_transport.f_destroy) {
        rs = s_duer_transport.f_destroy(trans->ctx);
    }

    return rs;
}
