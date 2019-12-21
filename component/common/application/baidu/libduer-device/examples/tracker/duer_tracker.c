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
// Author: suhao@baidu.com(suhao@baidu.com)
//
// Realize a tracker for report system information.

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <libgen.h>

#include "duer_daemon.h"

static void usage(const char *name) {
    fprintf(stderr, "\nUsage: %s [options]\n\n"
                    "Options:\n"
                    "\t-p|--profile  the profile which will be used\n"
                    "\t-h|--help     Print this message\n\n", basename((char *)name));
}

int main(int args, char *argv[]) {
    int rs;
    const char *profile = NULL;
    const char shortOptions[] = "p:h";
    const struct option longOptions[] = {
        {"profile", required_argument, NULL, 'p'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    do {
        rs = getopt_long(args, argv, shortOptions, longOptions, NULL);

        switch (rs) {
        case -1:
            break;
        case 'p':
            profile = optarg;
            break;
        case 'h':
        default:
            goto help;
        }
    } while (rs != -1);

    if (profile == NULL) {
        fprintf(stderr, "Not specified the profile!!!\n");
        goto help;
    }

    duer_daemon_run(profile);

    goto exit;

help:

    usage(argv[0]);

exit:

    return 0;
}
