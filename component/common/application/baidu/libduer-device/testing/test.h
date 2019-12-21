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
// Author: Zhang Leliang (zhangleliang@baidu.com)
//
// Description: unit test definitions

#ifndef BAIDU_DUER_LIBDUER_DEVICE_TESTING_TEST_H
#define BAIDU_DUER_LIBDUER_DEVICE_TESTING_TEST_H

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#define _test_init __attribute__((unused, section(".testing"))) __attribute__((aligned(4)))

#define CMOCKA_UNIT_TEST(f) struct CMUnitTest _test_##f _test_init = cmocka_unit_test(f)

#define CMOCKA_UNIT_TEST_SETUP(f, setup) \
        struct CMUnitTest _test_##f _test_init = \
        cmocka_unit_test_setup(f, setup)

#define CMOCKA_UNIT_TEST_TEARDOWN(f, teardown) \
        struct CMUnitTest _test_##f _test_init = \
        cmocka_unit_test_teardown(f, teardown)

#define CMOCKA_UNIT_TEST_SETUP_TEARDOWN(f, setup, teardown) \
        struct CMUnitTest _test_##f _test_init = \
        cmocka_unit_test_setup_teardown(f, setup, teardown)

#define CMOCKA_UNIT_TEST_PRESTATE(f, state) \
        struct CMUnitTest _test_##f _test_init = \
        cmocka_unit_test_prestate(f, state)

#define CMOCKA_UNIT_TEST_PRESTATE_SETUP_TEARDOWN(f, setup, teardown, state) \
        struct CMUnitTest _test_##f _test_init = \
        cmocka_unit_test_prestate_setup_teardown(f, setup, teardown, state)

#endif // BAIDU_DUER_LIBDUER_DEVICE_TESTING_TEST_H
