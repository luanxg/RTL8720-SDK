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
// Description: The network adapter.

#include "lightduer_net_transport_wrapper.h"
#ifndef NET_TRANS_ENCRYPTED_BY_AES_CBC
#include "lightduer_net_trans_encrypted.h"
#else
#include "lightduer_net_trans_aes_cbc_encrypted.h"
#endif
#include "lightduer_lib.h"
#include "lightduer_net_transport.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"

enum _baidu_ca_trans_type_enum {
    DUER_TRANS_NORMAL,
    DUER_TRANS_SECURE,
    DUER_TRANS_TOTAL
};

typedef struct _duer_transport_callbacks {
    duer_status_t (*f_connect)(duer_trans_ptr, const duer_addr_t*);
    duer_status_t (*f_send)(duer_trans_ptr, const void*, duer_size_t, const duer_addr_t*);
    duer_status_t (*f_recv)(duer_trans_ptr, void*, duer_size_t, duer_addr_t*);
    duer_status_t (*f_recv_timeout)(duer_trans_ptr, void*, duer_size_t, duer_u32_t, duer_addr_t*);
    duer_status_t (*f_close)(duer_trans_ptr);
} duer_trans_cbs;

const DUER_LOC duer_trans_cbs s_duer_trans_callbacks[DUER_TRANS_TOTAL] = {
    // DUER_TRANS_NORMAL
    {
        duer_trans_wrapper_connect,
        duer_trans_wrapper_send,
        duer_trans_wrapper_recv,
        duer_trans_wrapper_recv_timeout,
        duer_trans_wrapper_close
    },
#ifndef NET_TRANS_ENCRYPTED_BY_AES_CBC
    // DUER_TRANS_SECURE mbedtls
{
        duer_trans_encrypted_connect,
        duer_trans_encrypted_send,
        duer_trans_encrypted_recv,
        duer_trans_encrypted_recv_timeout,
        duer_trans_encrypted_close
    },
#else
    // DUER_TRANS_SECURE AES-CBC
    {
        duer_trans_aes_cbc_encrypted_connect,
        duer_trans_aes_cbc_encrypted_send,
        duer_trans_aes_cbc_encrypted_recv,
        duer_trans_aes_cbc_encrypted_recv_timeout,
        duer_trans_aes_cbc_encrypted_close
    },
#endif // NET_TRANS_ENCRYPTED_BY_AES_CBC
};

DUER_LOC_IMPL duer_u8_t duer_trans_is_encrypted(duer_trans_ptr ptr) {
    return ptr && ptr->is_encrypted;
}

DUER_LOC const duer_trans_cbs* duer_trans_get_callbacks(duer_trans_ptr ptr) {
    if (ptr) {
        if (duer_trans_is_encrypted(ptr)) {
            return &s_duer_trans_callbacks[DUER_TRANS_SECURE];
        } else {
            return &s_duer_trans_callbacks[DUER_TRANS_NORMAL];
        }
    }

    DUER_LOGE("Param error: ptr is NULL");
    return NULL;
}

DUER_INT_IMPL duer_trans_handler duer_trans_acquire(duer_transevt_func func, const void *key_info) {
    duer_trans_ptr trans = DUER_MALLOC(sizeof(duer_trans_t)); // ~= 44

    if (trans) {
        DUER_MEMSET(trans, 0, sizeof(duer_trans_t));
        trans->transevt_callback = func;
        duer_trans_wrapper_create(trans);
        trans->read_timeout = DUER_READ_FOREVER;
        trans->key_info = key_info;
        trans->is_encrypted = DUER_FALSE;
    }

    DUER_LOGD("duer_trans_acquire: trans = %x", trans);
    return trans;
}

DUER_INT_IMPL duer_status_t duer_trans_set_pk(duer_trans_handler hdlr,
                                           const void* data,
                                           duer_size_t size) {
    duer_trans_ptr trans = (duer_trans_ptr)hdlr;

    if (trans) {
        if (trans->cert) {
            return DUER_OK;
        }

        trans->cert = DUER_MALLOC(size); //size ~= 1209

        if (trans->cert) {
            DUER_MEMCPY(trans->cert, data, size);
            trans->cert_len = size;
            trans->is_encrypted = DUER_TRUE;
        } else {
            trans->cert_len = 0;
        }
    }

    return DUER_OK;
}

duer_status_t duer_trans_unset_pk(duer_trans_handler hdlr)
{
    duer_trans_ptr trans = (duer_trans_ptr)hdlr;

    if (trans) {
        if (trans->cert) {
            DUER_FREE(trans->cert);
            trans->cert = NULL;
            trans->cert_len = 0;
        }
    }
    return DUER_OK;
}

DUER_INT_IMPL duer_status_t duer_trans_set_read_timeout(duer_trans_handler hdlr,
                                                     duer_u32_t timeout) {
    duer_trans_ptr trans = (duer_trans_ptr)hdlr;

    if (trans) {
        trans->read_timeout = timeout;

        if (duer_trans_is_encrypted(trans)) {
#ifndef NET_TRANS_ENCRYPTED_BY_AES_CBC
            duer_trans_encrypted_set_read_timeout(hdlr, timeout);
#else
            duer_trans_aes_cbc_encrypted_set_read_timeout(hdlr, timeout);
#endif
        }

        return DUER_OK;
    }

    return DUER_ERR_FAILED;
}

DUER_INT_IMPL duer_status_t duer_trans_connect(duer_trans_handler hdlr,
                                            const duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_ptr trans = (duer_trans_ptr)hdlr;

    if (trans) {
        trans->addr.type = addr->type;
        // trans->addr.host ???
    }

    const duer_trans_cbs* cbs = duer_trans_get_callbacks(trans);

    if (cbs) {
        rs = cbs->f_connect(trans, addr);
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_send(duer_trans_handler hdlr,
                                         const void* data,
                                         duer_size_t size,
                                         const duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_ptr trans = (duer_trans_ptr)hdlr;
    const duer_trans_cbs* cbs = duer_trans_get_callbacks(trans);

    if (cbs) {
        rs = cbs->f_send(trans, data, size, addr);
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_recv(duer_trans_handler hdlr,
                                         void* data,
                                         duer_size_t size,
                                         duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_ptr trans = (duer_trans_ptr)hdlr;
    const duer_trans_cbs* cbs = duer_trans_get_callbacks(trans);

    if (cbs) {
        rs = cbs->f_recv(trans, data, size, addr);
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_recv_timeout(duer_trans_handler hdlr,
                                         void* data,
                                         duer_size_t size,
                                         duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_ptr trans = (duer_trans_ptr)hdlr;
    const duer_trans_cbs* cbs = duer_trans_get_callbacks(trans);

    if (cbs) {
        rs = cbs->f_recv_timeout(trans, data, size, trans->read_timeout, addr);
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_close(duer_trans_handler hdlr) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_ptr trans = (duer_trans_ptr)hdlr;
    const duer_trans_cbs* cbs = duer_trans_get_callbacks(trans);

    if (cbs) {
        rs = cbs->f_close(trans);
    }

    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_release(duer_trans_handler hdlr) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_ptr trans = (duer_trans_ptr)hdlr;

    if (trans) {
        duer_trans_close(hdlr);

        if (trans->cert) {
            DUER_LOGD("cert release");
            DUER_FREE(trans->cert);
            trans->cert = NULL;
            trans->cert_len = 0;
        }

        if(trans->secure) {
            DUER_LOGD("cert secure");
            DUER_FREE(trans->secure);
        }

        duer_trans_wrapper_destroy(trans);
        DUER_LOGD("duer_trans_release: trans = %x", trans);
        DUER_FREE(trans);
    }

    return DUER_OK;
}
