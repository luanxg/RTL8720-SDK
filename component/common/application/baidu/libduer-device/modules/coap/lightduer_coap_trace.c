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
// Author: Su Hao (suhao@baidu.com)
//
// Description: Adapter for mbed-trace.

#include "lightduer_coap_trace.h"
#include "lightduer_lib.h"
#include "mbed-trace/mbed_trace.h"
#include "lightduer_log.h"

// Suppress Compiler warning Function declared never referenced
#define SUPPRESS_WARNING(func) (void)(func)

DUER_LOC_IMPL void duer_coap_mbed_trace(const char* msg) {
    //DUER_LOGI_EXT(NULL, 0, "### %s", msg);
    DUER_LOGI("### %s", msg);
}

DUER_INT_IMPL void duer_coap_trace_enable(void) {
    SUPPRESS_WARNING(duer_coap_mbed_trace);
    mbed_trace_init();
    mbed_trace_config_set(TRACE_MODE_PLAIN | TRACE_ACTIVE_LEVEL_ALL);
    mbed_trace_print_function_set(duer_coap_mbed_trace);
}

DUER_INT_IMPL void duer_coap_trace_disable(void) {
    mbed_trace_free();
}
