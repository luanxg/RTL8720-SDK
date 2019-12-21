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
// The daemon for holding lightduer.

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "duer_daemon.h"
#include "duer_profile.h"
#include "duer_log.h"

#include "duer_system_info.h"
#include "duer_report_system_info.h"
#include "lightduer_connagent.h"
#include "lightduer_types.h"
#include "lightduer_mutex.h"
#include "lightduer_log.h"

extern int duer_init_system_info(void);

static void duer_events_listener(duer_event_t *event) {
    int ret = DUER_OK;

    if (event) {
        switch (event->_event) {
        case DUER_EVENT_STARTED:
            LOGI("lightduer has started");

            ret = duer_report_static_system_info();
            if (ret != DUER_OK) {
                LOGE("Report static system info failed ret: %d", ret);
            }

            break;
        case DUER_EVENT_STOPPED:
            LOGI("lightduer has stoped");
            break;
        default:
            LOGE("error event (%d) triggered", event->_event);
            break;
        }
    }
}

int duer_daemon_run(const char *path) {
    int ret = DUER_OK;

    duer_prof_handler profile = duer_profile_create(path);

    duer_initialize();

    duer_set_event_callback(duer_events_listener);

    duer_start(duer_profile_get_data(profile), duer_profile_get_size(profile));

    ret = duer_init_system_info();
    if (ret != DUER_OK) {
        DUER_LOGE("Init system info failed");

        return -EAGAIN;
    }

    ret = duer_system_info_init();
    if (ret != DUER_OK) {
        DUER_LOGE("Init system info failed");

        return -EAGAIN;
    }

    ret = duer_init_system_info_reporter();
    if (ret != DUER_OK) {
        DUER_LOGE("Init system info reporter failed");

        return -EAGAIN;
    }

    DUER_LOGI("Init system info successful");

    while (1) {
        sleep(60);
    }

    duer_profile_destroy(profile);

    return 0;
}
