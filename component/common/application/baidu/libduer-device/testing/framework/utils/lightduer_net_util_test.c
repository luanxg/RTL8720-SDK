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

#include "test.h"

#include "lightduer_net_util.h"

void duer_is_little_endian_test(void** state) {
    int x = 1;
    char* p = (char*)&x;

    duer_u8_t res = duer_is_little_endian();
    assert_int_equal(res, *p);
}

void duer_htons_test(void** state) {
    duer_u8_t is_little = duer_is_little_endian();
    duer_u16_t test_data = 0xABCD;
    duer_u16_t result = duer_htons(test_data);
    if (is_little) {
        assert_int_equal(result , 0xCDAB);
    } else {
        assert_int_equal(result , test_data);
    }
}

void duer_htonl_test(void** state) {
    duer_u8_t is_little = duer_is_little_endian();
    duer_u32_t test_data = 0xA1B2C3D4;
    duer_u32_t result = duer_htonl(test_data);
    if (is_little) {
        assert_int_equal(result , 0xD4C3B2A1);
    } else {
        assert_int_equal(result , test_data);
    }
}

void duer_ntohs_test(void** state) {
    duer_u8_t is_little = duer_is_little_endian();
    duer_u16_t test_data = 0xABCD;
    duer_u16_t result = duer_ntohs(test_data);
    if (is_little) {
        assert_int_equal(result , 0xCDAB);
    } else {
        assert_int_equal(result , test_data);
    }
}

void duer_ntohl_test(void** state) {
    duer_u8_t is_little = duer_is_little_endian();
    duer_u32_t test_data = 0xA1B2C3D4;
    duer_u32_t result = duer_ntohl(test_data);
    if (is_little) {
        assert_int_equal(result , 0xD4C3B2A1);
    } else {
        assert_int_equal(result , test_data);
    }
}

CMOCKA_UNIT_TEST(duer_is_little_endian_test);
CMOCKA_UNIT_TEST(duer_htons_test);
CMOCKA_UNIT_TEST(duer_htonl_test);
CMOCKA_UNIT_TEST(duer_ntohs_test);
CMOCKA_UNIT_TEST(duer_ntohl_test);
