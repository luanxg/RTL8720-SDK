#include "example_audio_m4a_mp3.h"
#include "mp3/mp3_codec.h"
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
#include "libAACdec/include/aacdecoder_lib.h"
#include <libavformat/avformat.h>
#include <libavformat/url.h>

#define CONFIG_EXAMPLE_M4A_STREAM_SGTL5000      1
#define CONFIG_EXAMPLE_M4A_STREAM_ALC5651       0
#define CONFIG_EXAMPLE_M4A_STREAM_ALC5680       0

#if CONFIG_EXAMPLE_M4A_STREAM_SGTL5000
#include "sgtl5000.h"
#endif

#if CONFIG_EXAMPLE_M4A_STREAM_ALC5651
#include "alc5651.h"
#endif

#if CONFIG_EXAMPLE_M4A_STREAM_ALC5680
#include "alc5680.h"
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

#define INPUT_FRAME_SIZE_C 1540

//------------------------------------- ---CONFIG Parameters M4A-----------------------------------------------//
#define I2S_DMA_PAGE_SIZE_M4A_C 4096   //Use frequency mapping table and set this value to number of decoded bytes 
//Options:- 1152, 2304, 4608

#define NUM_CHANNELS_M4A_C CH_STEREO       //Use m4a file properties to determine number of channels
//Options:- CH_MONO, CH_STEREO

#define SAMPLING_FREQ_M4A_C SR_44p1KHZ     //Use m4a file properties to identify frequency and use appropriate macro
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

#define FILE_NAME_M4A_C "sound.m4a"    //Specify the file name you wish to play that is present in the SDCARD

//------------------------------------- ---CONFIG Parameters M4A-----------------------------------------------//


//------------------------------------- ---CONFIG Parameters MP3-----------------------------------------------//
#define I2S_DMA_PAGE_SIZE_MP3_C 4608   //Use frequency mapping table and set this value to number of decoded bytes 
//Options:- 1152, 2304, 4608

#define NUM_CHANNELS_MP3_C CH_STEREO       //Use m4a file properties to determine number of channels
//Options:- CH_MONO, CH_STEREO

#define SAMPLING_FREQ_MP3_C SR_44p1KHZ     //Use m4a file properties to identify frequency and use appropriate macro
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

#define FILE_NAME_MP3_C "sound.mp3"    //Specify the file name you wish to play that is present in the SDCARD

//------------------------------------- ---CONFIG Parameters MP3-----------------------------------------------//

SDRAM_DATA_SECTION unsigned char MP3_Buf_C[INPUT_FRAME_SIZE_C]; 
SDRAM_DATA_SECTION signed short WAV_Buf_C[I2S_DMA_PAGE_SIZE_MP3_C];


#define I2S_DMA_PAGE_NUM    2   // Vaild number is 2~4

SDRAM_DATA_SECTION static u8 i2s_tx_buf_m4a[I2S_DMA_PAGE_SIZE_M4A_C*I2S_DMA_PAGE_NUM];
SDRAM_DATA_SECTION static u8 i2s_rx_buf_m4a[I2S_DMA_PAGE_SIZE_M4A_C*I2S_DMA_PAGE_NUM];

SDRAM_DATA_SECTION static u8 i2s_tx_buf_mp3[I2S_DMA_PAGE_SIZE_MP3_C*I2S_DMA_PAGE_NUM];
SDRAM_DATA_SECTION static u8 i2s_rx_buf_mp3[I2S_DMA_PAGE_SIZE_MP3_C*I2S_DMA_PAGE_NUM];

#define I2S_SCLK_PIN	PC_1
#define I2S_WS_PIN		PC_0
#define I2S_SD_PIN		PC_2

static i2s_t i2s_obj;

float volume_ratio_c = 0.5; // codec volume ration. 0.0 ~1.0

static void i2s_tx_complete(void *data, char *pbuf)
{   
	//
}

static void i2s_rx_complete(void *data, char* pbuf)
{
	//
}

void audio_play_sd_m4a_c(u8* filename){  
	AVFormatContext *in = NULL;
	AVStream *st = NULL;
	void *wav = NULL;
	int ret, i;
	HANDLE_AACDECODER handle;
	AAC_DECODER_ERROR err;
	int frame_size = 0;
	CStreamInfo *info = NULL;
	//WavHeader pwavHeader;
	u32 wav_length = 0;
	u32 wav_offset = 0;
	int *ptx_buf;
	u32 start_time = 0;
	u32 start_heap = 0;
	start_time = rtw_get_current_time();
	start_heap = xPortGetFreeHeapSize();
	av_register_all();
	avformat_network_init();
	ret = avformat_open_input(&in, filename, NULL, NULL);
	if (ret < 0) {
		printf("open input failed\n");
		return;
	}
	for (i = 0; i < in->nb_streams && !st; i++) {
		if (in->streams[i]->codec->codec_id == AV_CODEC_ID_AAC)
			st = in->streams[i];
	}
	if (!st) {
		printf("No AAC stream found\n");
		return;
	}
	if (!st->codec->extradata_size) {
		printf("No AAC ASC found\n");
		return;
	}
	handle = aacDecoder_Open(TT_MP4_RAW, 1);
	err = aacDecoder_ConfigRaw(handle, &st->codec->extradata, &st->codec->extradata_size);
	if (err != AAC_DEC_OK) {
		printf("Unable to decode the ASC\n");
		return;
	}

#if WRITE_RAW_DATA_SD
	char abs_path_op[16]; //Path to output file
	FIL m_file;
	int bw = 0;
	const char *outfile = "AudioSDTest.pcm";
	extern char file_logical_drv[4]; //root diretor
	memset(abs_path_op, 0x00, sizeof(abs_path_op));
	strcpy(abs_path_op, file_logical_drv);
	sprintf(&abs_path_op[strlen(abs_path_op)],"%s", outfile);
	FRESULT res = f_open(&m_file, abs_path_op, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);  // open read only file
	if(res != FR_OK){
		printf("Open source file %(s) fail.\n", outfile);
		goto exit;
	}
#endif
	while (1){
		int i;
		UINT valid;
		AVPacket pkt = { 0 };
		int ret = av_read_frame(in, &pkt);
		if (ret < 0) {
			if (ret == AVERROR(EAGAIN))
				continue;
			break;
		}
		if (pkt.stream_index != st->index) {
			av_free_packet(&pkt);
			continue;
		}

		valid = pkt.size;
		err = aacDecoder_Fill(handle, &pkt.data, &pkt.size, &valid);
		if (err != AAC_DEC_OK) {
			printf("Fill failed: %x\n", err);
			break;
		}
		err = aacDecoder_DecodeFrame(handle, WAV_Buf_C, I2S_DMA_PAGE_SIZE_M4A_C, 0);
		av_free_packet(&pkt);
		if (err == AAC_DEC_NOT_ENOUGH_BITS)
			continue;
		if (err != AAC_DEC_OK) {
			printf("Decode failed: %x\n", err);
			continue;
		}
		if(!info){
			info = aacDecoder_GetStreamInfo(handle);
			if (!info || info->sampleRate <= 0) {
				printf("No stream info\n");
				break;
			}
			printf("sampleRate: %d, frameSize: %d, numChannels: %d\n", info->sampleRate, info->frameSize, info->numChannels);
			frame_size = info->frameSize * info->numChannels;
		}
#if WRITE_RAW_DATA_SD
		do{
			res = f_write(&m_file, WAV_Buf_C, frame_size*2, (u32*)&bw);
			if(res){
				f_lseek(&m_file, 0); 
				printf("Write error.\n");
			}
			//printf("Write %d bytes.\n", bw);
		}while(bw < frame_size*2);
		bw = 0;
#endif
retry:
		ptx_buf = i2s_get_tx_page(&i2s_obj);
		if(ptx_buf){
			if(frame_size*2 <= I2S_DMA_PAGE_SIZE_M4A_C){
				/* if valid audio data short than a page, make sure the rest of the page set to 0*/
				memset((void*)ptx_buf, 0x00, I2S_DMA_PAGE_SIZE_M4A_C);
				memcpy((void*)ptx_buf, (void*)WAV_Buf_C, frame_size*2);
			}else
				memcpy((void*)ptx_buf, (void*)WAV_Buf_C, frame_size*2);
			i2s_send_page(&i2s_obj, (uint32_t*)ptx_buf);
		}else{
			vTaskDelay(1);
			goto retry;
		}
	}
exit:
#if WRITE_RAW_DATA_SD
	res = f_close(&m_file);
	if(res){
		printf("close file (%s) fail.\n", outfile);
	}
#endif
	printf("decoding finished (Heap used: 0x%x, Time passed: %dms)\n", start_heap - xPortGetFreeHeapSize(), rtw_get_current_time() - start_time);
	avformat_close_input(&in);
	aacDecoder_Close(handle);
	return;
}


void audio_play_sd_mp3_c(u8* filename){
	mp3_decoder_t mp3;
	mp3_info_t info;
	int drv_num = 0;
	int frame_size = 0;
	u32 read_length = 0;
	FRESULT res; 
	FATFS 	m_fs;
	FIL		m_file[2];  
	char	logical_drv[4]; //root diretor
	char abs_path[32]; //Path to input file
	DWORD bytes_left;
	DWORD file_size;
	int i = 0;
	int bw = 0;
    volatile u32 tim1 = 0;
    volatile u32 tim2 = 0;

	//WavHeader pwavHeader;
	u32 wav_length = 0;
	u32 wav_offset = 0;
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
	res = f_open(&m_file[0], abs_path, FA_OPEN_EXISTING | FA_READ); // open read only file
	if(res != FR_OK){
		printf("Open source file %s fail.\n", filename);
		goto umount;
	}
	
	file_size = (&m_file[0])->fsize;
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
		if(bytes_left >= INPUT_FRAME_SIZE_C)
		res = f_read(&m_file[0], MP3_Buf_C, INPUT_FRAME_SIZE_C,(UINT*)&read_length);	
		else if(bytes_left > 0)
		res = f_read(&m_file[0], MP3_Buf_C, bytes_left,(UINT*)&read_length);	
		if((res != FR_OK))
		{
			printf("Wav play done !\n");
			break;
		}
		frame_size = mp3_decode(mp3, MP3_Buf_C, INPUT_FRAME_SIZE_C, WAV_Buf_C, &info);
		bytes_left = bytes_left - frame_size;
		f_lseek(&m_file[0], (file_size-bytes_left));
                //printf("frame size [%d], Decoded bytes [%d]\n",frame_size,info.audio_bytes);
		
		retry:
		ptx_buf = i2s_get_tx_page(&i2s_obj);
		if(ptx_buf){
			if(info.audio_bytes < I2S_DMA_PAGE_SIZE_MP3_C){
				/* if valid audio data short than a page, make sure the rest of the page set to 0*/
				memset((void*)ptx_buf, 0x00, I2S_DMA_PAGE_SIZE_MP3_C);
				memcpy((void*)ptx_buf, (void*)WAV_Buf_C, info.audio_bytes);
			}else
				memcpy((void*)ptx_buf, (void*)WAV_Buf_C, info.audio_bytes);
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
	res = f_close(&m_file[0]);
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
        

        mp3_free(mp3);
	
}



void example_audio_m4a_mp3_thread(void* param){
        vTaskDelay(4000);
	printf("Audio codec demo begin......\n");

#if CONFIG_EXAMPLE_M4A_STREAM_ALC5651
	alc5651_init();
	alc5651_init_interface2();
#endif

	i2s_obj.channel_num = NUM_CHANNELS_MP3_C;
	i2s_obj.sampling_rate = SAMPLING_FREQ_MP3_C;
	i2s_obj.word_length = WL_16b;
	i2s_obj.direction = I2S_DIR_TXRX;	 
	i2s_init(&i2s_obj, I2S_SCLK_PIN, I2S_WS_PIN, I2S_SD_PIN);
	i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buf_mp3, (char*)i2s_rx_buf_mp3, \
		I2S_DMA_PAGE_NUM, I2S_DMA_PAGE_SIZE_MP3_C);
	i2s_tx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_tx_complete, (uint32_t)&i2s_obj);
	i2s_rx_irq_handler(&i2s_obj, (i2s_irq_handler)i2s_rx_complete, (uint32_t)&i2s_obj);

#if CONFIG_EXAMPLE_M4A_STREAM_SGTL5000
	sgtl5000_enable();
	sgtl5000_setVolume(volume_ratio_c);
#endif
	audio_play_sd_mp3_c(FILE_NAME_MP3_C);
	vTaskDelay(2000);
        i2s_obj.channel_num = NUM_CHANNELS_M4A_C;
	i2s_obj.sampling_rate = SAMPLING_FREQ_M4A_C;
	i2s_set_dma_buffer(&i2s_obj, (char*)i2s_tx_buf_m4a, (char*)i2s_rx_buf_m4a, \
		I2S_DMA_PAGE_NUM, I2S_DMA_PAGE_SIZE_M4A_C);
	audio_play_sd_m4a_c(FILE_NAME_M4A_C);
        
exit:
	vTaskDelete(NULL);
}

void example_audio_m4a_mp3(void)
{
	if(xTaskCreate(example_audio_m4a_mp3_thread, ((const char*)"example_audio_m4a_thread"), 2000, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_audio_m4a_thread) failed", __FUNCTION__);
}

