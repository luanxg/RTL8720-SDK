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
 * File: lightduer_adapter_config.c
 * Auth: Leliang Zhang(zhangleliang@baidu.com)
 * Desc: the default implements for the 'weak' functions
 */
#include "lightduer_adapter_config.h"
//Edit by Realtek
#include "FreeRTOS.h"

#ifdef DUER_PLATFORM_ESP8266
#include "adaptation.h"
#endif

__attribute__((weak)) int duer_get_port_tick_period_ms(void)
{
    //printf("lib tpm:%d\n", portTICK_PERIOD_MS);
    return portTICK_PERIOD_MS;
}

