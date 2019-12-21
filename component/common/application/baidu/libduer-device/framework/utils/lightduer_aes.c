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
// Description: aes encryption implementation

#include "lightduer_aes.h"
#include "mbedtls/aes.h"
#include "lightduer_lib.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_types.h"

//#define LIGHTDUER_AES_DEBUG
#ifdef LIGHTDUER_AES_DEBUG
#define DUER_AES_PRINT printf
#else
#define DUER_AES_PRINT DUER_LOGD
#endif

typedef struct _duer_aes_context {
    mbedtls_aes_context mbedctx; // used by the mbedtls aes algorithm
    unsigned char key[32]; // the key use in the algorithm, max size is 32bytes
    unsigned int keybits; // the length of the key, only 128, 192, 256 supported
    unsigned char iv[16]; // the initial vector, it's 16bytes
} duer_aes_context_t;

duer_aes_context duer_aes_context_init(void)
{
    duer_aes_context_t* aes_context = (duer_aes_context_t*)DUER_MALLOC(sizeof(duer_aes_context_t));
    if (aes_context == NULL) {
        DUER_LOGE("alloc duer_aes_context_t failed!");
        return NULL;
    }
    DUER_MEMSET(aes_context->key, 0, sizeof(aes_context->key));
    DUER_MEMSET(aes_context->iv, 0, sizeof(aes_context->iv));
    aes_context->keybits = 0;
    mbedtls_aes_init(&aes_context->mbedctx);
    return aes_context;
}

int duer_aes_setkey(duer_aes_context ctx,
                    const unsigned char *key,
                    unsigned int keybits)
{
    if (ctx == NULL) {
        DUER_LOGW("ctx is NULL!");
        return DUER_ERR_INVALID_PARAMETER;
    }
    if (keybits != 128 && keybits != 192 && keybits != 256) {
        DUER_LOGW("keybits should be one of (128, 192, 256)");
        return DUER_ERR_INVALID_PARAMETER;
    }
    duer_aes_context_t* aes_context = (duer_aes_context_t*)ctx;
    DUER_MEMCPY(aes_context->key, key, keybits / 8);
    aes_context->keybits = keybits;
    return DUER_OK;
}

int duer_aes_setiv(duer_aes_context ctx, unsigned char iv[16])
{
    if (ctx == NULL) {
        DUER_LOGW("ctx is NULL!");
        return DUER_ERR_INVALID_PARAMETER;
    }
    duer_aes_context_t* aes_context = (duer_aes_context_t*)ctx;
    DUER_MEMCPY(aes_context->iv, iv, sizeof(aes_context->iv));
    return DUER_OK;
}


int duer_aes_cbc_encrypt(duer_aes_context ctx,
                         size_t length,
                         const unsigned char* input,
                         unsigned char* output)
{
    if (ctx == NULL) {
        DUER_LOGW("ctx is NULL!");
        return DUER_ERR_INVALID_PARAMETER;
    }
    duer_aes_context_t* aes_context = (duer_aes_context_t*)ctx;

    unsigned char iv[16];
    DUER_MEMCPY(iv, aes_context->iv, sizeof(iv));

#ifdef LIGHTDUER_AES_DEBUG
    DUER_AES_PRINT("key:\n");
    for (int i = 0; i < (aes_context->keybits / 8); ++i) {
        DUER_AES_PRINT(" %x", (unsigned char)aes_context->key[i]);
    }
    DUER_AES_PRINT("\n");
    DUER_AES_PRINT("iv:\n");
    for (int i = 0; i < sizeof(iv); ++i) {
        DUER_AES_PRINT(" %x", (unsigned char)iv[i]);
    }
    DUER_AES_PRINT("\n");
    DUER_AES_PRINT("input data:\n");
    for (int i = 0; i < length; ++i) {
        DUER_AES_PRINT(" %x", (unsigned char)input[i]);
    }
    DUER_AES_PRINT("\n");
#endif
    int res = 0;
    do {
        //TODO if this needed every time??
        res = mbedtls_aes_setkey_enc(&aes_context->mbedctx, aes_context->key, aes_context->keybits);
        if (res) {
            break;
        }
        res = mbedtls_aes_crypt_cbc(&aes_context->mbedctx, MBEDTLS_AES_ENCRYPT,
                                    length, iv, input, output);
    } while(0);

    if (res != 0) {
        DUER_LOGW("encrypt fail!!res:%d", res);
        return DUER_ERR_FAILED;
    } else {
#ifdef LIGHTDUER_AES_DEBUG
        DUER_AES_PRINT("encrypt output data:\n");
        for (int i = 0; i < length; ++i) {
            DUER_AES_PRINT(" %x", (unsigned char)output[i]);
        }
        DUER_AES_PRINT("\n");
#endif
        return DUER_OK;
    }
}

int duer_aes_cbc_decrypt(duer_aes_context ctx,
                         size_t length,
                         const unsigned char* input,
                         unsigned char* output)
{
    if (ctx == NULL) {
        DUER_LOGW("ctx is NULL!");
        return DUER_ERR_INVALID_PARAMETER;
    }
    duer_aes_context_t* aes_context = (duer_aes_context_t*)ctx;

    unsigned char iv[16];
    DUER_MEMCPY(iv, aes_context->iv, sizeof(iv));
#ifdef LIGHTDUER_AES_DEBUG
    DUER_AES_PRINT("key:\n");
    for (int i = 0; i < (aes_context->keybits / 8); ++i) {
        DUER_AES_PRINT(" %x", (unsigned char)aes_context->key[i]);
    }
    DUER_AES_PRINT("\n");
    DUER_AES_PRINT("iv:\n");
    for (int i = 0; i < sizeof(iv); ++i) {
        DUER_AES_PRINT(" %x", (unsigned char)iv[i]);
    }
    DUER_AES_PRINT("\n");
    DUER_AES_PRINT("input data:\n");
    for (int i = 0; i < length; ++i) {
        DUER_AES_PRINT(" %x", (unsigned char)input[i]);
    }
    DUER_AES_PRINT("\n");
#endif
    int res = 0;
    do {
        //TODO if this needed every time??
        res = mbedtls_aes_setkey_dec(&aes_context->mbedctx, aes_context->key, aes_context->keybits);
        if (res) {
            break;
        }
        res = mbedtls_aes_crypt_cbc(&aes_context->mbedctx, MBEDTLS_AES_DECRYPT,
                                    length, iv, input, output);
    } while(0);

    if (res != 0) {
        DUER_LOGW("decrypt faild!! res:%d", res);
        return DUER_ERR_FAILED;
    } else {
#ifdef LIGHTDUER_AES_DEBUG
        DUER_AES_PRINT("decrypt output data:\n");
        for (int i = 0; i < length; ++i) {
            DUER_AES_PRINT(" %x", (unsigned char)output[i]);
        }
        DUER_AES_PRINT("\n");
#endif
        return DUER_OK;
    }
}

int duer_aes_context_destroy(duer_aes_context ctx)
{
    if (ctx == NULL) {
        DUER_LOGW("ctx is NULL!");
        return DUER_ERR_INVALID_PARAMETER;
    }
    duer_aes_context_t* aes_context = (duer_aes_context_t*)ctx;

    mbedtls_aes_free(&aes_context->mbedctx);
    DUER_FREE(aes_context);
    return DUER_OK;
}
