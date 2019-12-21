/**
 * Copyright (2017) Realsil Inc. All rights reserved.
 *
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: duerapp_alc6580_audio.h
 * Auth: Jingjun Wu (jingjun_wu@realsil.com.cn)
 * Desc: codec recorder files
 */

#ifndef DUERAPP_ALC6580_AUDIO_H
#define DUERAPP_ALC6580_AUDIO_H

#include "FreeRTOS.h"
#include "task.h"
#include "section_config.h"
#include "osdep_service.h"
#include <platform_stdlib.h>

#include "dma_api.h"
#include "rl6548.h"
#include "gpio_api.h"
#include "gpio_irq_api.h"
#include "timer_api.h"

#include "duerapp_config.h"
#include "queue.h"

#define I2S_DMA_PAGE_NUM    4   // Vaild number is 2~4
#define I2S_RX_PAGE_SIZE	640  //(640bytes * 8bits/byte)/(16K samples/second * 16bits/sample) = 20ms
#define I2S_FREQ_NUM 7

#define I2S_SCLK_PIN            PC_1
#define I2S_WS_PIN              PC_0
#define I2S_SD_PIN              PC_2

extern u8 i2s_comm_buf[AUDIO_MAX_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
extern i2s_t i2s_obj;
extern T_APP_DUER *pDuerAppInfo;
#endif
