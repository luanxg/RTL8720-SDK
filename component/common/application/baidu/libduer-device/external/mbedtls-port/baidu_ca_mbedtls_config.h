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
// Description: The configuration for mbedtls.

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_IOT_BAIDU_CA_INCLUDE_BAIDU_CA_MBEDTLS_CONFIG_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_IOT_BAIDU_CA_INCLUDE_BAIDU_CA_MBEDTLS_CONFIG_H

/************************************************************/
/* System support */
//#define MBEDTLS_HAVE_ASM
//#define MBEDTLS_HAVE_TIME

// TODO: there is a bug in RDA's hardware encryption function,
//       use software encryption default before RDA fix it
//
#if defined(TARGET_UNO_91H)
#if defined(MBEDTLS_HARDWARE_ENCRYPTION)
#define MBEDTLS_AES_ENCRYPT_CBC_ALT
#define MBEDTLS_AES_SETKEY_ENC_ALT
#define MBEDTLS_AES_SETKEY_DEC_ALT
#define MBEDTLS_AES_ENCRYPT_ALT
#define MBEDTLS_AES_DECRYPT_ALT
#define MBEDTLS_RSA_MONTMUL_ALT
#define MBEDTLS_DRBG_ALT
#else /* MBEDTLS_HARDWARE_ENCRYPTION */
/*
 * When using software encryption, 8750 bytes will be added to bss segment
 * and 4952 bytes will be added to text segment.
 * We define this macro to move the 8750 bytes from bss segment to text segment,
 * then 13192 bytes will be added to text segment.
 */
#define MBEDTLS_AES_ROM_TABLES
#endif /* MBEDTLS_HARDWARE_ENCRYPTION */
#endif /* TARGET_UNO_91H */


/* mbed TLS feature support */
#define MBEDTLS_CIPHER_MODE_CBC
#define MBEDTLS_PKCS1_V15
#define MBEDTLS_KEY_EXCHANGE_RSA_ENABLED
//#define MBEDTLS_SSL_PROTO_TLS1_1

/* mbed TLS modules */
#define MBEDTLS_AES_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CIPHER_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_DES_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_MD_C
#define MBEDTLS_MD5_C
//#define MBEDTLS_NET_C    ///////
#define MBEDTLS_OID_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_RSA_C
#define MBEDTLS_SHA1_C
#define MBEDTLS_SHA256_C
//Edit by Realtek
#ifndef CONFIG_PLATFORM_8195A
#define MBEDTLS_SHA512_C
#endif
#define MBEDTLS_SSL_CLI_C
#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_X509_USE_C
//#define MBEDTLS_SSL_SERVER_NAME_INDICATION     //////////////

/* For test certificates */
#define MBEDTLS_BASE64_C
#define MBEDTLS_CERTS_C
#define MBEDTLS_PEM_PARSE_C

/* For testing with compat.sh */
//#define MBEDTLS_FS_IO

/************************************************************/

//#define MBEDTLS_TIMING_C
#define MBEDTLS_CTR_DRBG_C

#if 1

#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY

#ifndef DUER_HM_MODULE_NAME
#define DUER_HM_MODULE_NAME DUER_HM_MBEDTLS
#endif

#include "lightduer_log.h"
#include "lightduer_memory.h"
#define MBEDTLS_PLATFORM_FREE_MACRO         DUER_FREE
#define MBEDTLS_PLATFORM_CALLOC_MACRO       DUER_CALLOC

#endif

//#define MBEDTLS_THREADING_C
//#define MBEDTLS_THREADING_ALT

#if defined(DUER_MBEDTLS_DEBUG) && (DUER_MBEDTLS_DEBUG > 0)
#define MBEDTLS_DEBUG_C
#endif

/************************************************************/

//#define MBEDTLS_SSL_PROTO_DTLS

#define MBEDTLS_CCM_C

//#if defined(MBEDTLS_SSL_PROTO_DTLS)
#define MBEDTLS_SSL_PROTO_TLS1_2
//#endif
//Edit by Realtek

// the biggest received coap packet is 2K
#ifndef MBEDTLS_SSL_MAX_CONTENT_LEN
#define MBEDTLS_SSL_MAX_CONTENT_LEN         (3*1024)
#endif

#define MBEDTLS_NO_PLATFORM_ENTROPY

/************************************************************/

#include "mbedtls/check_config.h"

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_IOT_BAIDU_CA_INCLUDE_BAIDU_CA_MBEDTLS_CONFIG_H
