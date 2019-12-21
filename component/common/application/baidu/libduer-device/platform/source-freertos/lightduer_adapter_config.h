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
/**
 * File: lightduer_adapter_config.h
 * Auth: Leliang Zhang(zhangleliang@baidu.com)
 * Desc: platform dependent functions
 *       when provide as lib, some functions should be re-implemented.
 */

#ifndef BAIDU_DUER_LIGHTDUER_ADAPTER_CONFIG_H
#define BAIDU_DUER_LIGHTDUER_ADAPTER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * this function need re-implement when libduer-device provided as a lib
 */
int duer_get_port_tick_period_ms(void);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIGHTDUER_ADAPTER_CONFIG_H
