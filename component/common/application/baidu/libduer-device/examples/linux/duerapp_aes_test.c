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
/**
 * File: duerapp_aes_test.c
 * Auth: Zhang Leliang(zhanglelaing@baidu.com)
 * Desc: Duer Application Main.
 */

#include "lightduer.h"
#include "lightduer_memory.h"
#include "lightduer_log.h"
#include "lightduer_lib.h"
#include "lightduer_aes.h"

char input[] = "aabbccddeeffgghhaabbccddeeffg";
// unsigned char input[] = {0xbe, 0xef, 0xde, 0xad, 0x0,  0x0,  0x0,  0x48, 0x40,  0x2,  0x1,  0x8f, 0xbd, 0x2, 0x72, 0x65,
//                          0x67, 0x5f, 0x66, 0x6f, 0x72, 0x5f, 0x63, 0x6f, 0x6e, 0x74, 0x72, 0x6f, 0x6c, 0xff, 0x7b, 0x22,
//                          0x64, 0x65, 0x76, 0x69, 0x63, 0x65, 0x55, 0x55, 0x49, 0x44, 0x22, 0x3a, 0x22, 0x30, 0x30, 0x64,
//                          0x66, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x33, 0x22, 0x2c, 0x22, 0x74, 0x6f,
//                          0x6b, 0x65, 0x6e, 0x22, 0x3a, 0x22, 0x66, 0x53, 0x71, 0x45, 0x34, 0x6a, 0x37, 0x67, 0x22, 0x7d};

char key_str[] = "8cd34facea95d5491b2cb5fbacacb0f0"; // 32bytes
unsigned char key[16];
unsigned char *input_data;
unsigned char *encrypted_data;
unsigned char *decrypted_data;
// char encrypted_data[80];
// char decrypted_data[80];
unsigned char iv[16];

duer_aes_context s_ctx;

void convert_bindtoken_to_key(char bind_token[32], unsigned char key[16]) {

    for (int i = 0; i < 16; ++i) {
        char str[3] = {bind_token[2*i], bind_token[2*i + 1], '\0'};
        int value = 0;
        sscanf(str, "%x", &value);
        key[i] = (unsigned char)value;
    }
}
void mbedtls_cbc_init() {
    DUER_MEMSET(iv, 0, sizeof(iv));//TODO how to get the IV
    //DUER_MEMSET(encrypted_data, 0, sizeof(encrypted_data));
    //DUER_MEMSET(decrypted_data, 0, sizeof(decrypted_data));
    //DUER_MEMCPY(key, key_str, sizeof(key));//convert the key to 128bit
    convert_bindtoken_to_key(key_str, key);
    s_ctx = duer_aes_context_init();
    duer_aes_setkey(s_ctx, key, sizeof(key) * 8);
    duer_aes_setiv(s_ctx, iv);
    printf("before decrypt got info: %s \n", input);
}

void mbedtls_cbc_encrypte() {
    size_t input_len = sizeof(input);
    size_t padding_len = input_len % 16;
    if (padding_len != 0) {
        padding_len = 16 - padding_len;
        input_data = DUER_MALLOC(input_len + padding_len);
        encrypted_data = DUER_MALLOC(input_len + padding_len);
        if (input_data == NULL || encrypted_data == NULL) {
            DUER_LOGI("malloc failed!");
            if (input_data) {
                DUER_FREE(input_data);
                input_data = NULL;
            }
            if (encrypted_data) {
                DUER_FREE(encrypted_data);
                encrypted_data = NULL;
            }
            return;
        }
        DUER_MEMSET(input_data, 0, input_len + padding_len);
        DUER_MEMCPY(input_data, input, input_len);
    } else {
        input_data = input;
    }
    duer_aes_cbc_encrypt(s_ctx, input_len + padding_len, input_data, encrypted_data);
    if (padding_len != 0) {
        DUER_FREE(input_data);
        input_data = NULL;
    }
}

void mbedtls_cbc_decrypte() {
    size_t input_len = sizeof(input);
    size_t padding_len = input_len % 16;
    if (padding_len != 0) {
        padding_len = 16 - padding_len;
        decrypted_data = DUER_MALLOC(input_len + padding_len);
        if (decrypted_data == NULL) {
            DUER_LOGI("malloc failed!");
            return;
        }
    }
    duer_aes_cbc_decrypt(s_ctx, input_len + padding_len, encrypted_data, decrypted_data);
    printf("after decrypt got info : %s \n", decrypted_data);
    if (decrypted_data != NULL) {
        DUER_FREE(decrypted_data);
        decrypted_data = NULL;
    }
}

void mbedtls_cbc_destroy() {
    duer_aes_context_destroy(s_ctx);
    if (input_data) {
        DUER_FREE(input_data);
        input_data = NULL;
    }
    if (encrypted_data) {
        DUER_FREE(encrypted_data);
        encrypted_data = NULL;
    }
    if (decrypted_data) {
        DUER_FREE(decrypted_data);
        decrypted_data = NULL;
    }
}

int main(int argc, char* argv[])
{
    duer_initialize();
    DUER_LOGI("after initialize");

    mbedtls_cbc_init();
    mbedtls_cbc_encrypte();
    mbedtls_cbc_decrypte();
    mbedtls_cbc_destroy();
    return 0;
}
