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

baidu_json* baidu_json_CreateObject(void) {
    return mock_ptr_type(baidu_json*);
}

baidu_json* baidu_json_CreateNumber(double num) {
    check_expected(num);
    return mock_ptr_type(baidu_json*);
}

baidu_json *baidu_json_CreateBool(int b) {
    check_expected(b);
    return mock_ptr_type(baidu_json*);
}

void baidu_json_AddItemToArray(baidu_json *array, baidu_json *item) {
    check_expected(array);
    check_expected(item);
}

void baidu_json_AddItemToObject(baidu_json *object, const char *string, baidu_json *item) {
    check_expected(object);
    check_expected(string);
    check_expected(item);
}

baidu_json* baidu_json_CreateString(const char *string) {
    check_expected(string);
    return mock_ptr_type(baidu_json*);
}

void baidu_json_Delete(baidu_json *c) {
    check_expected(c);
}

void baidu_json_release(void *ptr) {
    check_expected(ptr);
}

baidu_json* baidu_json_Parse(const char *value) {
    check_expected(value);
    return mock_ptr_type(baidu_json*);
}

baidu_json* baidu_json_GetObjectItem(const baidu_json *object, const char *string) {
    check_expected(object);
    check_expected(string);
    return mock_ptr_type(baidu_json*);
}

baidu_json *baidu_json_CreateNull(void) {
    return mock_ptr_type(baidu_json*);
}

char* baidu_json_PrintUnformatted(const baidu_json* item) {
    check_expected(item);
    return mock_ptr_type(char*);
}
