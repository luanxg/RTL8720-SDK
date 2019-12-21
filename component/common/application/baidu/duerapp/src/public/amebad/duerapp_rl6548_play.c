#include "duerapp_audio.h"
#include "duerapp_rl6548_audio.h"
#include "rtl8721d_audio.h"
#include <platform/platform_stdlib.h>
#include "rl6548.h"
#include "duerapp.h"
#include "duerapp_media.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "lightduer_types.h"


static int resample = 0; //0,not need resample; 1,stereo; 2,mono
static unsigned int audio_dma_page_sz;
static SP_TX_INFO sp_tx_info;
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];
static u32 play_has_inited = 0;
SDRAM_DATA_SECTION static unsigned int audio_freq_table[AUDIO_FREQ_NUM]={
    8000, 16000, 22050, 24000, 32000, 44100, 48000,
};
SDRAM_DATA_SECTION static unsigned int audio_dma_page_sz_table[2][AUDIO_FREQ_NUM]={
    {1152, 1152,1152,1152,2304,2304,2304},
    {2304, 2304,2304,2304,4608,4608,4608},
};

static u8 *sp_get_free_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);
	
	if (ptx_block->tx_gdma_own)
		return NULL;
	else{
		return (u8 *)ptx_block->tx_addr;
	}
}

static void sp_write_tx_page()
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);
	
	ptx_block->tx_gdma_own = 1;
	sp_tx_info.tx_usr_cnt++;
	if (sp_tx_info.tx_usr_cnt == SP_DMA_PAGE_NUM){
		sp_tx_info.tx_usr_cnt = 0;
	}
}

static void sp_release_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);
	
	if (sp_tx_info.tx_empty_flag){
	}
	else{
		ptx_block->tx_gdma_own = 0;
		sp_tx_info.tx_gdma_cnt++;
		if (sp_tx_info.tx_gdma_cnt == SP_DMA_PAGE_NUM){
			sp_tx_info.tx_gdma_cnt = 0;
		}
	}
}

static u8 *sp_get_ready_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);
	
	if (ptx_block->tx_gdma_own){
		sp_tx_info.tx_empty_flag = 0;
		return (u8 *)ptx_block->tx_addr;
	}
	else{
		sp_tx_info.tx_empty_flag = 1;
		return (u8 *)sp_tx_info.tx_zero_block.tx_addr;	//for audio buffer empty case
	}
}

static u32 sp_get_ready_tx_length(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);

	if (sp_tx_info.tx_empty_flag){
		return sp_tx_info.tx_zero_block.tx_length;
	}
	else{
		return ptx_block->tx_length;
	}
}

static void sp_init_tx_variables(void)
{
	int i;

	for(i=0; i<SP_ZERO_BUF_SIZE; i++){
		sp_zero_buf[i] = 0;
	}
	sp_tx_info.tx_zero_block.tx_addr = (u32)sp_zero_buf;
	sp_tx_info.tx_zero_block.tx_length = (u32)SP_ZERO_BUF_SIZE;
	
	sp_tx_info.tx_gdma_cnt = 0;
	sp_tx_info.tx_usr_cnt = 0;
	sp_tx_info.tx_empty_flag = 0;
	
	for(i=0; i<SP_DMA_PAGE_NUM; i++){
		sp_tx_info.tx_block[i].tx_gdma_own = 0;
		sp_tx_info.tx_block[i].tx_addr = audio_comm_buf+i*audio_dma_page_sz;
		sp_tx_info.tx_block[i].tx_length = audio_dma_page_sz;
	}
}

static void sp_player_tx_complete(void *Data)
{
	SP_GDMA_STRUCT *gs = (SP_GDMA_STRUCT *) Data;
	PGDMA_InitTypeDef GDMA_InitStruct;
	u32 tx_addr;
	u32 tx_length;
	
	GDMA_InitStruct = &(gs->SpTxGdmaInitStruct);
	
	/* Clear Pending ISR */
	GDMA_ClearINT(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum);
	
	sp_release_tx_page();
	tx_addr = (u32)sp_get_ready_tx_page();
	tx_length = sp_get_ready_tx_length();
	GDMA_SetSrcAddr(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, tx_addr);
	GDMA_SetBlkSize(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, tx_length>>2);
	
	GDMA_Cmd(GDMA_InitStruct->GDMA_Index, GDMA_InitStruct->GDMA_ChNum, ENABLE);
}

/**
 * @brief  Execute some operations when audio starts to play
 * @param  None
 * @retval None
 */
void audio_player_start()
{
    duerapp_gpio_led_mode(DUER_LED_SLOW_TWINKLE);
}

/**
 * @brief  Execute some operations when audio stop playing
 * @param  None
 * @retval None
 * @Note   It is necessary to initialize audio as recorder after playing music
 */
void audio_player_stop(int reason)
{
    duerapp_gpio_led_mode(DUER_LED_SLOW_TWINKLE);
}

/**
 * @brief  Tx PCM data to codec
 * @param  buf: pointer to buffer which keep the pcm data
 * @param  len: length of pcm data to be transmitted
 * @retval None
 */
void audio_play_pcm(char *buf, int len)
{
	u8 *ptx_buf;
	int i = 0;
	int start = 0;
	uint16_t *tmp_buf = NULL;
	uint16_t *src_buf = NULL;
	int short_len = len >> 1;
	int dma_cnt = 0;
	int total_dma_cnt = (resample + 1) >> 1;
	int tmp_start = 0;

retry:
	if(resample < 3) {
		ptx_buf = sp_get_free_tx_page();
		if(ptx_buf){
			if(len < audio_dma_page_sz){
				//if valid audio data short than a page, make sure the rest of the page set to 0
				memset((void*)ptx_buf, 0x00, audio_dma_page_sz);

				if(audio_dma_page_sz >= 2*len){ // sr=11025&12000,L/R resample pcm
					tmp_buf = (void*)ptx_buf;
					src_buf = (void*)buf;
					if(resample == 1){  //stereo
						for(i = 0;i<short_len;i=i+2){
							start = 2*i;
							tmp_buf[start+2] = tmp_buf[start] = src_buf[i];
							tmp_buf[start+3] = tmp_buf[start+1] = src_buf[i+1];
						}
					}else if(resample == 2){ //mono
						for(i = 0;i<short_len;i=i+2){
							start = 2*i;
							tmp_buf[start+1] = tmp_buf[start] = src_buf[i];
							tmp_buf[start+3] = tmp_buf[start+2] = src_buf[i+1];
						}
					}
				}else{
					memcpy((void*)ptx_buf, (void*)buf, len); 	
				}
			}else{
				memcpy((void*)ptx_buf, (void*)buf, len);
			}
			sp_write_tx_page();
		}else{
			vTaskDelay(1);
			goto retry;
		}
	} else {
		while(dma_cnt < total_dma_cnt) {
			ptx_buf = sp_get_free_tx_page();
			if(ptx_buf){
				memset((void*)ptx_buf, 0x00, audio_dma_page_sz);
				tmp_buf = (void*)ptx_buf;
				tmp_start = dma_cnt*(short_len>>1);
				buf = buf + (tmp_start << 1);
				src_buf = (void*)buf;
				if(resample == 3){  //stereo
					for(i = 0;i<(short_len>>1);i=i+2){
						start = 4*i;
						tmp_buf[start+6] = tmp_buf[start+4] = tmp_buf[start+2] = tmp_buf[start] = src_buf[i];
						tmp_buf[start+7] = tmp_buf[start+5] = tmp_buf[start+3] = tmp_buf[start+1] = src_buf[i+1];
					}
				}else if(resample == 4){ //mono
					for(i = 0;i<(short_len>>1);i=i+2){
						start = 4*i;
						tmp_buf[start+3] = tmp_buf[start+2] = tmp_buf[start+1] = tmp_buf[start] = src_buf[i];
						tmp_buf[start+7] = tmp_buf[start+6] = tmp_buf[start+5] = tmp_buf[start+4] = src_buf[i+1];
					}
				}
				sp_write_tx_page();
				dma_cnt ++;
			}else{
				vTaskDelay(1);
			}
		}
	}
}



/**
 * @brief  Initializes the interface with codec to be able to tx data
 * @param  para: pointer to a player_t structure which stores initialization parameters
 * @retval can be 0 or -1:
 *          0:  Init success
 *          -1: Init fail
 */
int initialize_audio_as_player(player_t *para)
{
	int i = 0, smpl_rate_idx, ch_num_idx;
	int sample_rate = 0;
	u32 tx_addr;
	u32 tx_length;
	
	resample = 0;

	if(pDuerAppInfo->audio_has_inited){
		audio_deinit();
		pDuerAppInfo->audio_has_inited = 0;
	}

	DUER_LOGI("Initialize audio as player.");

	// Get sample rate parameter
	sample_rate = para->sample_rate;
	switch(para->sample_rate){
		case 8000:
			smpl_rate_idx = SR_8K;
			break;
		case 11025:
			resample = 3;
			smpl_rate_idx = SR_44P1K;
			sample_rate = 22050;
			DUER_LOGW("11.025K Mp3 not support, resample to 44.1K");
			break;
		case 16000:
			smpl_rate_idx = SR_16K;
			break;
		case 22050:
			resample = 1;
			smpl_rate_idx = SR_44P1K;
	//		sample_rate = 44100;
			DUER_LOGW("22.05K Mp3 not support, resample to 44.1K");
			break;
		case 24000:
			resample = 1;
			smpl_rate_idx = SR_48K;
		//	sample_rate = 48000;
			DUER_LOGW("24K Mp3 not support, resample to 48K");
			break;
		case 32000:
			smpl_rate_idx = SR_32K;
			break;
		case 44100:
			smpl_rate_idx = SR_44P1K;
			break;
		case 48000:
			smpl_rate_idx = SR_48K;
			break;
		default:
			DUER_LOGE("Dont support sample rate %d", para->sample_rate);
			return -1;
			break;
	}

	// Get channel number parameter
	switch(para->nb_channels){
		case 1:
			if(resample == 1)
				resample = 2;
			if(resample == 3)
				resample = 4;
			ch_num_idx = CH_MONO;
			break;
		case 2:
			ch_num_idx = CH_STEREO;
			break;
		default:
			DUER_LOGE("Dont support channel numbers %d", para->nb_channels);
			return -1;
			break;
	}

	// Get audio dma page size
	for(i=0; i<AUDIO_FREQ_NUM; i++)
		if(audio_freq_table[i] == sample_rate)
			break;

	audio_dma_page_sz = audio_dma_page_sz_table[para->nb_channels - 1][i];

	if(resample > 0)
		audio_dma_page_sz = audio_dma_page_sz*2;

	para->audio_bytes = audio_dma_page_sz;
	sp_obj.sample_rate = smpl_rate_idx;		
	sp_obj.mono_stereo = ch_num_idx;

	DUER_LOGI(">> audio set sr %d, cn %d, page_sz %d\n",smpl_rate_idx,ch_num_idx, audio_dma_page_sz);

	sp_obj.word_len = WL_16;
	sp_obj.direction = APP_LINE_OUT;
	
	audio_set_sp_clk(sp_obj.sample_rate);
	/*codec init*/
	CODEC_Init(sp_obj.sample_rate, sp_obj.word_len, sp_obj.mono_stereo, sp_obj.direction);

	sp_init_tx_variables();

	/*configure Sport according to the parameters*/
	AUDIO_SP_StructInit(&SP_InitStruct);
	SP_InitStruct.SP_MonoStereo= sp_obj.mono_stereo;
	SP_InitStruct.SP_WordLen = sp_obj.word_len;
	AUDIO_SP_Init(AUDIO_SPORT_DEV, &SP_InitStruct);

	AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, ENABLE);
	AUDIO_SP_TxStart(AUDIO_SPORT_DEV, ENABLE);

	//clear audio buf
	memset(audio_comm_buf,0x00,AUDIO_MAX_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM);
	pDuerAppInfo->audio_has_inited = 1;
	play_has_inited = 1;

	tx_addr = (u32)sp_get_ready_tx_page();
	tx_length = sp_get_ready_tx_length();
	AUDIO_SP_TXGDMA_Init(0, &SPGdmaStruct.SpTxGdmaInitStruct, &SPGdmaStruct, (IRQ_FUN)sp_player_tx_complete, tx_addr, tx_length);

	return 0;
}

void audio_play_deinit(void)
{
	/* sport disable */
	if(play_has_inited){
		DUER_LOGI("Audio deinit play.\n");
		AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, DISABLE);
		AUDIO_SP_TxStart(AUDIO_SPORT_DEV, DISABLE);
		/* Tx GDMA ch disable */
		GDMA_ClearINT(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum);
		GDMA_Cmd(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum, DISABLE);
		GDMA_ChnlFree(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum);
		play_has_inited = 0;
	}
}

