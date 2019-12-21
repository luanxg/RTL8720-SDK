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
// Description: parser for the args

#include "duerapp_args.h"

#include <stdio.h>
#include <stdlib.h>

//#define DUERAPP_ARGS_DEBUG
#ifdef DUERAPP_ARGS_DEBUG
#define DEBUG_E(...) printf("[ERROR] in func:%s, line:%d", __func__, __LINE__), \
                     printf(__VA_ARGS__), \
                     printf("\n");
#define DEBUG_I(...) printf("[INFO] in func:%s, line:%d", __func__, __LINE__), \
                     printf(__VA_ARGS__), \
                     printf("\n");
#else
#define DEBUG_E(...)
#define DEBUG_I(...)
#endif

static void duer_args_usage(void) {
    fprintf(stderr, "Usage: %s [options]\n\n", PROGRAM_NAME);
    fprintf(stderr,
    "Optins:\n"
    "-p|--profile  the profile which will be used\n"
    "-r|--record   the dirctory hold the voice records or the record file\n"
    "-i|--interval the interval between two voice query in second, default is 5\n"
    "-h|--help     Print this message\n\n"
    );
}

void duer_args_parse(int argc, char* argv[], duer_args_t* args, bool allow_default) {
    const char shortOptions[] = "p:r:i:h";
    // value for the 2nd : no_argument; required_argument; optional_argument
    const struct option longOptions[] = {
        {"profile", required_argument, NULL, 'p'},
        {"record",  required_argument, NULL, 'r'},
        {"interval",  required_argument, NULL, 'i'},
        {0, 0, 0, 0}
    };

    int index = -1;
    int c = 0;
    if (args == NULL) {
        DEBUG_E("in func: %s, args is NULL\n", __func__);
        return;
    }
    bool no_param = true;
    while (1) {
        c = getopt_long(argc, argv, shortOptions, longOptions, NULL);

        if (c == -1) { // finish parse
            DEBUG_I(" 0 ");
            break;
        }
        if (no_param) {
            no_param = false;
        }
        DEBUG_I(" c[%c] ", c);
        switch (c) {
        case 0:
            break;
        case 'p':
            args->profile_path = optarg;
            break;
        case 'r':
            args->voice_record_path = optarg;
            break;
        case 'i':
            args->interval_between_query = (short)atoi(optarg);
            break;
        case 'h':
            duer_args_usage();
            exit(EXIT_SUCCESS);
            break;
        default:
            DEBUG_E(" c:[%c]", c);
            duer_args_usage();
            exit(EXIT_FAILURE);
            break;
        }
    }
    if (no_param && !allow_default) {
        duer_args_usage();
        exit(EXIT_FAILURE);
    }
}