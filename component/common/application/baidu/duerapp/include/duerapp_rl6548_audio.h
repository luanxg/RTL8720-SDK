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
 * File: duerapp_audio_record.h
 * Auth: Jingjun Wu (jingjun_wu@realsil.com.cn)
 * Desc: codec recorder files
 */

#ifndef DUERAPP_RL6548_AUDIO_H
#define DUERAPP_RL6548_AUDIO_H

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
#include "duerapp.h"

#define SP_DMA_PAGE_NUM		4   // Vaild number is 2~4
#define SP_FULL_BUF_SIZE	128
#define SP_ZERO_BUF_SIZE	128
#define AUDIO_FREQ_NUM 		7
#define AUDIO_RX_DMA_PAGE_SIZE	640  //(640bytes * 8bits/byte)/(16K samples/second * 16bits/sample) = 20ms
#define SP_DMA_PAGE_SIZE	AUDIO_RX_DMA_PAGE_SIZE   // 2 ~ 4096

typedef struct {
	GDMA_InitTypeDef       	SpTxGdmaInitStruct;              //Pointer to GDMA_InitTypeDef
	GDMA_InitTypeDef       	SpRxGdmaInitStruct;              //Pointer to GDMA_InitTypeDef		
}SP_GDMA_STRUCT, *pSP_GDMA_STRUCT;

typedef struct {
	u8 tx_gdma_own;
	u32 tx_addr;
	u32 tx_length;
	
}TX_BLOCK, *pTX_BLOCK;

typedef struct {
	TX_BLOCK tx_block[SP_DMA_PAGE_NUM];
	TX_BLOCK tx_zero_block;
	u8 tx_gdma_cnt;
	u8 tx_usr_cnt;
	u8 tx_empty_flag;
	
}SP_TX_INFO, *pSP_TX_INFO;

typedef struct {
	u8 rx_gdma_own;
	u32 rx_addr;
	u32 rx_length;
	
}RX_BLOCK, *pRX_BLOCK;

typedef struct {
	RX_BLOCK rx_block[SP_DMA_PAGE_NUM];
	RX_BLOCK rx_full_block;
	u8 rx_gdma_cnt;
	u8 rx_usr_cnt;
	u8 rx_full_flag;
	
}SP_RX_INFO, *pSP_RX_INFO;

typedef struct {
	u32 sample_rate;
	u32 word_len;
	u32 mono_stereo;
	u32 direction;
	u32 mono_channel_sel;
}SP_OBJ, *pSP_OBJ;

extern SP_GDMA_STRUCT SPGdmaStruct;
extern unsigned char audio_comm_buf[AUDIO_MAX_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
extern SP_OBJ sp_obj;
extern SP_InitTypeDef SP_InitStruct;
extern void audio_deinit(void);
extern void audio_set_sp_clk(u32 rate);
extern T_APP_DUER *pDuerAppInfo;

#endif // DUERAPP_RL6548_AUDIO_H
