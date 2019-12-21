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
 * File: baidu_ca_util_network.c
 * Auth: Su Hao(suhao@baidu.com)
 * Date: 2016.10.20
 * Desc: Network util tools.
 */

#include "lightduer_net_util.h"

#define SWAP_S(A)  ((((duer_u16_t)(A) & 0xff00) >> 8) | \
                    (((duer_u16_t)(A) & 0x00ff) << 8))

#define SWAP_L(A)  ((((duer_u32_t)(A) & 0xff000000) >> 24) | \
                    (((duer_u32_t)(A) & 0x00ff0000) >> 8) | \
                    (((duer_u32_t)(A) & 0x0000ff00) << 8) | \
                    (((duer_u32_t)(A) & 0x000000ff) << 24))

DUER_INT_IMPL duer_u8_t duer_is_little_endian(void)
{
    duer_u32_t x = 1;
    char *p = (char *) &x;
    return *p;
}

DUER_INT_IMPL duer_u16_t duer_htons(duer_u16_t value)
{
    return duer_is_little_endian() ? SWAP_S(value) : value;
}

DUER_INT_IMPL duer_u32_t duer_htonl(duer_u32_t value)
{
    return duer_is_little_endian() ? SWAP_L(value) : value;
}

DUER_INT_IMPL duer_u16_t duer_ntohs(duer_u16_t value)
{
    return duer_is_little_endian() ? SWAP_S(value) : value;
}

DUER_INT_IMPL duer_u32_t duer_ntohl(duer_u32_t value)
{
    return duer_is_little_endian() ? SWAP_L(value) : value;
}

