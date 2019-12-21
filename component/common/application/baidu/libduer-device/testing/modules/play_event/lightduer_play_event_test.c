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
#include "lightduer_play_event.h"
#include "lightduer_mutex.h"
#include "lightduer_types.h"

int duer_data_report(const baidu_json* data) {
    check_expected(data);
    return mock_type(int);
}

size_t duer_increase_topic_id(void) {
    return mock_type(size_t);
}
//--------
static const duer_mutex_t s_mutex = (duer_mutex_t)0x4004;
static const int s_topic_id = 10000;

void duer_init_play_event_test(void** state) {
    will_return(duer_mutex_create, NULL);
    int res = duer_init_play_event();
    assert_int_equal(res, DUER_ERR_FAILED);

    will_return(duer_mutex_create, s_mutex);
    expect_value(duer_mutex_unlock, mutex, s_mutex);
    will_return(duer_mutex_unlock, DUER_OK);

    res = duer_init_play_event();
    assert_int_equal(res, DUER_OK);
}

void duer_set_play_info_test(void** state) {
    int res = duer_set_play_info(NULL);
    assert_int_equal(res, DUER_ERR_INVALID_PARAMETER);
    //-------------
    struct PlayInfo play_info;
    play_info.message_id = "message_id";
    play_info.audio_item_id = "audio_item_id";
    play_info.topic_id = s_topic_id;

    expect_value(duer_mutex_lock, mutex, s_mutex);
    will_return(duer_mutex_lock, DUER_ERR_FAILED);

    res = duer_set_play_info(&play_info);
    assert_int_equal(res, DUER_ERR_FAILED);
    //-------------
    play_info.audio_item_id = "1234567890123456789012345678901";
    res = duer_set_play_info(&play_info);
    assert_int_equal(res, DUER_ERR_INVALID_PARAMETER);
    //-----------------
    play_info.audio_item_id = "audio_item_id";
    play_info.message_id =
        "12345678901234567890123456789012345678901234567890123456789012345678901";
    res = duer_set_play_info(&play_info);
    assert_int_equal(res, DUER_ERR_INVALID_PARAMETER);
    //-----------------
    play_info.message_id = "message_id";
    play_info.audio_item_id = "audio_item_id";

    expect_value(duer_mutex_lock, mutex, s_mutex);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, s_mutex);
    will_return(duer_mutex_unlock, DUER_OK);

    res = duer_set_play_info(&play_info);
    assert_int_equal(res, DUER_OK);
    //-----------------
}

void duer_set_play_source_test(void** state) {
    int ret = duer_report_play_status(-1);
    assert_int_equal(ret, DUER_ERR_INVALID_PARAMETER);
    //---------
    expect_value(duer_mutex_lock, mutex, s_mutex);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, s_mutex);
    will_return(duer_mutex_unlock, DUER_OK);

    ret = duer_set_play_source(PLAY_LOCAL_FILE);
    assert_int_equal(ret, DUER_OK);
    ret = duer_report_play_status(START_PLAY);
    assert_int_equal(ret, DUER_ERR_FAILED);
    //---------
    expect_value(duer_mutex_lock, mutex, s_mutex);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, s_mutex);
    will_return(duer_mutex_unlock, DUER_OK);

    ret = duer_set_play_source(PLAY_NET_FILE);
    assert_int_equal(ret, DUER_OK);

    will_return(baidu_json_CreateObject, NULL);
    ret = duer_report_play_status(START_PLAY);
    assert_int_equal(ret, DUER_ERR_MEMORY_OVERLOW);
    //-------
    baidu_json* data = (baidu_json*)0x2001;
    will_return(baidu_json_CreateObject, data);
    will_return(baidu_json_CreateObject, NULL);

    expect_value(baidu_json_Delete, c, data);

    ret = duer_report_play_status(START_PLAY);
    assert_int_equal(ret, DUER_ERR_MEMORY_OVERLOW);
    //-------
    baidu_json* speech_info = (baidu_json*)0x3001;
    will_return(baidu_json_CreateObject, data);
    will_return(baidu_json_CreateObject, speech_info);
    will_return(baidu_json_CreateObject, NULL);

    expect_value(baidu_json_Delete, c, speech_info);
    expect_value(baidu_json_Delete, c, data);

    ret = duer_report_play_status(START_PLAY);
    assert_int_equal(ret, DUER_ERR_MEMORY_OVERLOW);
    //======================
    baidu_json* play_info = (baidu_json*)0x4001;
    will_return_count(baidu_json_CreateObject, data, 1);
    will_return_count(baidu_json_CreateObject, speech_info, 1);
    will_return_count(baidu_json_CreateObject, play_info, 1);

    expect_value_count(baidu_json_Delete, c, play_info, 1);
    //-----------
    baidu_json* STRING = (baidu_json*)0xA001;
    expect_string(baidu_json_CreateString, string, "message_id");
    will_return(baidu_json_CreateString, STRING);
    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "message_id");
    expect_value(baidu_json_AddItemToObject, item, STRING);

    baidu_json* JNULL = (baidu_json*)0xA002;
    will_return(baidu_json_CreateNull, JNULL);
    expect_value(baidu_json_AddItemToObject, object, speech_info);
    expect_string(baidu_json_AddItemToObject, string, "type");
    expect_value(baidu_json_AddItemToObject, item, JNULL);

    expect_value(baidu_json_AddItemToObject, object, speech_info);
    expect_string(baidu_json_AddItemToObject, string, "data");
    expect_value(baidu_json_AddItemToObject, item, data);
    //START_PLAY
    baidu_json* NUMBER = (baidu_json*)0xA003;
    expect_value(baidu_json_CreateNumber, num, s_topic_id);
    will_return(baidu_json_CreateNumber, NUMBER);
    expect_value(baidu_json_AddItemToObject, object, speech_info);
    expect_string(baidu_json_AddItemToObject, string, "id");
    expect_value(baidu_json_AddItemToObject, item, NUMBER);

    expect_string(baidu_json_CreateString, string, "audio_item_id");
    will_return(baidu_json_CreateString, STRING);
    expect_value(baidu_json_AddItemToObject, object, data);
    expect_string(baidu_json_AddItemToObject, string, "token");
    expect_value(baidu_json_AddItemToObject, item, STRING);

    expect_value(baidu_json_AddItemToObject, object, play_info);
    expect_string(baidu_json_AddItemToObject, string, "play_start");
    expect_value(baidu_json_AddItemToObject, item, speech_info);

    expect_value(baidu_json_PrintUnformatted, item, play_info);
    will_return(baidu_json_PrintUnformatted, "hello test");

    expect_value(duer_data_report, data, play_info);
    will_return(duer_data_report, DUER_OK);

    ret = duer_report_play_status(START_PLAY);
    assert_int_equal(ret, DUER_OK);
}

void duer_report_play_status_test(void** state) {
    //PLAY_NET_FILE PLAY_LOCAL_FILE
    int ret = duer_set_play_source(-100);
    assert_int_equal(ret, DUER_ERR_INVALID_PARAMETER);
    //--------
    expect_value(duer_mutex_lock, mutex, s_mutex);
    will_return(duer_mutex_lock, DUER_ERR_FAILED);

    ret = duer_set_play_source(PLAY_NET_FILE);
    assert_int_equal(ret, DUER_ERR_FAILED);
    //--------
    expect_value(duer_mutex_lock, mutex, s_mutex);
    will_return(duer_mutex_lock, DUER_OK);

    expect_value(duer_mutex_unlock, mutex, s_mutex);
    will_return(duer_mutex_unlock, DUER_OK);

    ret = duer_set_play_source(PLAY_NET_FILE);
    assert_int_equal(ret, DUER_OK);

}

CMOCKA_UNIT_TEST(duer_init_play_event_test);
CMOCKA_UNIT_TEST(duer_set_play_info_test);
CMOCKA_UNIT_TEST(duer_set_play_source_test);
CMOCKA_UNIT_TEST(duer_report_play_status_test);

