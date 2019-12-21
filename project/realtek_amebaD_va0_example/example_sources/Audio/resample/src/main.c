#include "main.h"
#include <platform/platform_stdlib.h>
#include "osdep_service.h"
#include "rl6548.h"

#include "sine_48.00k_1ch_16bit.c"
PSRAM_BSS_SECTION short voice_sample[480000];
RESAMPLE_STAT state;

static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_TX_INFO sp_tx_info;

static u8 sp_tx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];

// allpass filter coefficients.
static const short kResampleAllpass[2][3] = {
	{821, 6110, 12382},
	{3050, 9368, 15063}
};

// interpolation coefficients
static const short kCoefficients48To32[2][8] = {
	{778, -2050, 1087, 23285, 12903, -3783, 441, 222},
	{222, 441, -3783, 12903, 23285, 1087, -2050, 778}
};

/**
  * @brief  Resample 48K to 32K, resampling ratio: 2/3.
  * @param  In: input, int (normalized, not saturated) :: size 3 * K
  * @param  Out: output, int (shifted 15 positions to the left, + offset 16384) :: size 2 * K
  * @param  K: number of blocks
  * @retval None
  */
void Resample48khzTo32khz(const int *In, int *Out, int K)
{
	/////////////////////////////////////////////////////////////
	// Filter operation:
	//
	// Perform resampling (3 input samples -> 2 output samples);
	// process in sub blocks of size 3 samples.
	int tmp;
	int m;

	for (m = 0; m < K; m++)
	{
		tmp = 1 << 14;
		tmp += kCoefficients48To32[0][0] * In[0];
		tmp += kCoefficients48To32[0][1] * In[1];
		tmp += kCoefficients48To32[0][2] * In[2];
		tmp += kCoefficients48To32[0][3] * In[3];
		tmp += kCoefficients48To32[0][4] * In[4];
		tmp += kCoefficients48To32[0][5] * In[5];
		tmp += kCoefficients48To32[0][6] * In[6];
		tmp += kCoefficients48To32[0][7] * In[7];
		Out[0] = tmp;

		tmp = 1 << 14;
		tmp += kCoefficients48To32[1][0] * In[1];
		tmp += kCoefficients48To32[1][1] * In[2];
		tmp += kCoefficients48To32[1][2] * In[3];
		tmp += kCoefficients48To32[1][3] * In[4];
		tmp += kCoefficients48To32[1][4] * In[5];
		tmp += kCoefficients48To32[1][5] * In[6];
		tmp += kCoefficients48To32[1][6] * In[7];
		tmp += kCoefficients48To32[1][7] * In[8];
		Out[1] = tmp;

		// update pointers
		In += 3;
		Out += 2;
	}
}

/**
  * @brief  Twice decimator.
  * @param  in: input, int (shifted 15 positions to the left, + offset 16384) OVERWRITTEN!
  * @param  len: input length
  * @param  out: output, short (saturated) (of length len/2)
  * @param  state: filter state array; length = 8
  * @retval None
  */
void ResampleDownBy2IntToShort(int *in, int len, short *out, int *state)
{
	int tmp0, tmp1, diff;
	int i;

	len >>= 1;

	// lower allpass filter (operates on even input samples)
	for (i = 0; i < len; i++)
	{
		tmp0 = in[i << 1];
		diff = tmp0 - state[1];
		// UBSan: -1771017321 - 999586185 cannot be represented in type 'int'

		// scale down and round
		diff = (diff + (1 << 13)) >> 14;
		tmp1 = state[0] + diff * kResampleAllpass[1][0];
		state[0] = tmp0;
		diff = tmp1 - state[2];
		// scale down and truncate
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		tmp0 = state[1] + diff * kResampleAllpass[1][1];
		state[1] = tmp1;
		diff = tmp0 - state[3];
		// scale down and truncate
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		state[3] = state[2] + diff * kResampleAllpass[1][2];
		state[2] = tmp0;

		// divide by two and store temporarily
		in[i << 1] = (state[3] >> 1);
	}

	in++;

	// upper allpass filter (operates on odd input samples)
	for (i = 0; i < len; i++)
	{
		tmp0 = in[i << 1];
		diff = tmp0 - state[5];
		// scale down and round
		diff = (diff + (1 << 13)) >> 14;
		tmp1 = state[4] + diff * kResampleAllpass[0][0];
		state[4] = tmp0;
		diff = tmp1 - state[6];
		// scale down and round
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		tmp0 = state[5] + diff * kResampleAllpass[0][1];
		state[5] = tmp1;
		diff = tmp0 - state[7];
		// scale down and truncate
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		state[7] = state[6] + diff * kResampleAllpass[0][2];
		state[6] = tmp0;

		// divide by two and store temporarily
		in[i << 1] = (state[7] >> 1);
	}

	in--;

	// combine allpass outputs
	for (i = 0; i < len; i += 2)
	{
		// divide by two, add both allpass outputs and round
		tmp0 = (in[i << 1] + in[(i << 1) + 1]) >> 15;
		tmp1 = (in[(i << 1) + 2] + in[(i << 1) + 3]) >> 15;
		if (tmp0 > (int)0x00007FFF)
		tmp0 = 0x00007FFF;
		if (tmp0 < (int)0xFFFF8000)
		tmp0 = 0xFFFF8000;
		out[i] = (short)tmp0;
		if (tmp1 > (int)0x00007FFF)
		tmp1 = 0x00007FFF;
		if (tmp1 < (int)0xFFFF8000)
		tmp1 = 0xFFFF8000;
		out[i + 1] = (short)tmp1;
	}
}

/**
  * @brief  Lowpass filter.
  * @param  in: input, short
  * @param  len: input length
  * @param  out: output, int (normalized, not saturated)
  * @param  state: filter state array; length = 16
  * @retval None
  */
void ResampleLPBy2ShortToInt(const short* in, int len, int* out, int* state)
{
	int tmp0, tmp1, diff;
	int i;

	len >>= 1;

	// lower allpass filter: odd input -> even output samples
	in++;
	// initial state of polyphase delay element
	tmp0 = state[12];
	for (i = 0; i < len; i++)
	{
		diff = tmp0 - state[1];
		// scale down and round
		diff = (diff + (1 << 13)) >> 14;
		tmp1 = state[0] + diff * kResampleAllpass[1][0];
		state[0] = tmp0;
		diff = tmp1 - state[2];
		// scale down and truncate
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		tmp0 = state[1] + diff * kResampleAllpass[1][1];
		state[1] = tmp1;
		diff = tmp0 - state[3];
		// scale down and truncate
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		state[3] = state[2] + diff * kResampleAllpass[1][2];
		state[2] = tmp0;

		// scale down, round and store
		out[i << 1] = state[3] >> 1;
		tmp0 = ((int)in[i << 1] << 15) + (1 << 14);
	}
	in--;

	// upper allpass filter: even input -> even output samples
	for (i = 0; i < len; i++)
	{
		tmp0 = ((int)in[i << 1] << 15) + (1 << 14);
		diff = tmp0 - state[5];
		// scale down and round
		diff = (diff + (1 << 13)) >> 14;
		tmp1 = state[4] + diff * kResampleAllpass[0][0];
		state[4] = tmp0;
		diff = tmp1 - state[6];
		// scale down and round
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		tmp0 = state[5] + diff * kResampleAllpass[0][1];
		state[5] = tmp1;
		diff = tmp0 - state[7];
		// scale down and truncate
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		state[7] = state[6] + diff * kResampleAllpass[0][2];
		state[6] = tmp0;

		// average the two allpass outputs, scale down and store
		out[i << 1] = (out[i << 1] + (state[7] >> 1)) >> 15;
	}

	// switch to odd output samples
	out++;

	// lower allpass filter: even input -> odd output samples
	for (i = 0; i < len; i++)
	{
		tmp0 = ((int)in[i << 1] << 15) + (1 << 14);
		diff = tmp0 - state[9];
		// scale down and round
		diff = (diff + (1 << 13)) >> 14;
		tmp1 = state[8] + diff * kResampleAllpass[1][0];
		state[8] = tmp0;
		diff = tmp1 - state[10];
		// scale down and truncate
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		tmp0 = state[9] + diff * kResampleAllpass[1][1];
		state[9] = tmp1;
		diff = tmp0 - state[11];
		// scale down and truncate
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		state[11] = state[10] + diff * kResampleAllpass[1][2];
		state[10] = tmp0;

		// scale down, round and store
		out[i << 1] = state[11] >> 1;
	}

	// upper allpass filter: odd input -> odd output samples
	in++;
	for (i = 0; i < len; i++)
	{
		tmp0 = ((int)in[i << 1] << 15) + (1 << 14);
		diff = tmp0 - state[13];
		// scale down and round
		diff = (diff + (1 << 13)) >> 14;
		tmp1 = state[12] + diff * kResampleAllpass[0][0];
		state[12] = tmp0;
		diff = tmp1 - state[14];
		// scale down and round
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		tmp0 = state[13] + diff * kResampleAllpass[0][1];
		state[13] = tmp1;
		diff = tmp0 - state[15];
		// scale down and truncate
		diff = diff >> 14;
		if (diff < 0)
			diff += 1;
		state[15] = state[14] + diff * kResampleAllpass[0][2];
		state[14] = tmp0;

		// average the two allpass outputs, scale down and store
		out[i << 1] = (out[i << 1] + (state[15] >> 1)) >> 15;
	}
}

// initialize state of 48 -> 16 resampler
void ResampleReset48khzTo16khz(RESAMPLE_STAT* state)
{
	memset(state->S_48_48, 0, 16 * sizeof(int));
	memset(state->S_48_32, 0, 8 * sizeof(int));
	memset(state->S_32_16, 0, 8 * sizeof(int));
}

/**
  * @brief  48K -> 16K resampler.
  * @param  in: input, short  in[len]
  * @param  len: input length
  * @param  out: output, short out[len/3]
  * @retval None
  */
void Resample48khzTo16khz(const short* in, int len, short* out)
{
	int *temp = NULL;
	temp = (int *) malloc ((len+16)*sizeof(int));
        if (temp == NULL) {
		printf("ERROR: malloc\r\n");
		return;
        }
	///// 48 --> 48(LP)   short --> int/////
	ResampleLPBy2ShortToInt(in, len, temp + 16, state.S_48_48);
	///// 48 --> 32  int --> int /////
	memcpy(temp + 8, state.S_48_32, 8 * sizeof(int));
	memcpy(state.S_48_32, temp + 8+len, 8 * sizeof(int));
	Resample48khzTo32khz(temp + 8, temp, len/3);
	///// 32 --> 16  int --> short /////
	ResampleDownBy2IntToShort(temp, len/3*2, out, state.S_32_16);
	free(temp);
}
u8 *sp_get_free_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);
	
	if (ptx_block->tx_gdma_own)
		return NULL;
	else{
		return ptx_block->tx_addr;
	}
}

void sp_write_tx_page(u8 *src, u32 length)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_usr_cnt]);
	
	memcpy(ptx_block->tx_addr, src, length);
	ptx_block->tx_gdma_own = 1;
	sp_tx_info.tx_usr_cnt++;
	if (sp_tx_info.tx_usr_cnt == SP_DMA_PAGE_NUM){
		sp_tx_info.tx_usr_cnt = 0;
	}
}

void sp_release_tx_page(void)
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

u8 *sp_get_ready_tx_page(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);
	
	if (ptx_block->tx_gdma_own){
		sp_tx_info.tx_empty_flag = 0;
		return ptx_block->tx_addr;
	}
	else{
		sp_tx_info.tx_empty_flag = 1;
		return sp_tx_info.tx_zero_block.tx_addr;	//for audio buffer empty case
	}
}

u32 sp_get_ready_tx_length(void)
{
	pTX_BLOCK ptx_block = &(sp_tx_info.tx_block[sp_tx_info.tx_gdma_cnt]);

	if (sp_tx_info.tx_empty_flag){
		return sp_tx_info.tx_zero_block.tx_length;
	}
	else{
		return ptx_block->tx_length;
	}
}

void sp_tx_complete(void *Data)
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

static void sp_init_hal(pSP_OBJ psp_obj)
{
	u32 div;
	GPIO_InitTypeDef GPIO_InitStruct_temp;
	
	PLL_I2S_Set(ENABLE);		//enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
	//PLL_PCM_Set(ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/8.2k

	RCC_PeriphClockCmd(APBPeriph_AUDIOC, APBPeriph_AUDIOC_CLOCK, ENABLE);
	RCC_PeriphClockCmd(APBPeriph_SPORT, APBPeriph_SPORT_CLOCK, ENABLE);	

	//set clock divider to gen clock sample_rate*256 from 98.304M.
	switch(psp_obj->sample_rate){
		case SR_48K:
			div = 8;
			break;
		case SR_96K:
			div = 4;
			break;
		case SR_32K:
			div = 12;
			break;
		case SR_16K:
			div = 24;
			break;
		case SR_8K:
			div = 48;
			break;
		default:
			DBG_8195A("sample rate not supported!!\n");
			break;
	}
	PLL_Div(div);

	/*codec init*/
	CODEC_Init(psp_obj->sample_rate, psp_obj->word_len, psp_obj->mono_stereo, psp_obj->direction);
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
		sp_tx_info.tx_block[i].tx_addr = sp_tx_buf+i*SP_DMA_PAGE_SIZE;
		sp_tx_info.tx_block[i].tx_length = SP_DMA_PAGE_SIZE;
	}
}

void app_init_psram(void)
{
	u32 temp;
	PCTL_InitTypeDef  PCTL_InitStruct;

	/*set rwds pull down*/
	temp = HAL_READ32(PINMUX_REG_BASE, 0x104);
	temp &= ~(PAD_BIT_PULL_UP_RESISTOR_EN | PAD_BIT_PULL_DOWN_RESISTOR_EN);
	temp |= PAD_BIT_PULL_DOWN_RESISTOR_EN;
	HAL_WRITE32(PINMUX_REG_BASE, 0x104, temp);

	PSRAM_CTRL_StructInit(&PCTL_InitStruct);
	PSRAM_CTRL_Init(&PCTL_InitStruct);

	PSRAM_PHY_REG_Write(REG_PSRAM_CAL_PARA, 0x02030316);

	/*check psram valid*/
	HAL_WRITE32(PSRAM_BASE, 0, 0);
	assert_param(0 == HAL_READ32(PSRAM_BASE, 0));

	if(SYSCFG_CUT_VERSION_A != SYSCFG_CUTVersion()) {
		if(_FALSE == PSRAM_calibration())
			return;

		if(FALSE == psram_dev_config.psram_dev_cal_enable) {
			temp = PSRAM_PHY_REG_Read(REG_PSRAM_CAL_CTRL);
			temp &= (~BIT_PSRAM_CFG_CAL_EN);
			PSRAM_PHY_REG_Write(REG_PSRAM_CAL_CTRL, temp);
		}
	}

	/*init psram bss area*/
	memset(__psram_bss_start__, 0, __psram_bss_end__ - __psram_bss_start__);
}


void example_audio_resample_thread(void* param)
{
	int i = 0;
	u32 tx_addr;
	u32 tx_length;
	pSP_OBJ psp_obj = (pSP_OBJ)param;
	u32 resample_size;
	u32 sample_size = sizeof(pcm_data)>>1;
	short voice_resample[SP_DMA_PAGE_SIZE>>1];

	app_init_psram();
	
	DBG_8195A("Audio resample demo begin......\n");
	memcpy((char*)voice_sample, (char*)pcm_data, sizeof(pcm_data));
	resample_size = sample_size/3;
	DBG_8195A("sample size: %d, resample size: %d......\n", sample_size, resample_size);
	ResampleReset48khzTo16khz(&state);

	sp_init_hal(psp_obj);
	CODEC_SetVolume(0x90, 0x90);
	
	sp_init_tx_variables();

	/*configure Sport according to the parameters*/
	AUDIO_SP_StructInit(&SP_InitStruct);
	SP_InitStruct.SP_MonoStereo= psp_obj->mono_stereo;
	SP_InitStruct.SP_WordLen = psp_obj->word_len;

	AUDIO_SP_Init(AUDIO_SPORT_DEV, &SP_InitStruct);
	
	AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, ENABLE);
	AUDIO_SP_TxStart(AUDIO_SPORT_DEV, ENABLE);
	
	tx_addr = (u32)sp_get_ready_tx_page();
	tx_length = sp_get_ready_tx_length();
	AUDIO_SP_TXGDMA_Init(0, &SPGdmaStruct.SpTxGdmaInitStruct, &SPGdmaStruct, (IRQ_FUN)sp_tx_complete, tx_addr, tx_length);

	while(1){	
		if(sp_get_free_tx_page()){
			memset(voice_resample, 0, SP_DMA_PAGE_SIZE);
			Resample48khzTo16khz(voice_sample+i*(SP_DMA_PAGE_SIZE*3/2), SP_DMA_PAGE_SIZE*3/2, voice_resample);
			sp_write_tx_page((u8 *)voice_resample, SP_DMA_PAGE_SIZE);
			i++;	
			if ((i+1)*SP_DMA_PAGE_SIZE > resample_size*2){
				i = 0;
				//ResampleReset48khzTo16khz(&state);
			}

		}
		else{
			vTaskDelay(1);
		}		
	}
exit:
	vTaskDelete(NULL);
}

void main(void)
{
	sp_obj.sample_rate = SR_16K;
	sp_obj.word_len = WL_16;
	sp_obj.mono_stereo = CH_MONO;
	sp_obj.direction = APP_LINE_OUT;
	if(xTaskCreate(example_audio_resample_thread, ((const char*)"example_audio_resample_thread"), 1024, (void *)(&sp_obj), tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_audio_resample_thread) failed", __FUNCTION__);

	vTaskStartScheduler();
	while(1){
		vTaskDelay( 1000 / portTICK_RATE_MS );
	}
}

