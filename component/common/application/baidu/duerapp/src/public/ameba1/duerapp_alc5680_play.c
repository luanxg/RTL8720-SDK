#include "duerapp.h"
#include "duerapp_alc5680_audio.h"
#include "duerapp_media.h"

#include "lightduer_ds_log_audio.h" 

static int resample = 0; //0,not need resample; 1,stereo; 2,mono
static unsigned int i2s_dma_page_sz;

SDRAM_DATA_SECTION static unsigned int i2s_freq_table[I2S_FREQ_NUM]={
    8000, 16000, 22050, 24000, 32000, 44100, 48000,
};
SDRAM_DATA_SECTION static unsigned int i2s_dma_page_sz_table[2][I2S_FREQ_NUM]={
    {1152, 1152,1152,1152,2304,2304,2304},
    {2304, 2304,2304,2304,4608,4608,4608},
};

static void i2s_player_tx_complete(void *data, char *pbuf)
{
    //
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
    int *ptx_buf;
	int i = 0;
	int start = 0;
	uint16_t *tmp_buf = NULL;
	uint16_t *src_buf = NULL;
	int short_len = len >> 1;

retry:
    ptx_buf = i2s_get_tx_page(&i2s_obj);
    if(ptx_buf){
        if(len < i2s_dma_page_sz){
            //if valid audio data short than a page, make sure the rest of the page set to 0
			memset((void*)ptx_buf, 0x00, i2s_dma_page_sz);
			
		 	if(i2s_dma_page_sz >= 2*len){ // sr=11025&12000,L/R resample pcm

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
						tmp_buf[start+3] = tmp_buf[start+2] = buf[i+1];
					}
				}
			}else{
				memcpy((void*)ptx_buf, (void*)buf, len); 	
			}
        }else{
            memcpy((void*)ptx_buf, (void*)buf, len);
        }

        i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
    }else{
        vTaskDelay(1);
        goto retry;
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
		resample = 0;
	
		if(pDuerAppInfo->i2s_has_inited){
			i2s_deinit(&i2s_obj);
			pDuerAppInfo->i2s_has_inited = 0;
		}
	
		DUER_LOGI("Initialize audio as player.");
	
		// Get sample rate parameter
		sample_rate = para->sample_rate;
		switch(para->sample_rate){
			
			case 8000:
				smpl_rate_idx = SR_8KHZ;
				sample_rate = 16000;
				break;
			case 11025:
				resample = 1;
				smpl_rate_idx = SR_22p05KHZ;
				sample_rate = 22050;
				DUER_LOGW("11.025K Mp3 not support, resample to 22.05K");
				break;
			case 12000:
				resample = 1;
				smpl_rate_idx = SR_24KHZ;
				sample_rate = 24000;
				DUER_LOGW("12K Mp3 not support, resample to 24K");
				break;
			case 16000:
				smpl_rate_idx = SR_16KHZ;
				break;
			case 22050:
				smpl_rate_idx = SR_22p05KHZ;
				break;
			case 24000:
				smpl_rate_idx = SR_24KHZ;
				break;
			case 32000:
				smpl_rate_idx = SR_32KHZ;
				break;
			case 44100:
				smpl_rate_idx = SR_44p1KHZ;
				break;
			case 48000:
				smpl_rate_idx = SR_48KHZ;
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
				ch_num_idx = I2S_CH_MONO;
				break;
			case 2:
				ch_num_idx = I2S_CH_STEREO;
				break;
			default:
				DUER_LOGE("Dont support channel numbers %d", para->nb_channels);
				return -1;
				break;
		}
	
		// Get i2s dma page size
		for(i=0; i<I2S_FREQ_NUM; i++)
			if(i2s_freq_table[i] == sample_rate)
				break;
	
		i2s_dma_page_sz = i2s_dma_page_sz_table[para->nb_channels - 1][i];
	
		if(resample > 0)
			i2s_dma_page_sz = i2s_dma_page_sz*2;


		para->audio_bytes = i2s_dma_page_sz;
		i2s_obj.channel_num = ch_num_idx;
		i2s_obj.sampling_rate = smpl_rate_idx;
	
		DUER_LOGI(">> i2s set sr %d, cn %d, page_sz %d\n",smpl_rate_idx,ch_num_idx,i2s_dma_page_sz);
	
		i2s_obj.word_length = WL_16b;
		i2s_obj.direction = I2S_DIR_TX;
		i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_PIN);
		i2s_set_dma_buffer(&i2s_obj, (char*)i2s_comm_buf, NULL, \
				I2S_DMA_PAGE_NUM, i2s_dma_page_sz);
		i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_player_tx_complete, (uint32_t)&i2s_obj);
	
		//clear i2s buf
		memset(i2s_comm_buf,0x00,AUDIO_MAX_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM);
		pDuerAppInfo->i2s_has_inited = 1;
	
		return 0;
	}





