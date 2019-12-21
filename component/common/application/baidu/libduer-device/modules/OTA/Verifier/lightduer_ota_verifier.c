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
 * File: lightduer_ota_verification.c
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: OTA Verification
 */

#include "lightduer_ota_verifier.h"
#include "lightduer_ota_unpacker.h"
#include <stdint.h>
#include <stdbool.h>
#include "zlib.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"

#define SHA1_LEN 20

#define RSA_N   "b7699a83da6100367cd9f43ac124" \
                "dd0f4fce5a4d39c02a2f844fb7867f" \
                "869cb535373d0022039bf5cb58e869" \
                "b041998baaf10d08705d043227eb4b" \
                "20204e63c0d409f3225c6eb013abc6" \
                "fef7d89435258eaa872c52020c0f07" \
                "fd3ce59fb6a042f6de4b2c26e50732" \
                "c8691cb744df77856b8b2a59066cca" \
                "ccd4e289d0440fe34b"

#define RSA_E   "10001"

duer_ota_verifier_t *duer_ota_verification_create_verifier(void)
{
    int ret = 0;
    duer_ota_verifier_t *verifier = NULL;

    do {
        verifier = DUER_MALLOC(sizeof(*verifier));
        if (verifier == NULL) {
            DUER_LOGE("OTA Verifier: Create verifier failed");

            return NULL;
        }

        DUER_MEMSET(verifier, 0, sizeof(*verifier));

        mbedtls_sha1_init(&verifier->sha1);
        mbedtls_sha1_starts(&verifier->sha1);

        mbedtls_rsa_init(&verifier->rsa, MBEDTLS_RSA_PKCS_V15, 0);
        verifier->rsa.len = KEY_LEN;

        ret = mbedtls_mpi_read_string(&verifier->rsa.N , 16, RSA_N);
        if (ret != 0) {
            DUER_LOGE("OTA Verifier: Read RSA N failed ret:%d", ret);
            break;
        }

        ret = mbedtls_mpi_read_string(&verifier->rsa.E , 16, RSA_E);
        if (ret != 0) {
            DUER_LOGE("OTA Verifier: Read RSA E failed ret:%d", ret);
            break;
        }

        ret = mbedtls_rsa_check_pubkey(&verifier->rsa);
        if(ret != 0) {
            DUER_LOGE("OTA Verifier: Check key-pair failed");

            break;
        }

        return verifier;
    } while (0);

    mbedtls_sha1_free(&verifier->sha1);

    mbedtls_rsa_free(&verifier->rsa);

    if (verifier != NULL) {
        DUER_FREE(verifier);
    }

    return NULL;
}

void duer_ota_verification_destroy_verifier(duer_ota_verifier_t *verifier)
{
    if (verifier == NULL) {
        DUER_LOGE("OTA Verifier: Argument Error");

        return;
    }

    mbedtls_sha1_free(&verifier->sha1);

    mbedtls_rsa_free(&verifier->rsa);

    DUER_FREE(verifier);
}

void duer_ota_verification_update(
        duer_ota_verifier_t *verifier,
        uint8_t const *data,
        size_t size)
{
    mbedtls_sha1_update(&verifier->sha1, data, size);
}

int duer_ota_verification_verify(duer_ota_verifier_t *verifier,
        duer_ota_package_header_t const *header)
{
    int i = 0;
    int ret = DUER_OK;
    unsigned char sha1[SHA1_LEN];
    unsigned char package_signature[KEY_LEN];

    if (verifier == NULL || header == NULL) {
        DUER_LOGE("OTA Verifier: Argument Error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    DUER_MEMMOVE(package_signature, header->package_signature, KEY_LEN);

    DUER_LOGD("OTA Verifier: Package signature:");
    for (i = 0; i < KEY_LEN; i++) {
        DUER_LOGD("%.2x", package_signature[i]);
    }

    mbedtls_sha1_finish(&verifier->sha1, sha1);

    DUER_LOGD("OTA Verifier: MD5:");
    for (i = 0; i < SHA1_LEN; i++) {
        DUER_LOGD("%.2x", sha1[i]);
    }

    ret = mbedtls_rsa_pkcs1_verify(
            &verifier->rsa,
            NULL,
            NULL,
            MBEDTLS_RSA_PUBLIC,
            MBEDTLS_MD_SHA1,
            0,
            sha1,
            package_signature);
    if (ret != 0) {
        DUER_LOGE(" OTA Verifier: verify failed ret:%d", ret);

        ret = DUER_ERR_FAILED;
    }

    return ret;
}
