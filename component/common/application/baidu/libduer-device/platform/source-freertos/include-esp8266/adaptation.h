/*
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
 *
 * File: adaptation.h
 * Auth: Zhong Shuai (zhongshuai@baidu.com)
 * Desc: Adapte ESP8266 SDK FreeRTOS
 */

#ifndef BAIDU_DUER_ADAPTATION_H
#define BAIDU_DUER_ADAPTATION_H

#include "freertos/FreeRTOSConfig.h"
#include "freertos/timers.h"
#include "freertos/portmacro.h"

#ifdef __cplusplus
extern "C" {
#endif

#define portTICK_PERIOD_MS ((TickType_t)1000 / configTICK_RATE_HZ)

#define TimerHandle_t        xTimerHandle

#define SemaphoreHandle_t    xSemaphoreHandle

#define TaskHandle_t         xTaskHandle

#define QueueHandle_t        xQueueHandle

typedef long BaseType_t;

#if( configUSE_16_BIT_TICKS == 1 )
	typedef uint16_t TickType_t;

#ifndef portMAX_DELAY
	#define portMAX_DELAY ( TickType_t ) 0xffff
#endif

#else
	typedef uint32_t TickType_t;

#ifndef portMAX_DELAY
	#define portMAX_DELAY ( TickType_t ) 0xffffffffUL
#endif

	/* 32-bit tick type on a 32-bit architecture, so reads of the tick count do
	not need to be guarded with a critical section. */
	#define portTICK_TYPE_IS_ATOMIC 1
#endif

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_ADAPTATION_H
