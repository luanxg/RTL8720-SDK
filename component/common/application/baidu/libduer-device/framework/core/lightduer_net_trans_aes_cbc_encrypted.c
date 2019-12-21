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
// Author: Leliang Zhang(zhangleliang@baidu.com)
//
// Description: The encrypted network I/O.
//
#ifdef NET_TRANS_ENCRYPTED_BY_AES_CBC

#include "lightduer_net_trans_aes_cbc_encrypted.h"

#include "lightduer_aes.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_memory.h"
#include "lightduer_net_util.h"
#include "lightduer_net_transport.h"

/*
    ----------------------------------------------------------
    |MAGIC NUMBER | LEN_1 | LEN_2 | raw data | encrypted data|
    ----------------------------------------------------------
    LEN_1 : 4byte, length of 1 + len(raw data) + len(encrypted data)[>16bytes]
    LEN_2 : 1byte, length of raw data(max 256)
*/

#define DUER_COAP_TCP_HDR  (0xbeefdead) // see the value in lightduer_coap.c
#define DUER_BDCAEC_MN 0xBDCAEC01 // baidu connent agent encrypted communicatiion magic number
#define DUER_BDCAEC_MN_SIZE sizeof(DUER_BDCAEC_MN)
#define DUER_BDCAEC_LEN_1_SIZE 4
#define DUER_BDCAEC_LEN_2_SIZE 1

//#define DEV_DEBUG_AES
#ifdef DEV_DEBUG_AES
#define DUER_AES_PRINT printf
#else
#define DUER_AES_PRINT DUER_LOGD
#endif

//#define DEV_INFO_AES
#ifdef DEV_INFO_AES
#define DUER_AES_INFO_PRINT DUER_LOGI
#else
#define DUER_AES_INFO_PRINT DUER_LOGD
#endif

typedef struct _duer_trans_aes_cbc_encrypted_header_s {
    duer_u32_t       magic_num;
    duer_u32_t       msg_size;
    duer_u8_t        raw_data_size;
    duer_u8_t        raw_data[0];
} duer_trans_aes_cbc_encrypted_header_t;

// convert the string representation of bindToken to binary format
// e.g. 8cd34facea95d5491b2cb5fbacacb0f0 -> 0x8cd34facea95d5491b2cb5fbacacb0f0
static void convert_bindtoken_to_key(char bind_token[32], unsigned char key[16]) {
    for (int i = 0; i < 16; ++i) {
        int value = 0;
        char str[3] = {bind_token[2*i], bind_token[2*i + 1], '\0'};
        sscanf(str, "%x", &value);
        key[i] = (unsigned char)value;
    }
}

duer_status_t duer_trans_aes_cbc_encrypted_connect(duer_trans_ptr trans,
                                           const duer_addr_t *addr)
{
    duer_status_t rs = DUER_ERR_FAILED;
    DUER_LOGV("==> duer_trans_encrypted_do_connect: trans = %p", trans);

    rs = duer_trans_wrapper_connect(trans, addr);

    if (rs == DUER_OK) {
        duer_aes_context ptr = (duer_aes_context)trans->secure;
        if (ptr) {
            duer_aes_context_destroy(ptr);
            trans->secure = NULL;
            ptr = NULL;
        }
        ptr = duer_aes_context_init();
        do {
            if (ptr == NULL) {
                DUER_LOGE("aes context init failed!");
                rs = DUER_ERR_TRANS_INTERNAL_ERROR;
                break;
            }
            unsigned char key[16];
            DUER_LOGV("str_key: %s", trans->cert);
            convert_bindtoken_to_key(trans->cert, key);
            rs = duer_aes_setkey(ptr, key, sizeof(key) * 8);
            if (rs != DUER_OK) {
                DUER_LOGE("duer_aes_setkey failed! rs:%d", rs);
                rs = DUER_ERR_TRANS_INTERNAL_ERROR;
                break;
            }
            unsigned char iv[16];
            DUER_MEMSET(iv, 0, sizeof(iv));//TODO how to get the IV
            rs = duer_aes_setiv(ptr, iv);
            if (rs != DUER_OK) {
                DUER_LOGE("duer_aes_setiv failed, rs:%d!", rs);
                rs = DUER_ERR_TRANS_INTERNAL_ERROR;
                break;
            }
            trans->secure = ptr;
        } while (0);

        if (rs != DUER_OK) {
            duer_aes_context_destroy(ptr);
            ptr = NULL;
            duer_trans_wrapper_close(trans);
        }
    }
    DUER_LOGV("<== duer_trans_encrypted_do_connect: rs = %d", rs);
    return rs;
}

duer_status_t duer_trans_aes_cbc_encrypted_send(duer_trans_ptr trans,
                                        const void* data,
                                        duer_size_t size,
                                        const duer_addr_t* addr)
{
    duer_aes_context ptr = trans ? (duer_aes_context)trans->secure : NULL;
    if (ptr == NULL) {
        DUER_LOGE("invalid paramter");
        return DUER_ERR_TRANS_INTERNAL_ERROR;
    }

    /*
        -------------------------------------------------
        |MAGIC NUMBER | LEN | 14 | UUID | encrypted data|
        -------------------------------------------------
    */

    unsigned char *output = NULL;
    unsigned char *input = NULL;
    duer_status_t rs;
    duer_size_t header_len = 0;
    duer_size_t output_len = 0;
    duer_trans_aes_cbc_encrypted_header_t *p_header = NULL;

    duer_size_t padding_len = size % 16;
    if (padding_len != 0) {
        padding_len = 16 - padding_len;
        input = DUER_MALLOC(size + padding_len);
        if (input == NULL) {
            DUER_LOGE("malloc input failed!");
            rs = DUER_ERR_MEMORY_OVERLOW;
            goto exit;
        }
        DUER_MEMSET(input, 0, size + padding_len);
        DUER_MEMCPY(input, data, size);
    }

    header_len = DUER_BDCAEC_MN_SIZE + DUER_BDCAEC_LEN_1_SIZE + DUER_BDCAEC_LEN_2_SIZE
                             + DUER_STRLEN(trans->key_info);
    output_len = header_len + size + padding_len;
    output = DUER_MALLOC(output_len);
    if (output == NULL) {
        DUER_LOGE("malloc output failed!");
        rs = DUER_ERR_MEMORY_OVERLOW;
        goto exit;
    }

    DUER_MEMSET(output, 0, size);

    p_header = (duer_trans_aes_cbc_encrypted_header_t*)output;
    p_header->magic_num = duer_htonl(DUER_BDCAEC_MN);
    p_header->msg_size = duer_htonl(output_len - DUER_BDCAEC_MN_SIZE - DUER_BDCAEC_LEN_1_SIZE);
    p_header->raw_data_size = DUER_STRLEN(trans->key_info);
#ifdef DEV_DEBUG_AES
    DUER_AES_PRINT("lightduer_net_trans_encrypted.c, output_len:%lu\n", output_len);
    unsigned int *magic_nu = (unsigned int*)output;
    DUER_AES_PRINT("lightduer_net_trans_encrypted.c, magic_num:0x%x \n", duer_htonl(*magic_nu));
    unsigned int *msg_len = (unsigned int*)(output + 4);
    DUER_AES_PRINT("lightduer_net_trans_encrypted.c, msg_len:%d\n", duer_htonl(*msg_len));
    unsigned char *raw_len = output + 8;
    DUER_AES_PRINT("lightduer_net_trans_encrypted.c, raw_len:%d\n", *raw_len);
    DUER_AES_PRINT("lightduer_net_trans_encrypted.c, encrypted_data_len:%d\n",
            duer_htonl(*msg_len) - 1 - *raw_len);
#endif
    DUER_MEMCPY(p_header->raw_data, trans->key_info, DUER_STRLEN(trans->key_info));
    if (input) {
        rs = duer_aes_cbc_encrypt(ptr, size + padding_len, input, output + header_len);
    } else {
        rs = duer_aes_cbc_encrypt(ptr, size, data, output + header_len);
    }

    if (rs < 0) {
        DUER_LOGW("encrypt failed!!rs:%d", rs);
        goto exit;
    }

    rs = duer_trans_wrapper_send(trans, output, output_len, addr);
    DUER_AES_INFO_PRINT("after send: rs:%d", rs);
exit:
    if (output) {
        DUER_FREE(output);
        output = NULL;
    }
    if (input) {
        DUER_FREE(input);
        input = NULL;
    }
    return rs;
}

duer_status_t duer_trans_aes_cbc_encrypted_set_read_timeout(duer_trans_ptr trans,
                                                    duer_u32_t timeout)
{
    DUER_LOGE("no implementation");
    return DUER_ERR_FAILED;
}

duer_status_t duer_trans_aes_cbc_encrypted_recv(duer_trans_ptr trans,
                                        void* data,
                                        duer_size_t size,
                                        duer_addr_t* addr)
{
    duer_status_t rs = DUER_ERR_TRANS_INTERNAL_ERROR;
    duer_size_t need_to_read = size;
    if (size == DUER_SIZEOF_TCPHEADER) {
        DUER_AES_INFO_PRINT("current trans->received_header_bytes: %d [mostly 0]",
                             trans->received_header_bytes);
        if (trans->received_header_bytes == DUER_SIZEOF_TCPHEADER) {//all header already received
            DUER_MEMCPY(data, trans->received_header, size);
            rs = size;
            return rs;
        } else if (trans->received_header_bytes < DUER_SIZEOF_TCPHEADER) {//part header received
            need_to_read -= trans->received_header_bytes;
        } else {
            DUER_LOGE("wrong received_header_bytes(%d) value[0-%d]!!",
                    trans->received_header_bytes, DUER_SIZEOF_TCPHEADER);
            rs = DUER_ERR_TRANS_INTERNAL_ERROR;
            return rs;
        }
    } else if (size >= DUER_MIN_BODY_SIZE && size <= DUER_MAX_BODY_SIZE) {
        DUER_AES_INFO_PRINT("current trans->received_body_bytes: %d [mostly 0]",
                             trans->received_body_bytes);
        if (trans->received_body_bytes < size) { // trans->received_body_bytes should less than size
            need_to_read -= trans->received_body_bytes;
        } else {
            DUER_LOGE("wrong received_body_bytes(%d) value[%d-%d]!!",
                    trans->received_body_bytes, DUER_MIN_BODY_SIZE, DUER_MAX_BODY_SIZE);
            rs = DUER_ERR_TRANS_INTERNAL_ERROR;
            return rs;
        }
    } else {
        DUER_LOGE("wrong size try to receive, header[%d], body[%d-%d]!!",
                DUER_SIZEOF_TCPHEADER, DUER_MIN_BODY_SIZE, DUER_MAX_BODY_SIZE);
        rs = DUER_ERR_TRANS_INTERNAL_ERROR;
        return rs;
    }
    rs = duer_trans_wrapper_recv(trans, data, need_to_read, addr);
    if (rs <= 0) {
        if (rs != DUER_ERR_TRANS_WOULD_BLOCK) {
            DUER_LOGW("recv error! rs:%d, size:%d", rs, size);
        }
        goto exit;
    }

    DUER_AES_INFO_PRINT("received size:%d [mostly %d]", rs, size);
    if (size == DUER_SIZEOF_TCPHEADER) { //header
        if (rs == need_to_read) { // all header info got
            DUER_MEMCPY(trans->received_header + trans->received_header_bytes,
                        data, need_to_read);//cached the received data
            trans->received_header_bytes = size;
            DUER_MEMCPY(data, trans->received_header, size);
            rs = size;

            duer_trans_aes_cbc_encrypted_header_t *p_header
                    = (duer_trans_aes_cbc_encrypted_header_t*)data;
            unsigned int magic_num = duer_htonl(p_header->magic_num);
            unsigned int length = duer_htonl(p_header->msg_size);
#ifdef DEV_DEBUG_AES
            unsigned int *p_magic_num = (unsigned int*)data;
            DUER_AES_PRINT("magic_number: %x\n", duer_htonl(*p_magic_num));
            unsigned int *p_length = (unsigned int*)(data + 4);
            DUER_AES_PRINT("outer_length:%u\n", duer_htonl(*p_length));
#endif
            if (magic_num == DUER_BDCAEC_MN) {//AES-encrypted method used, update the magic number
                p_header->magic_num = duer_htonl(DUER_COAP_TCP_HDR);
                DUER_MEMCPY(trans->received_header, data, size);
            }
        } else {//part header info received
            DUER_LOGW("got only part of header info, rarely case happened!rs:%d < need_to_read:%d",
                      rs, need_to_read);
            //cached the received data
            DUER_MEMCPY(trans->received_header + trans->received_header_bytes,
                        data, rs);
            trans->received_header_bytes += rs;
            rs = DUER_ERR_TRANS_WOULD_BLOCK; // notify the caller to try again
            goto exit;
        }
    } else { //body
        if (rs != need_to_read) {
            DUER_LOGE("receive part body info, rarely case happened! rs:%d, need_to_read: %d",
                       rs, need_to_read);
            DUER_MEMCPY(trans->received_body + trans->received_body_bytes,
                        data, rs);
            trans->received_body_bytes += rs;
            rs = DUER_ERR_TRANS_WOULD_BLOCK; // notify the caller to try again
            goto exit;
        } else {
            duer_aes_context ptr = trans ? (duer_aes_context)trans->secure : NULL;
            if (ptr == NULL) {
                DUER_LOGE("aes_context is NULL");
                rs = DUER_ERR_TRANS_INTERNAL_ERROR;
                goto exit;
            }

            //all body info got
            if (need_to_read != size) { // partly received before rarely happen
                DUER_LOGW("rarely happen case!!!");
                DUER_MEMCPY(trans->received_body + trans->received_body_bytes,
                        data, need_to_read);
                DUER_MEMCPY(data, trans->received_body, size);
                rs = size;
            }
            //clear cached info, ? need to reset the buffer content after handle
            trans->received_header_bytes = 0;
            trans->received_body_bytes = 0;

            // begin to parse the received body
            unsigned char *p_raw_data_len = (unsigned char*)data;
            unsigned int raw_data_len = *p_raw_data_len;
            unsigned int encrypted_msg_len = size -1 - raw_data_len;
            unsigned char *output = (unsigned char *)trans->received_body;
#ifdef DEV_DEBUG_AES
            DUER_AES_PRINT("raw_data_len: %x \n", raw_data_len);
            DUER_AES_PRINT("encrypted_msg_len: %x \n", encrypted_msg_len);
            //unsigned char *raw_data = (unsigned char*)(data + 1);
            //DUER_AES_PRINT("raw_data:%s \n", raw_data);
#endif
            rs = duer_aes_cbc_decrypt(ptr, encrypted_msg_len,
                        (const unsigned char*)data + 1 + raw_data_len,
                        output);
            if (rs < 0) {
                DUER_LOGE("decrypted failed! rs:%d", rs);
                rs = DUER_ERR_TRANS_INTERNAL_ERROR;
                output = NULL;
                goto exit;
            }
            unsigned int *p_length = (unsigned int*)(output + 4);
            unsigned int length = duer_htonl(*p_length);
#ifdef DEV_DEBUG_AES
            unsigned int *p_magic_num = (unsigned int*)output;
            unsigned int magic_num = duer_htonl(*p_magic_num);
            DUER_AES_PRINT("coap magic_number: %x\n", magic_num);
            DUER_AES_PRINT("coap_length:%u\n", length);
#endif
            DUER_MEMSET(data, 0, size);
            DUER_MEMCPY(data, output + 8, length);
            if (rs == 0) {
                rs = length;
            }
            output = NULL;
        }
    }

exit:

    return rs;
}

duer_status_t duer_trans_aes_cbc_encrypted_recv_timeout(duer_trans_ptr trans,
                                        void* data,
                                        duer_size_t size,
                                        duer_u32_t timeout,
                                        duer_addr_t* addr)
{
    return duer_trans_aes_cbc_encrypted_recv(trans, data, size, addr);
}

duer_status_t duer_trans_aes_cbc_encrypted_close(duer_trans_ptr trans)
{
    duer_aes_context ptr = trans ? (duer_aes_context)trans->secure : NULL;
    if (ptr) {
        duer_aes_context_destroy(ptr);
        trans->secure = NULL;
    }
    return duer_trans_wrapper_close(trans);
}

#endif // NET_TRANS_ENCRYPTED_BY_AES_CBC
