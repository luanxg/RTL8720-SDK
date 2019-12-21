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

#include "lightduer_connagent.h"
#include "lightduer_engine.h"
#include "lightduer_events.h"

#define G_EVENTS_HANDLER 0x1001

static duer_engine_notify_func s_engin_notify_func;

void duer_voice_initialize(void) {
}

void duer_voice_finalize(void) {
}

void duer_dcs_router_init(void) {
}

void baidu_json_release(void *ptr) {
    check_expected(ptr);
}

void duer_events_destroy(duer_events_handler handler) {
    check_expected(handler);
}

void duer_engine_stop(int what, void *object) {
    check_expected(what);
    check_expected(object);
}

void duer_engine_start(int what, void *object) {
}

void duer_engine_add_resources(int what, void *object) {
}

void duer_engine_send(int what, void *object) {
}

int duer_engine_enqueue_report_data(const baidu_json *data) {
    check_expected(data);
    return mock_type(int);
}

void duer_engine_data_available(int what, void *object) {
}

void duer_engine_create(int what, void *object) {
}

void duer_engine_destroy(int what, void *object) {
}

void baidu_ca_adapter_initialize() {
    return;
}

duer_events_handler duer_events_create(const char *name,
        size_t stack_size, size_t queue_length) {
    check_expected(name);
    check_expected(stack_size);
    check_expected(queue_length);
    return mock_ptr_type(duer_events_handler);
}

void duer_engine_register_notify(duer_engine_notify_func func) {
    check_expected(func);
    s_engin_notify_func = func;
    return;
}

int duer_events_call(duer_events_handler handler, duer_events_func func,
        int what, void *object) {
    check_expected(handler);
    check_expected(func);
    check_expected(what);
    check_expected(object);
    return mock_type(int);
}

void my_duer_event_callback(duer_event_t* event) {
    check_expected(event->_event);
    check_expected(event->_object);
}

//begin test cases
void duer_set_event_callback_test(void** state) {
    duer_set_event_callback(NULL);
    duer_set_event_callback(my_duer_event_callback);
}

void duer_initialize_test(void** state) {
    expect_string(duer_events_create, name, "lightduer_ca");
    expect_value(duer_events_create, stack_size, 6 * 1024); // #define TASK_STACK_SIZE (1024 * 6)
    expect_value(duer_events_create, queue_length, 10); // #define QUEUE_LENGTH    (10)
    will_return(duer_events_create, NULL);
    expect_not_value(duer_engine_register_notify, func, NULL);

    duer_initialize();
    //
    expect_string(duer_events_create, name, "lightduer_ca");
    expect_value(duer_events_create, stack_size, 6 * 1024); // #define TASK_STACK_SIZE (1024 * 6)
    expect_value(duer_events_create, queue_length, 10); // #define QUEUE_LENGTH    (10)
    will_return(duer_events_create, G_EVENTS_HANDLER);
    expect_not_value(duer_engine_register_notify, func, NULL);

    expect_value(duer_events_call, handler, G_EVENTS_HANDLER);
    expect_not_value(duer_events_call, func, NULL);
    expect_value(duer_events_call, what, 0);
    expect_value(duer_events_call, object, NULL);
    will_return(duer_events_call, DUER_OK);
    duer_initialize();
}

void duer_start_test(void** state) {
    char mydata[] = "hello world";

    expect_value(duer_malloc, size, sizeof(mydata));
    will_return(duer_malloc, NULL);

    int res = duer_start(mydata, sizeof(mydata));
    assert_int_equal(res, DUER_ERR_MEMORY_OVERLOW);
    //=====
    void* buffer = test_malloc(sizeof(mydata));

    expect_value(duer_malloc, size, sizeof(mydata));
    will_return(duer_malloc, buffer);

    expect_value(duer_events_call, handler, G_EVENTS_HANDLER);
    expect_not_value(duer_events_call, func, NULL);
    expect_value(duer_events_call, what, sizeof(mydata));
    expect_value(duer_events_call, object, buffer);
    will_return(duer_events_call, DUER_ERR_FAILED);

    expect_value(duer_free, ptr, buffer);
    res = duer_start(mydata, sizeof(mydata));
    assert_int_equal(res, DUER_ERR_FAILED);
    //====
    expect_value(duer_malloc, size, sizeof(mydata));
    will_return(duer_malloc, buffer);

    expect_value(duer_events_call, handler, G_EVENTS_HANDLER);
    expect_not_value(duer_events_call, func, NULL);
    expect_value(duer_events_call, what, sizeof(mydata));
    expect_value(duer_events_call, object, buffer);
    will_return(duer_events_call, DUER_OK);

    res = duer_start(mydata, sizeof(mydata));
    assert_string_equal(buffer, mydata);

    test_free(buffer);
}

void duer_add_resources_test(void** state) {
    duer_add_resources(NULL, 0);
    duer_add_resources(NULL, 1);
    duer_add_resources((duer_res_t*)0x100, 0);
    //===========
    expect_value(duer_malloc, size, sizeof(duer_res_t));
    will_return(duer_malloc, NULL);
    duer_add_resources((duer_res_t*)0x100, 1);
    //=========
    duer_res_t* buffer = (duer_res_t*)test_malloc(sizeof(duer_res_t));
    memset(buffer, 0, sizeof(duer_res_t));
    int mode = 1;
    int allowed = 6;
    char* path = "/play";
    char data[] = "hello world";
    duer_size_t size = sizeof(data);
    duer_res_t resource = {mode, allowed, path, .res.s_res={data, size}};
    //----------
    expect_value(duer_malloc, size, sizeof(duer_res_t));
    will_return(duer_malloc, buffer);

    expect_value(duer_events_call, handler, G_EVENTS_HANDLER);
    expect_not_value(duer_events_call, func, NULL);
    expect_value(duer_events_call, what, 1);
    expect_value(duer_events_call, object, buffer);
    will_return(duer_events_call, DUER_ERR_FAILED);

    expect_value(duer_free, ptr, buffer);

    duer_add_resources(&resource, 1);
    //==========
    expect_value(duer_malloc, size, sizeof(duer_res_t));
    will_return(duer_malloc, buffer);

    expect_value(duer_events_call, handler, G_EVENTS_HANDLER);
    expect_not_value(duer_events_call, func, NULL);
    expect_value(duer_events_call, what, 1);
    expect_value(duer_events_call, object, buffer);
    will_return(duer_events_call, DUER_OK);

    duer_add_resources(&resource, 1);
    assert_int_equal(buffer->mode, mode);
    assert_int_equal(buffer->allowed, allowed);
    assert_string_equal(buffer->path, path);
    assert_string_equal(buffer->res.s_res.data, data);
    assert_int_equal(buffer->res.s_res.size, size);

    test_free(buffer);
}

void duer_data_report_test(void** state) {

    int res = duer_data_report(NULL);
    assert_int_equal(res, DUER_ERR_INVALID_PARAMETER);

    expect_value(duer_engine_enqueue_report_data, data, 0x2002);
    will_return(duer_engine_enqueue_report_data, DUER_ERR_FAILED);
    res = duer_data_report((baidu_json*)0x2002);
    assert_int_equal(res, DUER_ERR_FAILED);

    expect_value(duer_engine_enqueue_report_data, data, 0x2002);
    will_return(duer_engine_enqueue_report_data, DUER_OK);

    expect_value(duer_events_call, handler, G_EVENTS_HANDLER);
    expect_not_value(duer_events_call, func, NULL);
    expect_value(duer_events_call, what, 0);
    expect_value(duer_events_call, object, NULL);
    will_return(duer_events_call, DUER_OK);
    res = duer_data_report((baidu_json*)0x2002);
    assert_int_equal(res, DUER_OK);
}

//int duer_response(int msg_code, const void *data, size_t size);

void duer_data_available_test(void** state) {
    expect_value(duer_events_call, handler, G_EVENTS_HANDLER);
    expect_not_value(duer_events_call, func, NULL);
    expect_value(duer_events_call, what, 0);
    expect_value(duer_events_call, object, NULL);
    will_return(duer_events_call, DUER_OK);
    int res = duer_data_available();
    assert_int_equal(res, DUER_OK);
}

void duer_stop_test(void** state) {
    expect_value(duer_events_call, handler, G_EVENTS_HANDLER);
    expect_not_value(duer_events_call, func, NULL);
    expect_value(duer_events_call, what, 0);
    expect_value(duer_events_call, object, NULL);
    will_return(duer_events_call, DUER_OK);
    int res = duer_stop();
    assert_int_equal(res, DUER_OK);
}

void duer_finalize_test() {
    expect_value(duer_events_call, handler, G_EVENTS_HANDLER);
    expect_not_value(duer_events_call, func, NULL);
    expect_value(duer_events_call, what, 0);
    expect_value(duer_events_call, object, NULL);
    will_return(duer_events_call, DUER_OK);
    int res = duer_finalize();
    assert_int_equal(res, DUER_OK);
}

void duer_engine_notify_callback_test() {
    // begin test function duer_engine_notify_callback save in s_engin_notify_func
    if (s_engin_notify_func) {
        expect_value(duer_engine_stop, what, 0);
        expect_value(duer_engine_stop, object, NULL);
        s_engin_notify_func(DUER_EVT_CREATE, DUER_ERR_FAILED, 0, NULL);
        //---------------
        expect_value(duer_free, ptr, 0x4001);
        s_engin_notify_func(DUER_EVT_START, DUER_ERR_TRANS_WOULD_BLOCK, 0, (void*)0x4001);
        //---------------------
        expect_value(duer_engine_stop, what, 0);
        expect_value(duer_engine_stop, object, NULL);
        s_engin_notify_func(DUER_EVT_START, DUER_ERR_FAILED, 0, NULL);
        //-----------------
        expect_value(my_duer_event_callback, event->_event, DUER_EVENT_STARTED);
        expect_value(my_duer_event_callback, event->_object, NULL);
        s_engin_notify_func(DUER_EVT_START, DUER_OK, 0, NULL);
        //----------
        expect_value(duer_free, ptr, 0x4001);
        s_engin_notify_func(DUER_EVT_ADD_RESOURCES, DUER_OK, 0, (void*)0x4001);
        //------------------
        const char* test_obj = "Hello world";
        expect_value(baidu_json_release, ptr, test_obj);
        s_engin_notify_func(DUER_EVT_SEND_DATA, DUER_OK, 0, (void*)test_obj);
        //-----------------
        s_engin_notify_func(DUER_EVT_DATA_AVAILABLE, DUER_OK, 0, NULL);
        //-----------------
        expect_value(my_duer_event_callback, event->_event, DUER_EVENT_STOPPED);
        expect_value(my_duer_event_callback, event->_object, NULL);
        s_engin_notify_func(DUER_EVT_STOP, DUER_OK, 0, NULL);
        //-----------------
        expect_value(duer_events_destroy, handler, G_EVENTS_HANDLER);
        s_engin_notify_func(DUER_EVT_DESTROY, DUER_OK, 0, NULL);
        //-----------------
        s_engin_notify_func(1000000, DUER_OK, 0, NULL);
        //-----------------
    } else {
        assert_int_equal(10, 1);// mock fail
    }
}

CMOCKA_UNIT_TEST(duer_set_event_callback_test);
CMOCKA_UNIT_TEST(duer_initialize_test);
CMOCKA_UNIT_TEST(duer_start_test);
CMOCKA_UNIT_TEST(duer_add_resources_test);
CMOCKA_UNIT_TEST(duer_data_report_test);
CMOCKA_UNIT_TEST(duer_data_available_test);
CMOCKA_UNIT_TEST(duer_stop_test);
CMOCKA_UNIT_TEST(duer_finalize_test);
CMOCKA_UNIT_TEST(duer_engine_notify_callback_test);

