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

#include "baidu_json.h"
#include "lightduer_coap_defs.h"
#include "lightduer_mutex.h"
#include "lightduer_speex.h"
#include "lightduer_types.h"
#include "lightduer_voice.h"

static const int SAMPLERATE = 8000;

int mbedtls_base64_encode( unsigned char *dst, size_t dlen, size_t *olen,
                           const unsigned char *src, size_t slen ) {
    check_expected(dst);
    check_expected(dlen);
    check_expected(olen);
    check_expected(src);
    check_expected(slen);
    return mock_type(int);
}

int duer_data_report(const baidu_json *data) {
    check_expected(data);
    return mock_type(int);
}

const duer_mutex_t MUTEXT = (duer_mutex_t)0x1008;

static duer_speex_handler SPEEX_HANDLER = (duer_speex_handler)0x4523;
static duer_encoded_func s_duer_encoded_func;

duer_speex_handler duer_speex_create(int rate) {
    check_expected(rate);
    return mock_ptr_type(duer_speex_handler);
}

void duer_speex_encode(duer_speex_handler handler,
                       const void *data,
                       size_t size,
                       duer_encoded_func func) {
    check_expected(handler);
    check_expected(data);
    check_expected(size);
    check_expected(func);
    s_duer_encoded_func = func;
}

void duer_speex_destroy(duer_speex_handler handler) {
    check_expected(handler);
}

static duer_notify_f s_duer_voice_response;

void duer_add_resources(const duer_res_t *res, size_t length) {
    check_expected(res);
    check_expected(length);
    s_duer_voice_response = res->res.f_res;
}

//================

void my_duer_voice_result_func(baidu_json *result) {
    check_expected(result);
}

void duer_voice_set_result_callback_test(void** state) {
    duer_voice_set_result_callback(my_duer_voice_result_func);
}

extern int duer_voice_initialize();

void duer_voice_initialize_test(void** state) {
    will_return(duer_mutex_create, NULL);
    int res = duer_voice_initialize();
    assert_int_equal(res, DUER_ERR_FAILED);
    //-------------
    will_return(duer_mutex_create, MUTEXT);
    expect_not_value(duer_add_resources, res, NULL);
    expect_value(duer_add_resources, length, 1);
    res = duer_voice_initialize();
    assert_int_equal(res, DUER_OK);
}

void duer_voice_response_test(void** state) {
    duer_context ctx = (duer_context)0x1005;

    duer_msg_t msg;
    char token[] = "token";
    char path[] = "/path";
    char query[] = "query";
    char payload[] = "payload";
    msg.token = token;
    msg.token_len = sizeof(token);
    msg.path = path;
    msg.path_len = sizeof(path);
    msg.query = query;
    msg.query_len = sizeof(query);
    msg.payload = payload;
    msg.payload_len = sizeof(payload);
    msg.msg_type = DUER_MSG_TYPE_CONFIRMABLE;
    msg.msg_code = DUER_RES_OP_GET;
    msg.msg_id = 0x2001;

    duer_addr_t addr;
    addr.type = DUER_PROTO_TCP;
    addr.port = 100;
    char host[] = "host";
    addr.host = host;
    addr.host_size = sizeof(host);

    char* action = test_malloc(sizeof(payload) + 1);
    expect_value(duer_malloc, size, sizeof(payload) + 1);
    will_return(duer_malloc, action);

    baidu_json* value = (baidu_json*)0x3001;
    expect_value(baidu_json_Parse, value, action);
    will_return(baidu_json_Parse, value);

    expect_value(duer_mutex_lock, mutex, MUTEXT);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, MUTEXT);
    will_return(duer_mutex_unlock, DUER_OK);

    size_t topic_id = duer_increase_topic_id();
    baidu_json* id = (baidu_json*)test_malloc(sizeof(baidu_json));
    id->valueint = topic_id;

    expect_value(baidu_json_GetObjectItem, object, value);
    expect_string(baidu_json_GetObjectItem, string, "id");
    will_return(baidu_json_GetObjectItem, id);

    expect_value(my_duer_voice_result_func, result, value);

    expect_value(baidu_json_Delete, c, value);

    expect_value(duer_free, ptr, action);

    int res = s_duer_voice_response(ctx, &msg, &addr);
    assert_string_equal(action, payload);
    assert_int_equal(res, DUER_OK);
    test_free(id);
    test_free(action);
}

void duer_voice_start_test(void** state) {
    expect_value(duer_speex_create, rate, SAMPLERATE);
    will_return(duer_speex_create, SPEEX_HANDLER);

    expect_value(duer_mutex_lock, mutex, MUTEXT);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, MUTEXT);
    will_return(duer_mutex_unlock, DUER_OK);

    int res = duer_voice_start(SAMPLERATE);
    assert_int_equal(res, DUER_OK);
}

void duer_voice_send_test(void** state) {
    const char* DATA = "HELLO WORLD!!";
    expect_value(duer_speex_encode, handler, SPEEX_HANDLER);
    expect_string(duer_speex_encode, data, DATA);
    expect_value(duer_speex_encode, size, sizeof(DATA));
    expect_not_value(duer_speex_encode, func, NULL);
    int res = duer_voice_send(DATA, sizeof(DATA));
    assert_int_equal(res, DUER_OK);
}

void duer_voice_stop_test(void** state) {
    const char test_data[] = "test     DATA";
    s_duer_encoded_func(test_data, sizeof(test_data));

    expect_value(duer_speex_encode, handler, SPEEX_HANDLER);
    expect_value(duer_speex_encode, data, NULL);
    expect_value(duer_speex_encode, size, 0);
    expect_not_value(duer_speex_encode, func, NULL);

    //begin duer_send_content
    expect_not_value(mbedtls_base64_encode, dst, NULL);
    expect_value(mbedtls_base64_encode, dlen, 800);
    expect_not_value(mbedtls_base64_encode, olen, NULL);
    expect_string(mbedtls_base64_encode, src, test_data);
    expect_value(mbedtls_base64_encode, slen, sizeof(test_data));
    will_return(mbedtls_base64_encode, DUER_OK);

    baidu_json* NUMBER = (baidu_json*)0x2001;
    baidu_json* voice = (baidu_json*)0x1234;
    will_return(baidu_json_CreateObject, voice);

    expect_not_value(baidu_json_CreateNumber, num, 0);
    will_return(baidu_json_CreateNumber, NUMBER);
    expect_value(baidu_json_AddItemToObject, object, voice);
    expect_string(baidu_json_AddItemToObject, string, "id");
    expect_value(baidu_json_AddItemToObject, item, NUMBER);

    expect_value(baidu_json_CreateNumber, num, 0);
    will_return(baidu_json_CreateNumber, NUMBER);
    expect_value(baidu_json_AddItemToObject, object, voice);
    expect_string(baidu_json_AddItemToObject, string, "segment");
    expect_value(baidu_json_AddItemToObject, item, NUMBER);

    expect_value(baidu_json_CreateNumber, num, SAMPLERATE);
    will_return(baidu_json_CreateNumber, NUMBER);
    expect_value(baidu_json_AddItemToObject, object, voice);
    expect_string(baidu_json_AddItemToObject, string, "rate");
    expect_value(baidu_json_AddItemToObject, item, NUMBER);

    expect_value(baidu_json_CreateNumber, num, 1);
    will_return(baidu_json_CreateNumber, NUMBER);
    expect_value(baidu_json_AddItemToObject, object, voice);
    expect_string(baidu_json_AddItemToObject, string, "channel");
    expect_value(baidu_json_AddItemToObject, item, NUMBER);

    expect_value(baidu_json_CreateNumber, num, 1);
    will_return(baidu_json_CreateNumber, NUMBER);
    expect_value(baidu_json_AddItemToObject, object, voice);
    expect_string(baidu_json_AddItemToObject, string, "eof");
    expect_value(baidu_json_AddItemToObject, item, NUMBER);

    baidu_json* STRING = (baidu_json*)0x3001;

    expect_not_value(baidu_json_CreateString, string, NULL);
    will_return(baidu_json_CreateString, STRING);
    expect_value(baidu_json_AddItemToObject, object, voice);
    expect_string(baidu_json_AddItemToObject, string, "voice_data");
    expect_value(baidu_json_AddItemToObject, item, STRING);

    baidu_json* value = (baidu_json*)0x3008;
    will_return(baidu_json_CreateObject, value);

    expect_value(baidu_json_AddItemToObject, object, value);
    expect_string(baidu_json_AddItemToObject, string, "voice_control");
    expect_value(baidu_json_AddItemToObject, item, voice);

    //end duer_send_content

    expect_value(duer_data_report, data, value);
    will_return(duer_data_report, DUER_OK);

    expect_value(baidu_json_Delete, c, value);

    expect_not_value(duer_speex_destroy, handler, NULL);
    int res = duer_voice_stop();
    assert_int_equal(res, DUER_OK);
}

void duer_increase_topic_id_test(void** state) {
    expect_value(duer_mutex_lock, mutex, MUTEXT);
    will_return(duer_mutex_lock, DUER_ERR_FAILED);
    size_t id = duer_increase_topic_id();
    assert_int_equal(id, 0);

    expect_value(duer_mutex_lock, mutex, MUTEXT);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, MUTEXT);
    will_return(duer_mutex_unlock, DUER_ERR_FAILED);

    id = duer_increase_topic_id();
    assert_int_equal(id, 0);

    expect_value(duer_mutex_lock, mutex, MUTEXT);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, MUTEXT);
    will_return(duer_mutex_unlock, DUER_OK);

    id = duer_increase_topic_id();
    assert_int_not_equal(id, 0);

}

CMOCKA_UNIT_TEST(duer_voice_set_result_callback_test);
CMOCKA_UNIT_TEST(duer_voice_initialize_test);
CMOCKA_UNIT_TEST(duer_increase_topic_id_test);
CMOCKA_UNIT_TEST(duer_voice_response_test);
CMOCKA_UNIT_TEST(duer_voice_start_test);
CMOCKA_UNIT_TEST(duer_voice_send_test);
CMOCKA_UNIT_TEST(duer_voice_stop_test);

