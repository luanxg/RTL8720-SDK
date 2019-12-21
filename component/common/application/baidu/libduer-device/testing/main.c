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

#include <stdio.h>
#include "test.h"

extern struct CMUnitTest _unittest_start;
extern struct CMUnitTest _unittest_end;

int main() {
    size_t test_count = &_unittest_end - &_unittest_start;
    typedef struct CMUnitTest (*tests_array)[test_count];
    tests_array tests = (tests_array)&_unittest_start;

    //printf("size of element:%ld\n", sizeof(struct CMUnitTest));
    //printf("size of element:%ld\n", sizeof(tests[0][0]));
    //printf("size of element:%ld\n", sizeof(tests[0][1]));
    //printf("size of element:%ld\n", sizeof(_unittest_start));
    //printf("size of array:%ld\n", sizeof(tests[0]));
    printf("test_count:%ld\n", test_count);
    //printf("count of array:%ld\n", sizeof(tests[0])/sizeof(tests[0][0]));
    //printf("1 of array:%p\n", &(tests[0][0]));
    //printf("2 of array:%p\n", &(tests[0][1]));
    return cmocka_run_group_tests(tests[0], NULL, NULL);
}
