#include "example_audio_mp3.h"
#include "mp3/mp3_codec.h"
#include <platform/platform_stdlib.h>
#include "platform_opts.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>
#include "section_config.h"
#include "rl6548.h"

#if CONFIG_EXAMPLE_AUDIO_MP3

//-----------------Frequency Mapping Table--------------------//
/*+-------------+-------------------------+--------------------+
| Frequency(hz) | Number of Channels      | Decoded Bytes      |
                |(CH_MONO:1 CH_STEREO:2)  |(I2S_DMA_PAGE_SIZE) |
+---------------+-------------------------+--------------------+
|          8000 |                       1 |               1152 |
|          8000 |                       2 |               2304 |
|         16000 |                       1 |               1152 |
|         16000 |                       2 |               2304 |
|         22050 |                       1 |               1152 |
|         22050 |                       2 |               2304 |
|         24000 |                       1 |               1152 |
|         24000 |                       2 |               2304 |
|         32000 |                       1 |               2304 |
|         32000 |                       2 |               4608 |
|         44100 |                       1 |               2304 |
|         44100 |                       2 |               4608 |
|         48000 |                       1 |               2304 |
|         48000 |                       2 |               4608 |
+---------------+-------------------------+------------------+*/


//------------------------------------- ---CONFIG Parameters-----------------------------------------------//
//#define I2S_DMA_PAGE_SIZE 4608   //Use frequency mapping table and set this value to number of decoded bytes 
                                 //Options:- 1152, 2304, 4608

#define NUM_CHANNELS CH_STEREO   //Use mp3 file properties to determine number of channels
                                 //Options:- CH_MONO, CH_STEREO

#define SAMPLING_FREQ SR_44P1K   //Use mp3 file properties to identify frequency and use appropriate macro
                                 //Options:- SR_8KHZ     =>8000hz  - PASS       
                                 //          SR_16KHZ    =>16000hz - PASS
                                 //          SR_24KHZ    =>24000hz - PASS
                                 //          SR_32KHZ    =>32000hz - PASS
                                 //          SR_48KHZ    =>48000hz - PASS
                                 //          SR_96KHZ    =>96000hz ~ NOT SUPPORTED
                                 //          SR_7p35KHZ  =>7350hz  ~ NOT SUPPORTED
                                 //          SR_14p7KHZ  =>14700hz ~ NOT SUPPORTED
                                 //          SR_22p05KHZ =>22050hz - PASS
                                 //          SR_29p4KHZ  =>29400hz ~ NOT SUPPORTED
                                 //          SR_44p1KHZ  =>44100hz - PASS
                                 //          SR_88p2KHZ  =>88200hz ~ NOT SUPPORTED

#define FILE_NAME "AudioSDTest.mp3"    //Specify the file name you wish to play that is present in the SDCARD

//------------------------------------- ---CONFIG Parameters-----------------------------------------------//
                                
#ifdef __GNUC__
#undef SDRAM_DATA_SECTION
#define SDRAM_DATA_SECTION 
#endif

#define INPUT_FRAME_SIZE 1500

SDRAM_DATA_SECTION unsigned char MP3_Buf[INPUT_FRAME_SIZE]; 
SDRAM_DATA_SECTION signed short WAV_Buf[MP3_DECODE_SIZE*2];

static SP_InitTypeDef SP_InitStruct;
static SP_GDMA_STRUCT SPGdmaStruct;
static SP_OBJ sp_obj;
static SP_TX_INFO sp_tx_info;

static u8 sp_tx_buf[SP_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
static u8 sp_zero_buf[SP_ZERO_BUF_SIZE];

static int sample_rate_table[] = {44100, 48000, 32000};
static int bit_rate_table[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};

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
	ptx_block-> tx_gdma_own = 1;
	ptx_block -> tx_length = length;
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

void sp_rx_complete(void *data, char* pbuf)
{
	//
}

static void sp_init_hal(pSP_OBJ psp_obj)
{
	u32 div;
	
	PLL_I2S_Set(ENABLE);		//enable 98.304MHz PLL. needed if fs=8k/16k/32k/48k/96k
	PLL_PCM_Set(ENABLE);		//enable 45.1584MHz PLL. needed if fs=44.1k/8.2k
	PLL_Sel(PLL_I2S);
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
		case SR_44P1K:
           	 	div = 4;
            		PLL_Sel(PLL_PCM);
            		break;	
		default:
			DBG_8195A("sample rate not supported!!\n");
			break;
	}
	PLL_Div(div);

	/*codec init*/
	CODEC_Init(psp_obj->sample_rate, psp_obj->word_len, psp_obj->mono_stereo, psp_obj->direction);
	CODEC_SetVolume(80, 80);
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



void audio_play_sd_mp3(u8* filename){
	mp3_decoder_t mp3;
	mp3_info_t info;
	int drv_num = 0;
	int frame_size = 0;
	u32 read_length = 0;
	FRESULT res; 
	FATFS 	m_fs;
	FIL		m_file;  
	char	logical_drv[4]; //root diretor
	char abs_path[32]; //Path to input file
	DWORD bytes_left;
	DWORD file_size;
	volatile u32 tim1 = 0;
	volatile u32 tim2 = 0;
	int *ptx_buf;

	int sync_word[4] = {0, 0, 0, 0};
	int suspect_area[10] = {0}; 
	int count = 0;
	int bit_rate = 0;
	int sample_rate = 0;
	int frame_size_cal = 0;
	int new_offset = 0;
	int head_count = 0;
	int flag = 0;


	drv_num = FATFS_RegisterDiskDriver(&SD_disk_Driver);
	if(drv_num < 0){
		printf("Rigester disk driver to FATFS fail.\n");
		return;
	}else{
		logical_drv[0] = drv_num + '0';
		logical_drv[1] = ':';
		logical_drv[2] = '/';
		logical_drv[3] = 0;
	}

	if(f_mount(&m_fs, logical_drv, 1)!= FR_OK){
		printf("FATFS mount logical drive fail, please format DISK to FAT16/32.\n");
		goto unreg;
	}

	memset(abs_path, 0x00, sizeof(abs_path));
	strcpy(abs_path, logical_drv);
	sprintf(&abs_path[strlen(abs_path)],"%s", filename);

	//Open source file
	res = f_open(&m_file, abs_path, FA_OPEN_EXISTING | FA_READ); // open read only file
	
	if(res != FR_OK){
		printf("Open source file %s fail.\n", filename);
		goto umount;
	}
	//the addresses of sync word of MP3 files with ID3v2 headers may be larger than INPUT_FRAME_SIZE
	//find the first sync word, and move to this address to play audio files.
	res = f_read(&m_file, MP3_Buf, INPUT_FRAME_SIZE,(UINT*)&read_length);	
	while(read_length == INPUT_FRAME_SIZE){
		for(int i = 0; i < INPUT_FRAME_SIZE - 3; i++){
			sync_word[0] = MP3_Buf[i];
			sync_word[1] = MP3_Buf[i + 1];
			sync_word[2] = MP3_Buf[i + 2];
			sync_word[3] = MP3_Buf[i + 3];
			count = count + 1;
			//for frame with CRC parity, sync_word[1] == 0xfa is ok
			if(sync_word[0] == 0xff && ((sync_word[1] == 0xfb) || (sync_word[1] == 0xfa))){
				if(((sync_word[2] & 0xf0 )== 0xf0) ||((sync_word[2] & 0x0c) == 0x0c)){
					continue ;
				}else{
					bit_rate = bit_rate_table[(sync_word[2]&0xf0)>>4];
					sample_rate = sample_rate_table[(sync_word[2] & 0x0c)>>2];
					//not sure if plus 1,should be parity value
					frame_size_cal = 144000 * bit_rate/sample_rate + 1;
					new_offset = count - 4 + frame_size_cal;
					f_lseek(&m_file, new_offset);
					res = f_read(&m_file, MP3_Buf, 10,(UINT*)&read_length);

					for(int i = 0; i < 10; i++){
						suspect_area[i] = MP3_Buf[i];
					}
					for(int i = 0; i < 9; i ++){
						if(suspect_area[i] == 0xff && (suspect_area[i + 1] == 0xfb||suspect_area[i + 1] == 0xfa)){
							head_count = count - 1;
							flag = 1;
						}
					}
					if(flag == 1){
						break;
					}else{
						continue;
					}
				}
			}
		}
		f_lseek(&m_file, count);
		res = f_read(&m_file, MP3_Buf, INPUT_FRAME_SIZE,(UINT*)&read_length);	
		if(flag ==1){
			break;
		}	
	}



	file_size = (&m_file)->fsize;
	bytes_left = file_size - head_count;
	printf("File size is %d\n",file_size);
	
	mp3 = mp3_create();
	if(!mp3)
	{
		printf("mp3 context create fail\n");
		goto exit;
	}
        tim1 = rtw_get_current_time();
	f_lseek(&m_file, head_count);
	
	do{
		/* Read a block */
		if(bytes_left >= INPUT_FRAME_SIZE)
		res = f_read(&m_file, MP3_Buf, INPUT_FRAME_SIZE,(UINT*)&read_length);	
		else if(bytes_left > 0)
		res = f_read(&m_file, MP3_Buf, bytes_left,(UINT*)&read_length);	
		if((res != FR_OK))
		{
			printf("Wav play done !\n");
			break;
		}
		
		frame_size = mp3_decode(mp3, MP3_Buf, INPUT_FRAME_SIZE, WAV_Buf, &info);
		bytes_left = bytes_left - frame_size;
		f_lseek(&m_file, (file_size-bytes_left ));
		
		retry:
		ptx_buf = sp_get_free_tx_page();
		if(ptx_buf){
			if(info.audio_bytes < MP3_DECODE_SIZE){
				/* if valid audio data short than a page, make sure the rest of the page set to 0*/
				memset(((void*)WAV_Buf)+info.audio_bytes, 0x00, MP3_DECODE_SIZE-info.audio_bytes);
			}else{

			}
		    	sp_write_tx_page((void*)WAV_Buf, SP_DMA_PAGE_SIZE);
			
			retry2:
			if (sp_get_free_tx_page()){
				sp_write_tx_page(((void*)WAV_Buf)+SP_DMA_PAGE_SIZE, SP_DMA_PAGE_SIZE);
			}
			else{
				vTaskDelay(1);
				goto retry2;
			}
				
		}else{
			vTaskDelay(1);
			goto retry;
		}
	
	}while(read_length > 0);

	
exit:	
	// close source file
	res = f_close(&m_file);
	if(res){
		printf("close file (%s) fail.\n", filename);
	}

umount:
	if(f_mount(NULL, logical_drv, 1) != FR_OK){
		printf("FATFS unmount logical drive fail.\n");
	}

unreg:	
	if(FATFS_UnRegisterDiskDriver(drv_num))	
	printf("Unregister disk driver from FATFS fail.\n");
	
	mp3_done(mp3);
	AUDIO_SP_TdmaCmd(AUDIO_SPORT_DEV, DISABLE);
	AUDIO_SP_TxStart(AUDIO_SPORT_DEV, DISABLE);	
	GDMA_ClearINT(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum);
	GDMA_Cmd(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum, DISABLE);
	GDMA_ChnlFree(SPGdmaStruct.SpTxGdmaInitStruct.GDMA_Index, SPGdmaStruct.SpTxGdmaInitStruct.GDMA_ChNum);
	
}

void example_audio_mp3_thread(void* param)
{
	u32 tx_addr;
	u32 tx_length;
	pSP_OBJ psp_obj = (pSP_OBJ)param;

	
	printf("Audio codec mp3 demo begin......\n");
	
	sp_init_hal(psp_obj);
	
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
	
	char wav[16] = FILE_NAME;
	audio_play_sd_mp3(wav);
		
exit:
	vTaskDelete(NULL);
}

void example_audio_mp3(void)
{
	
	sp_obj.mono_stereo = NUM_CHANNELS;
	sp_obj.sample_rate = SAMPLING_FREQ;
	sp_obj.word_len = WL_16;
	sp_obj.direction = APP_LINE_OUT;
	
	if(xTaskCreate(example_audio_mp3_thread, ((const char*)"example_audio_mp3_thread"), 2000, (void *)(&sp_obj), tskIDLE_PRIORITY + 1, NULL) != pdPASS)
	printf("\n\r%s xTaskCreate(example_audio_mp3_thread) failed", __FUNCTION__);
}

void vApplicationMallocFailedHook(void)
{

	
}

#endif
