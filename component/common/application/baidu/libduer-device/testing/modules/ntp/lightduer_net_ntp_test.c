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

#include "lightduer_types.h"
#include "lightduer_net_ntp.h"
#include "lightduer_net_transport.h"
#include "lightduer_network_defs.h"

//-----------
//duer_u32_t duer_ntohl(duer_u32_t value) {
//}
//
//duer_u32_t duer_htonl(duer_u32_t value) {
//}

static int CURRENT_TIME = 0;
duer_u32_t duer_timestamp() {
    return CURRENT_TIME++;
}

void duer_sleep(duer_u32_t ms) {
    check_expected(ms);
}
//-----------
duer_status_t duer_trans_send(duer_trans_handler hdlr,
                              const void* data,
                              duer_size_t size,
                              const duer_addr_t* addr) {
    //check_expected(hdlr);
    //check_expected(data);
    //check_expected(size);
    //check_expected(addr);
    return mock_type(duer_status_t);
}

duer_trans_handler duer_trans_acquire(duer_context context) {
    check_expected(context);
    return mock_ptr_type(duer_trans_handler);
}

duer_status_t duer_trans_connect(duer_trans_handler hdlr,
                                 const duer_addr_t* addr) {
    check_expected(hdlr);
    check_expected(addr->host);
    check_expected(addr->host_size);
    check_expected(addr->port);
    check_expected(addr->type);
    return mock_type(duer_status_t);
}

duer_status_t duer_trans_recv(duer_trans_handler hdlr,
                              void* data,
                              duer_size_t size,
                              duer_addr_t* addr) {
    check_expected(hdlr);
    check_expected(data);
    check_expected(size);
    check_expected(addr->host);
    check_expected(addr->port);
    return mock_type(duer_status_t);
}
//-----------
static const duer_trans_handler HANDLER = (duer_trans_handler)0x1001;
#define NTP_SERVER "s2c.time.edu.cn"    // 202.112.10.36
#define NTP_PORT (123)

void duer_ntp_client_test_normal1(void** state) {
    char host[] = "host";
    int timeout = 3000;
    DuerTime duer_time;
    duer_time.sec = 10;
    duer_time.usec = 1000;

    expect_value(duer_trans_acquire, context, NULL);
    will_return(duer_trans_acquire, HANDLER);

    expect_value(duer_trans_connect, hdlr, HANDLER);
    expect_string(duer_trans_connect, addr->host, host);
    expect_value(duer_trans_connect, addr->host_size, strlen(host));
    expect_value(duer_trans_connect, addr->port, NTP_PORT);
    expect_value(duer_trans_connect, addr->type, DUER_PROTO_UDP);
    will_return(duer_trans_connect, DUER_OK);

    //expect_value(duer_trans_send, , );
    will_return(duer_trans_send, DUER_OK);

    expect_value_count(duer_trans_recv, hdlr, HANDLER, 2);
    expect_not_value_count(duer_trans_recv, data, NULL, 2);
    expect_value_count(duer_trans_recv, size, 4*325, 2);
    expect_string_count(duer_trans_recv, addr->host, host, 2);
    expect_value_count(duer_trans_recv, addr->port, NTP_PORT, 2);
    will_return(duer_trans_recv, DUER_ERR_FAILED);
    will_return(duer_trans_recv, DUER_OK);

    expect_value(duer_sleep, ms, 500);

    int res = duer_ntp_client(host, timeout, &duer_time);
    assert_int_equal(res, DUER_OK);
}

void duer_ntp_client_test_normal2(void** state) {
    DuerTime duer_time;
    duer_time.sec = 10;
    duer_time.usec = 1000;

    expect_value(duer_trans_acquire, context, NULL);
    will_return(duer_trans_acquire, HANDLER);

    expect_value(duer_trans_connect, hdlr, HANDLER);
    expect_string(duer_trans_connect, addr->host, NTP_SERVER);
    expect_value(duer_trans_connect, addr->host_size, strlen(NTP_SERVER));
    expect_value(duer_trans_connect, addr->port, NTP_PORT);
    expect_value(duer_trans_connect, addr->type, DUER_PROTO_UDP);
    will_return(duer_trans_connect, DUER_OK);

    //expect_value(duer_trans_send, , );
    will_return(duer_trans_send, DUER_OK);

    expect_value_count(duer_trans_recv, hdlr, HANDLER, 2);
    expect_not_value_count(duer_trans_recv, data, NULL, 2);
    expect_value_count(duer_trans_recv, size, 4*325, 2);
    expect_string_count(duer_trans_recv, addr->host, NTP_SERVER, 2);
    expect_value_count(duer_trans_recv, addr->port, NTP_PORT, 2);
    will_return(duer_trans_recv, DUER_ERR_FAILED);
    will_return(duer_trans_recv, DUER_OK);

    expect_value(duer_sleep, ms, 500);

    int res = duer_ntp_client(NULL, 0, &duer_time);
    assert_int_equal(res, DUER_OK);
}

CMOCKA_UNIT_TEST(duer_ntp_client_test_normal1);
CMOCKA_UNIT_TEST(duer_ntp_client_test_normal2);
