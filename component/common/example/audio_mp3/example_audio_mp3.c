#include "mp3/mp3_codec.h"
#include "example_audio_mp3.h"
#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "i2s_api.h"
#include "analogin_api.h"
#include <stdlib.h>
#include "platform_opts.h"
#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>
#include "section_config.h"

#if CONFIG_EXAMPLE_MP3_STREAM_SGTL5000
#include "sgtl5000.h"
#else
#include "alc5651.h"
#endif

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
#define I2S_DMA_PAGE_SIZE 4608   //Use frequency mapping table and set this value to number of decoded bytes 
                                 //Options:- 1152, 2304, 4608

#define NUM_CHANNELS CH_STEREO   //Use mp3 file properties to determine number of channels
                                 //Options:- CH_MONO, CH_STEREO

#define SAMPLING_FREQ SR_32KHZ   //Use mp3 file properties to identify frequency and use appropriate macro
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
                                


#define INPUT_FRAME_SIZE 1500

SDRAM_DATA_SECTION unsigned char MP3_Buf[INPUT_FRAME_SIZE]; 
SDRAM_DATA_SECTION signed short WAV_Buf[I2S_DMA_PAGE_SIZE];


#define I2S_DMA_PAGE_NUM    2   // Vaild number is 2~4

static u8 i2s_tx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
static u8 i2s_rx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];

#define I2S_SCLK_PIN            PC_1
#define I2S_WS_PIN              PC_0
#define I2S_SD_PIN              PC_2

static i2s_t i2s_obj;


static void i2s_tx_complete(void *data, char *pbuf)
{   
	//
}

static void i2s_rx_complete(void *data, char* pbuf)
{
	//
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

	//WavHeader pwavHeader;
	//u32 wav_length = 0;
	//u32 wav_offset = 0;
	int *ptx_buf;
        
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
	
	file_size = (&m_file)->fsize;
	bytes_left = file_size;
	printf("File size is %d\n",file_size);
	
	mp3 = mp3_create();
	if(!mp3)
	{
		printf("mp3 context create fail\n");
		goto exit;
	}
        tim1 = rtw_get_current_time();
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
		f_lseek(&m_file, (file_size-bytes_left));
                //printf("frame size [%d], Decoded bytes [%d]\n",frame_size,info.audio_bytes);
		
		retry:
		ptx_buf = i2s_get_tx_page(&i2s_obj);
		if(ptx_buf){
			if(info.audio_bytes < I2S_DMA_PAGE_SIZE){
				/* if valid audio data short than a page, make sure the rest of the page set to 0*/
				memset((void*)ptx_buf, 0x00, I2S_DMA_PAGE_SIZE);
				memcpy((void*)ptx_buf, (void*)WAV_Buf, info.audio_bytes);
			}else
				memcpy((void*)ptx_buf, (void*)WAV_Buf, info.audio_bytes);
			    i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
		}else{
			vTaskDelay(1);
			goto retry;
		}
		
		
		
	}while(read_length > 0);
        tim2 = rtw_get_current_time();
        printf("Decode time = %dms\n",(tim2-tim1));
	printf("PCM done\n");
	
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
	
}

void example_audio_mp3_thread(void* param){
	printf("Audio codec demo begin......\n");
	
	
	i2s_obj.channel_num = NUM_CHANNELS;
	i2s_obj.sampling_rate = SAMPLING_FREQ;
	i2s_obj.word_length = WL_16b;
	i2s_obj.direction = I2S_DIR_TXRX;	 
	i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_PIN);
	i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buf, (char*)i2s_rx_buf, \
		I2S_DMA_PAGE_NUM, I2S_DMA_PAGE_SIZE);
	i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_tx_complete, (uint32_t)&i2s_obj);
	i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_rx_complete, (uint32_t)&i2s_obj);
#if CONFIG_EXAMPLE_MP3_STREAM_SGTL5000
        sgtl5000_enable();
        sgtl5000_setVolume(0.5);
#else
	alc5651_init();
	alc5651_init_interface2();    
#endif
	
	char wav[16] = FILE_NAME;
	audio_play_sd_mp3(wav);
exit:
	vTaskDelete(NULL);
}

void example_audio_mp3(void)
{
	if(xTaskCreate(example_audio_mp3_thread, ((const char*)"example_audio_mp3_thread"), 2000, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
	printf("\n\r%s xTaskCreate(example_audio_mp3_thread) failed", __FUNCTION__);
}

