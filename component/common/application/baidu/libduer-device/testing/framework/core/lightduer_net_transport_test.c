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

#include "lightduer_net_transport.h"

#define SOC_CONTEXT 0x55AA
#define CONTEXT 0xAADD
#define ADDR 0x11BB
#define DATA 0xBBCC
#define DATA_LEN 4
#define TIMEOUT 10

static duer_socket_t bcasoc_create(duer_context context) {
    check_expected(context);
    return mock_ptr_type(duer_socket_t);
}

static duer_status_t bcasoc_connect(duer_socket_t ctx, const duer_addr_t *addr) {
    check_expected(ctx);
    check_expected(addr);
    return mock_type(duer_status_t);
}

static duer_status_t bcasoc_send(duer_socket_t ctx,
                                 const void *data, duer_size_t size,
                                 const duer_addr_t *addr) {
    check_expected(ctx);
    check_expected(data);
    check_expected(size);
    check_expected(addr);
    return mock_type(duer_status_t);
}

static duer_status_t bcasoc_recv(duer_socket_t ctx,
                                 void *data, duer_size_t size, duer_addr_t *addr) {
    check_expected(ctx);
    check_expected(data);
    check_expected(size);
    check_expected(addr);
    return mock_type(duer_status_t);
}

static duer_status_t bcasoc_recv_timeout(duer_socket_t ctx,
                                         void *data, duer_size_t size,
                                         duer_u32_t timeout, duer_addr_t *addr) {
    check_expected(ctx);
    check_expected(data);
    check_expected(size);
    check_expected(timeout);
    check_expected(addr);
    return mock_type(duer_status_t);
}

static duer_status_t bcasoc_close(duer_socket_t ctx) {
    check_expected(ctx);
    return mock_type(duer_status_t);
}

static duer_status_t bcasoc_destroy(duer_socket_t ctx) {
    check_expected(ctx);
    return mock_type(duer_status_t);
}

static int duer_trans_create(void** state) {
    duer_trans_ptr trans = test_malloc(sizeof(duer_trans_t));

    assert_ptr_not_equal(trans, NULL);
    memset(trans, 0, sizeof(duer_trans_t));
    trans->soc_context = (duer_context)SOC_CONTEXT;
    trans->ctx = (duer_socket_t)CONTEXT;
    trans->read_timeout = DUER_READ_FOREVER;
    *state = trans;
    return 0;
}

static int duer_trans_destroy(void** state) {
    test_free(*state);
    return 0;
}

void baidu_ca_transport_init_test(void** state) {
    baidu_ca_transport_init(bcasoc_create,
                            bcasoc_connect,
                            bcasoc_send,
                            bcasoc_recv,
                            bcasoc_recv_timeout,
                            bcasoc_close,
                            bcasoc_destroy);
}

void duer_trans_wrapper_create_test(void** state) {
    duer_trans_ptr p_trans = (duer_trans_ptr)(*state);

    assert_ptr_equal(p_trans->ctx, CONTEXT); // set in duer_trans_create

    expect_value(bcasoc_create, context, SOC_CONTEXT);
    will_return(bcasoc_create, NULL);
    duer_status_t status = duer_trans_wrapper_create(p_trans);
    assert_int_equal(status, DUER_ERR_FAILED);

    status = duer_trans_wrapper_create(NULL);
    assert_int_equal(status, DUER_ERR_FAILED);

    baidu_ca_transport_init_test(state);// the value already set in baidu_ca_transport_init_test

    expect_value(bcasoc_create, context, SOC_CONTEXT);
    will_return(bcasoc_create, CONTEXT);
    assert_ptr_equal(p_trans->ctx, NULL); // clear by the above invoke, so it's NULL
    status = duer_trans_wrapper_create(p_trans);
    assert_int_equal(status, DUER_OK);
    assert_ptr_equal(p_trans->ctx, CONTEXT);
}

void duer_trans_wrapper_connect_test(void** state) {
    duer_trans_ptr p_trans = (duer_trans_ptr)(*state);

    assert_ptr_equal(p_trans->soc_context, SOC_CONTEXT); // set in duer_trans_create

    duer_status_t status = duer_trans_wrapper_connect(NULL, NULL);
    assert_int_equal(status, DUER_ERR_FAILED);

    expect_value(bcasoc_connect, ctx, CONTEXT);
    expect_value(bcasoc_connect, addr, ADDR);
    will_return(bcasoc_connect, DUER_OK);
    status = duer_trans_wrapper_connect(p_trans, (duer_addr_t*)ADDR);
    assert_int_equal(status, DUER_OK);
}

void duer_trans_wrapper_send_test(void** state) {
    duer_trans_ptr p_trans = (duer_trans_ptr)(*state);

    assert_ptr_equal(p_trans->soc_context, SOC_CONTEXT); // set in duer_trans_create

    duer_status_t status = duer_trans_wrapper_send(NULL, NULL, 0, NULL);
    assert_int_equal(status, DUER_ERR_FAILED);

    expect_value(bcasoc_send, ctx, CONTEXT);
    expect_value(bcasoc_send, data, DATA);
    expect_value(bcasoc_send, size, DATA_LEN);
    expect_value(bcasoc_send, addr, ADDR);
    will_return(bcasoc_send, DUER_OK);
    status = duer_trans_wrapper_send(p_trans, (void*)DATA,
                                        (duer_size_t)DATA_LEN,
                                        (duer_addr_t*)ADDR);
    assert_int_equal(status, DUER_OK);
}

void duer_trans_wrapper_recv_test(void** state) {
    duer_trans_ptr p_trans = (duer_trans_ptr)(*state);

    assert_ptr_equal(p_trans->soc_context, SOC_CONTEXT); // set in duer_trans_create

    duer_status_t status = duer_trans_wrapper_recv(NULL, NULL, 0, NULL);
    assert_int_equal(status, DUER_ERR_FAILED);

    baidu_ca_transport_init(bcasoc_create,
                            bcasoc_connect,
                            bcasoc_send,
                            bcasoc_recv,
                            NULL,
                            bcasoc_close,
                            bcasoc_destroy);

    expect_value(bcasoc_recv, ctx, CONTEXT);
    expect_value(bcasoc_recv, data, DATA);
    expect_value(bcasoc_recv, size, DATA_LEN);
    expect_value(bcasoc_recv, addr, ADDR);
    will_return(bcasoc_recv, DUER_OK);
    status = duer_trans_wrapper_recv(p_trans, (void*)DATA,
                                        (duer_size_t)DATA_LEN,
                                        (duer_addr_t*)ADDR);
    assert_int_equal(status, DUER_OK);

    baidu_ca_transport_init_test(state);

    expect_value(bcasoc_recv_timeout, ctx, CONTEXT);
    expect_value(bcasoc_recv_timeout, data, DATA);
    expect_value(bcasoc_recv_timeout, size, DATA_LEN);
    expect_value(bcasoc_recv_timeout, timeout, DUER_READ_FOREVER);
    expect_value(bcasoc_recv_timeout, addr, ADDR);
    will_return(bcasoc_recv_timeout, DUER_OK);
    status = duer_trans_wrapper_recv(p_trans, (void*)DATA,
                                        (duer_size_t)DATA_LEN,
                                        (duer_addr_t*)ADDR);
    assert_int_equal(status, DUER_OK);
}

void duer_trans_wrapper_recv_timeout_test(void** state){
    duer_trans_ptr p_trans = (duer_trans_ptr)(*state);

    assert_ptr_equal(p_trans->soc_context, SOC_CONTEXT); // set in duer_trans_create

    duer_status_t status = duer_trans_wrapper_recv_timeout(NULL, NULL, 0, 0, NULL);
    assert_int_equal(status, DUER_ERR_FAILED);

    expect_value(bcasoc_recv_timeout, ctx, CONTEXT);
    expect_value(bcasoc_recv_timeout, data, DATA);
    expect_value(bcasoc_recv_timeout, size, DATA_LEN);
    expect_value(bcasoc_recv_timeout, timeout, TIMEOUT);
    expect_value(bcasoc_recv_timeout, addr, ADDR);
    will_return(bcasoc_recv_timeout, DUER_OK);
    status = duer_trans_wrapper_recv_timeout(p_trans, (void*)DATA,
                                        (duer_size_t)DATA_LEN, TIMEOUT,
                                        (duer_addr_t*)ADDR);
    assert_int_equal(status, DUER_OK);
}

void duer_trans_wrapper_close_test(void** state) {
    duer_trans_ptr p_trans = (duer_trans_ptr)(*state);

    assert_ptr_equal(p_trans->soc_context, SOC_CONTEXT); // set in duer_trans_create

    duer_status_t status = duer_trans_wrapper_close(NULL);
    assert_int_equal(status, DUER_ERR_FAILED);

    expect_value(bcasoc_close, ctx, CONTEXT);
    will_return(bcasoc_close, DUER_OK);
    status = duer_trans_wrapper_close(p_trans);
    assert_int_equal(status, DUER_OK);
}

void duer_trans_wrapper_destroy_test(void** state) {
    duer_trans_ptr p_trans = (duer_trans_ptr)(*state);

    assert_ptr_equal(p_trans->soc_context, SOC_CONTEXT); // set in duer_trans_create

    duer_status_t status = duer_trans_wrapper_destroy(NULL);
    assert_int_equal(status, DUER_ERR_FAILED);

    expect_value(bcasoc_destroy, ctx, CONTEXT);
    will_return(bcasoc_destroy, DUER_OK);
    status = duer_trans_wrapper_destroy(p_trans);
    assert_int_equal(status, DUER_OK);
}

CMOCKA_UNIT_TEST(baidu_ca_transport_init_test);
CMOCKA_UNIT_TEST_SETUP_TEARDOWN(duer_trans_wrapper_create_test,
                                duer_trans_create, duer_trans_destroy);
CMOCKA_UNIT_TEST_SETUP_TEARDOWN(duer_trans_wrapper_connect_test,
                                duer_trans_create, duer_trans_destroy);
CMOCKA_UNIT_TEST_SETUP_TEARDOWN(duer_trans_wrapper_send_test,
                                duer_trans_create, duer_trans_destroy);
CMOCKA_UNIT_TEST_SETUP_TEARDOWN(duer_trans_wrapper_recv_test,
                                duer_trans_create, duer_trans_destroy);
CMOCKA_UNIT_TEST_SETUP_TEARDOWN(duer_trans_wrapper_recv_timeout_test,
                                duer_trans_create, duer_trans_destroy);
CMOCKA_UNIT_TEST_SETUP_TEARDOWN(duer_trans_wrapper_close_test,
                                duer_trans_create, duer_trans_destroy);
CMOCKA_UNIT_TEST_SETUP_TEARDOWN(duer_trans_wrapper_destroy_test,
                                duer_trans_create, duer_trans_destroy);
