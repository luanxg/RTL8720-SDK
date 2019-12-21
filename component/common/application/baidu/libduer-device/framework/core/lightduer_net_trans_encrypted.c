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
// Description: The encrypted network I/O.
//

#include "lightduer_types.h"
#include "lightduer_net_trans_encrypted.h"

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/timing.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/net.h"
#include "mbedtls/debug.h"

#include "lightduer_lib.h"
#include "lightduer_timestamp.h"
#include "lightduer_net_transport.h"
#include "lightduer_random.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"


#define DRBG_PERS_CLIENT            "BCA DTLS CLIENT"
#define COMMON_NAME                 "*.iot.baidu.com"

#if defined(MBEDTLS_SSL_PROTO_DTLS)
#define HANDSHAKE_TIMEOUT_MIN       (1000)
#define HANDSHAKE_TIMEOUT_MAX       (4000)
#endif

// Suppress Compiler warning Function declared never referenced
#define SUPPRESS_WARNING(func) (void)(func)

typedef enum _baidu_ca_trans_status_enum {
    DUER_TRANS_ST_DISCONNECTED,
    DUER_TRANS_ST_CONNECTED,
    DUER_TRANS_ST_HANDSHAKED,
} duer_trans_status_e;

typedef struct _duer_trans_encrypted_s {
#ifndef MBEDTLS_DRBG_ALT
    mbedtls_entropy_context         entropy;
    mbedtls_ctr_drbg_context        ctr_drbg;
#endif
    mbedtls_x509_crt                cacert;
    mbedtls_ssl_context             ssl;
    mbedtls_ssl_config              ssl_conf;
    mbedtls_timing_delay_context    timer;

    duer_u8_t                        status;
    duer_u8_t                        ssl_setup_flag;
} duer_trans_encrypted_t, *duer_trans_encrypted_ptr;

DUER_LOC_IMPL duer_status_t duer_trans_mbedtls2status(int status) {
    duer_status_t rs = status;

    if (status == MBEDTLS_ERR_SSL_TIMEOUT) {
        rs = DUER_ERR_TRANS_TIMEOUT;
    } else if (status == MBEDTLS_ERR_SSL_WANT_READ
               || status == MBEDTLS_ERR_SSL_WANT_WRITE) {
        rs = DUER_ERR_TRANS_WOULD_BLOCK;
    } else {
        // do nothing
    }

    if (rs < 0 && rs != DUER_ERR_TRANS_WOULD_BLOCK) {
        DUER_LOGE("mbedtls error: %d(-0x%04x)", status, -status);
    }

    return rs;
}

#if defined(DUER_MBEDTLS_DEBUG) && (DUER_MBEDTLS_DEBUG > 0)
DUER_LOC_IMPL void duer_trans_encrypted_wrap_debug(void* ctx, int level,
        const char* file, int line, const char* msg) {
    duer_debug_print(level, file, line, msg);
}
#endif

DUER_LOC_IMPL int duer_trans_encrypted_wrap_send(void* ctx,
        const unsigned char* buf, size_t len) {
    duer_status_t rs = duer_trans_wrapper_send((duer_trans_ptr)ctx, buf, len, NULL);

    if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
        return MBEDTLS_ERR_SSL_WANT_WRITE;
    } else if (rs < DUER_OK) {
        return MBEDTLS_ERR_NET_SEND_FAILED;
    } else {
        // do nothing
    }

    return rs;
}

DUER_LOC_IMPL int duer_trans_encrypted_wrap_recv(void* ctx, unsigned char* buf,
        size_t len) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_ptr trans = (duer_trans_ptr)ctx;
    rs = duer_trans_wrapper_recv(trans, buf, len, NULL);

    if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    } else if (rs < DUER_OK) {
        return MBEDTLS_ERR_NET_RECV_FAILED;
    } else {
        // do nothing
    }

    return rs;
}

DUER_LOC_IMPL int duer_trans_encrypted_wrap_recv_timeout(void* ctx,
        unsigned char* buf, size_t len, uint32_t timeout) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_ptr trans = (duer_trans_ptr)ctx;
    DUER_LOGV("duer_trans_encrypted_wrap_recv_timeout: timeout = %d", timeout);
    rs = duer_trans_wrapper_recv_timeout(trans, buf, len, timeout, NULL);

    if (rs == DUER_ERR_TRANS_WOULD_BLOCK) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    } else if (rs == DUER_ERR_TRANS_TIMEOUT) {
        return MBEDTLS_ERR_SSL_TIMEOUT;
    } else if (rs < DUER_OK) {
        return MBEDTLS_ERR_NET_RECV_FAILED;
    } else {
        // do nothing
    }

    return rs;
}

#ifndef MBEDTLS_DRBG_ALT
DUER_LOC_IMPL int duer_trans_encrypted_poll(void* ctx, unsigned char* output,
        size_t len, size_t* olen) {
    uint16_t i;
    for (i = 0; i < len; i++) {
        output[i] = duer_random() & 0xFF;
    }

    *olen = len;
    DUER_LOGD("duer_trans_encrypted_poll: len = %d", len);
    return 0;
}
#endif

DUER_LOC_IMPL unsigned long duer_trans_encrypted_get_timer(
    struct mbedtls_timing_hr_time* val, int reset) {
    unsigned long delta;
    duer_u32_t offset = duer_timestamp();
    duer_u32_t* t = (duer_u32_t*)val;

    if (reset) {
        if (t) {
            *t  = offset;
        }

        return 0;
    }

    delta = offset - *t;
    return delta;
}

DUER_LOC_IMPL void duer_trans_encrypted_set_delay(void* data, uint32_t int_ms,
        uint32_t fin_ms) {
    mbedtls_timing_delay_context* ctx = (mbedtls_timing_delay_context*) data;
    ctx->int_ms = int_ms;
    ctx->fin_ms = fin_ms;

    if (fin_ms != 0) {
        duer_trans_encrypted_get_timer(&ctx->timer, 1);
    }
}

DUER_LOC_IMPL int duer_trans_encrypted_get_delay(void* data) {
    mbedtls_timing_delay_context* ctx = (mbedtls_timing_delay_context*)data;
    unsigned long elapsed_ms;

    if (ctx->fin_ms == 0) {
        return -1;
    }

    elapsed_ms = duer_trans_encrypted_get_timer(&ctx->timer, 0);

    if (elapsed_ms >= ctx->fin_ms) {
        return 2;
    }

    if (elapsed_ms >= ctx->int_ms) {
        return 1;
    }

    return 0;
}

DUER_LOC_IMPL duer_trans_encrypted_ptr duer_trans_get_context(duer_trans_ptr trans) {
    duer_trans_encrypted_ptr ptr = NULL;
    int rs = 0;
    duer_u32_t use_read_timeout = 0;
    DUER_LOGV("==> duer_trans_get_context: trans = %p", trans);

    if (!trans) {
        goto exit;
    }

    ptr = (duer_trans_encrypted_ptr)trans->secure;

    if (ptr) {
        goto exit;
    }

    ptr = (duer_trans_encrypted_ptr)DUER_MALLOC(sizeof(duer_trans_encrypted_t)); // size ~= 1452

    if (!ptr) {
        goto exit;
    }

    DUER_MEMSET(ptr, 0, sizeof(duer_trans_encrypted_t));
    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_ssl_init(&ptr->ssl);
    mbedtls_ssl_config_init(&ptr->ssl_conf);
    mbedtls_x509_crt_init(&ptr->cacert);
#ifndef MBEDTLS_DRBG_ALT
    mbedtls_ctr_drbg_init(&ptr->ctr_drbg);
    mbedtls_entropy_init(&ptr->entropy);
    SUPPRESS_WARNING(duer_trans_encrypted_poll);
    rs = mbedtls_entropy_add_source(&ptr->entropy, duer_trans_encrypted_poll, NULL, 128, 1);

    if (rs != 0) {
        DUER_LOGE("mbedtls_entropy_add_source failed: %d", rs);
        goto error;
    }

    rs = mbedtls_ctr_drbg_seed(&ptr->ctr_drbg,
                               mbedtls_entropy_func,
                               &ptr->entropy,
                               (const unsigned char*)(DRBG_PERS_CLIENT),
                               sizeof(DRBG_PERS_CLIENT));

    if (rs != 0) {
        DUER_LOGE("mbedtls_ctr_drbg_seed failed: %d", rs);
        goto error;
    }

#endif
    /*
     * 1. Load certificates
     */
    rs = mbedtls_x509_crt_parse(&ptr->cacert,
                                (const unsigned char*)trans->cert,
                                trans->cert_len);

    if (rs != 0) {
        DUER_LOGE("mbedtls_x509_crt_parse failed: %d", rs);
        goto error;
    }

    /*
     * 2. Setup stuff
     */
    rs = mbedtls_ssl_config_defaults(&ptr->ssl_conf,
                                     MBEDTLS_SSL_IS_CLIENT,
                                     trans->addr.type == DUER_PROTO_UDP
                                     ? MBEDTLS_SSL_TRANSPORT_DATAGRAM
                                     : MBEDTLS_SSL_TRANSPORT_STREAM,
                                     MBEDTLS_SSL_PRESET_DEFAULT);

    if (rs != 0) {
        DUER_LOGE("mbedtls_ssl_config_defaults failed: %d", rs);
        goto error;
    }

    /* OPTIONAL is usually a bad choice for security, but makes interop easier
     * in this simplified example, in which the ca chain is hardcoded.
     * Production code should set a proper ca chain and use REQUIRED. */
    mbedtls_ssl_conf_authmode(&ptr->ssl_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    //mbedtls_ssl_conf_authmode(&ssl_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&ptr->ssl_conf, &ptr->cacert, NULL);
#ifndef MBEDTLS_DRBG_ALT
    mbedtls_ssl_conf_rng(&ptr->ssl_conf, mbedtls_ctr_drbg_random, &ptr->ctr_drbg);
#else
    mbedtls_ssl_conf_rng(&ptr->ssl_conf, mbedtls_ctr_drbg_random, NULL);
#endif
    //config-option it by user
    //mbedtls_ssl_conf_read_timeout(&_ssl_conf, REPORT_NET_RECEIVE_TIMEOUT);
#if defined(DUER_MBEDTLS_DEBUG) && (DUER_MBEDTLS_DEBUG > 0)
    SUPPRESS_WARNING(duer_trans_encrypted_wrap_debug);
    mbedtls_ssl_conf_dbg(&ptr->ssl_conf, duer_trans_encrypted_wrap_debug, NULL);
    mbedtls_debug_set_threshold(DUER_MBEDTLS_DEBUG);
#endif
    rs = mbedtls_ssl_setup(&ptr->ssl, &ptr->ssl_conf);

    if (rs != 0) {
        DUER_LOGE("mbedtls_ssl_setup failed: %d", rs);
        goto error;
    }
    ptr->ssl_setup_flag = 1;
    rs = mbedtls_ssl_set_hostname(&ptr->ssl, COMMON_NAME);

    if (rs != 0) {
        DUER_LOGE("mbedtls_ssl_setup failed: %d", rs);
        goto error;
    }

    use_read_timeout = trans->read_timeout != DUER_READ_FOREVER;
    mbedtls_ssl_set_bio(&ptr->ssl, trans,
                        duer_trans_encrypted_wrap_send, duer_trans_encrypted_wrap_recv,
                        use_read_timeout ? duer_trans_encrypted_wrap_recv_timeout : NULL);
    mbedtls_ssl_set_timer_cb(&ptr->ssl,
                             &ptr->timer,
                             duer_trans_encrypted_set_delay,
                             duer_trans_encrypted_get_delay);

    if (use_read_timeout) {
        mbedtls_ssl_conf_read_timeout(&ptr->ssl_conf, trans->read_timeout);
    }
#if defined(MBEDTLS_SSL_PROTO_DTLS)
    mbedtls_ssl_conf_handshake_timeout(&ptr->ssl_conf, HANDSHAKE_TIMEOUT_MIN,
                                       HANDSHAKE_TIMEOUT_MAX);
#endif
    ptr->status = DUER_TRANS_ST_DISCONNECTED;
    trans->secure = ptr;
exit:
    DUER_LOGV("<== duer_trans_get_context: ptr = %p", ptr);
    return ptr;
error:

    if (ptr) {
        DUER_FREE(ptr);
        ptr = NULL;
    }

    goto exit;
}

DUER_LOC_IMPL duer_status_t duer_trans_encrypted_handshake(duer_trans_ptr trans) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_encrypted_ptr ptr = trans ? (duer_trans_encrypted_ptr)trans->secure : NULL;
    DUER_LOGD("==> duer_trans_encrypted_handshake: trans = %p", trans);

    if (!ptr) {
        goto exit;
    }

    rs = mbedtls_ssl_handshake(&ptr->ssl);
    DUER_LOGD("    duer_trans_encrypted_handshake: rs = %d(-0x%04x)", rs, -rs);
    rs = duer_trans_mbedtls2status(rs);

    if (rs == DUER_OK) {
        ptr->status = DUER_TRANS_ST_HANDSHAKED;
    }

exit:
    DUER_LOGV("<== duer_trans_encrypted_handshake: rs = %d", rs);
    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_encrypted_do_connect(duer_trans_ptr trans,
        const duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_encrypted_ptr ptr = NULL;
    DUER_LOGV("==> duer_trans_encrypted_do_connect: trans = %p", trans);
    ptr = duer_trans_get_context(trans);

    if (!ptr) {
        goto exit;
    }

    rs = duer_trans_wrapper_connect(trans, addr);

    if (rs == DUER_OK || rs == DUER_INF_TRANS_IP_BY_HTTP_DNS) {
        ptr->status = DUER_TRANS_ST_CONNECTED;
    }

exit:
    DUER_LOGV("<== duer_trans_encrypted_do_connect: rs = %d", rs);
    return rs;
}

DUER_LOC_IMPL duer_status_t duer_trans_encrypted_do_prepare(duer_trans_ptr trans) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_encrypted_ptr ptr = duer_trans_get_context(trans);
    DUER_LOGV("==> duer_trans_encrypted_do_prepare: trans = %p", trans);

    if (!ptr) {
        DUER_LOGW("    duer_trans_encrypted_do_prepare: ptr is null!!!");
        goto exit;
    }

    switch (ptr->status) {
    case DUER_TRANS_ST_DISCONNECTED:
        rs = duer_trans_encrypted_do_connect(trans, &trans->addr); // trans->addr ? where set this
        break;

    case DUER_TRANS_ST_CONNECTED:
        rs = duer_trans_encrypted_handshake(trans);
        break;

    case DUER_TRANS_ST_HANDSHAKED:
        rs = DUER_OK;
        break;
    default:
        // do nothing
        break;
    }

exit:
    DUER_LOGV("<== duer_trans_encrypted_do_prepare: rs = %d", rs);
    return rs;
}

DUER_LOC_IMPL duer_status_t duer_trans_encrypted_prepare(duer_trans_ptr trans) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_encrypted_ptr ptr = duer_trans_get_context(trans);
    DUER_LOGV("==> duer_trans_encrypted_prepare: trans = %p", trans);

    if (!ptr) {
        DUER_LOGW("    duer_trans_encrypted_prepare: ptr is null!!!");
        goto exit;
    }

    rs = DUER_OK;

    while (ptr->status != DUER_TRANS_ST_HANDSHAKED && rs == DUER_OK) {
        DUER_LOGD("encrypted status: %d", ptr->status);
        rs = duer_trans_encrypted_do_prepare(trans);
    }

exit:
    DUER_LOGV("<== duer_trans_encrypted_prepare: rs = %d", rs);
    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_encrypted_connect(duer_trans_ptr trans,
        const duer_addr_t* addr) {
    duer_status_t rs = DUER_ERR_FAILED;
    duer_trans_encrypted_ptr ptr = NULL;
    DUER_LOGD("==> duer_trans_encrypted_connect: trans = %p", trans);
    ptr = duer_trans_get_context(trans);

    if (!ptr) {
        goto exit;
    }

    if (ptr->status == DUER_TRANS_ST_DISCONNECTED) {
        rs = duer_trans_encrypted_do_connect(trans, addr);

        if (rs < DUER_OK && rs != DUER_INF_TRANS_IP_BY_HTTP_DNS) {
            goto exit;
        }
    }

    rs = duer_trans_encrypted_prepare(trans);
exit:
    DUER_LOGV("<== duer_trans_encrypted_connect: rs = %d", rs);
    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_encrypted_send(duer_trans_ptr trans,
        const void* data,
        duer_size_t size,
        const duer_addr_t* addr) {
    duer_trans_encrypted_ptr ptr = trans ? (duer_trans_encrypted_ptr)trans->secure : NULL;
    int rs = DUER_ERR_FAILED;
    DUER_LOGV("==> duer_trans_encrypted_send: trans = %p, size:%d", trans, size);
    rs = duer_trans_encrypted_prepare(trans);

    if (rs < DUER_OK) {
        DUER_LOGW("    duer_trans_encrypted_send: prepare failed, because rs = %d", rs);
        goto exit;
    }

    rs = mbedtls_ssl_write(&(ptr->ssl), (const unsigned char*)data, size);
    rs = duer_trans_mbedtls2status(rs);
    DUER_LOGV("<== duer_trans_encrypted_send: rs = %d", rs);
exit:
    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_encrypted_recv(duer_trans_ptr trans,
        void* data,
        duer_size_t size,
        duer_addr_t* addr) {
    duer_trans_encrypted_ptr ptr = trans ? (duer_trans_encrypted_ptr)trans->secure : NULL;
    int rs = DUER_ERR_FAILED;
    DUER_LOGV("==> duer_trans_encrypted_recv: trans = %p", trans);
    rs = duer_trans_encrypted_prepare(trans);

    if (rs < DUER_OK) {
        DUER_LOGE("    duer_trans_encrypted_recv: prepare failed, because rs = %d", rs);
        goto exit;
    }

    rs = mbedtls_ssl_read(&(ptr->ssl), (unsigned char*)data, size);
    rs = duer_trans_mbedtls2status(rs);
    DUER_LOGV("<== duer_trans_encrypted_recv: rs = %d", rs);
exit:
    return rs;
}

DUER_INT_IMPL duer_status_t duer_trans_encrypted_recv_timeout(duer_trans_ptr trans,
        void* data,
        duer_size_t size,
        duer_u32_t timeout,
        duer_addr_t* addr) {
    return duer_trans_encrypted_recv(trans, data, size, addr);
}

DUER_INT_IMPL duer_status_t duer_trans_encrypted_close(duer_trans_ptr trans) {
    duer_trans_encrypted_ptr ptr = trans ? (duer_trans_encrypted_ptr)trans->secure : NULL;
    DUER_LOGV("==> duer_trans_encrypted_close: trans = %p", trans);

    if (ptr) {
        if (ptr->ssl_setup_flag == 1) {
            mbedtls_ssl_close_notify(&ptr->ssl);
            mbedtls_ssl_session_reset(&ptr->ssl);
            if (trans) {
                if (trans->ctx) {
                    DUER_LOGD("trans close");
                    duer_trans_wrapper_close(trans);
                }
            }
            mbedtls_x509_crt_free(&ptr->cacert);
            mbedtls_ssl_free(&ptr->ssl);
            mbedtls_ssl_config_free(&ptr->ssl_conf);
    #ifndef MBEDTLS_DRBG_ALT
            mbedtls_ctr_drbg_free(&ptr->ctr_drbg);
            mbedtls_entropy_free(&ptr->entropy);
    #endif
            ptr->ssl_setup_flag = 0;
        }
    }

    DUER_LOGV("<== duer_trans_encrypted_close");
    return DUER_OK;
}

DUER_INT_IMPL duer_status_t duer_trans_encrypted_set_read_timeout(
    duer_trans_ptr trans, duer_u32_t timeout) {
    duer_trans_encrypted_ptr ptr = trans ? (duer_trans_encrypted_ptr)trans->secure : NULL;

    if (ptr) {
        if (timeout != DUER_READ_FOREVER) {
            mbedtls_ssl_conf_read_timeout(&ptr->ssl_conf, timeout);
        }
        return DUER_OK;
    }

    return DUER_ERR_FAILED;
}
