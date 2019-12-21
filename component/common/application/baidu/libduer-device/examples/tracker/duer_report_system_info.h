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
// Author: Zhong Shuai (zhongshuai@baidu.com)
//
// Desc: Report system infomation to Duer Cloud Head File

#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_TRACKER_DUER_REPORT_SYSTEM_INFO_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_TRACKER_DUER_REPORT_SYSTEM_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

enum LogLevel {
    LOG_EMERG,
    LOG_ALERT,
    LOG_CRIT,
    LOG_ERR,
    LOG_WARNING,
    LOG_NOTICE,
    LOG_INFO,
    LOG_DEBUG,
};

extern int duer_init_system_info_reporter(void);

extern int duer_report_static_system_info(void);

extern int duer_report_dynamic_system_info(void);

extern int duer_report_memory_info(void);

extern int duer_report_disk_info(void);

extern int duer_report_network_info(void);

extern int duer_report_system_log(enum LogLevel level);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_TRACKER_DUER_REPORT_SYSTEM_INFO_H
