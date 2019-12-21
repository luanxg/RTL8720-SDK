#include "mp3/mp3_codec.h"
#include "example_flash_mp3.h"
#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "i2s_api.h"
#include "analogin_api.h"
#include <stdlib.h>
#include "platform_opts.h"
#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include "section_config.h"

#include "flash_api.h"
#include <disk_if/inc/flash_fatfs.h>

#if CONFIG_EXAMPLE_FLASH_MP3
#if (_MAX_SS != 4096)
	#error set _MAX_SS MACRO to 4096 in ffconf.h to define maximum supported range of sector size for flash memory. See the description below the MACRO for details.
#endif
#if (_USE_MKFS != 1)
	#error define _USE_MKFS MACRO to 1 in ffconf.h for on-board flash memory to enable f_mkfs() which creates FATFS volume on Flash.
#endif
#endif

#if CONFIG_EXAMPLE_MP3_STREAM_SGTL5000
#include "sgtl5000.h"
#else
#include "alc5651.h"
#endif

#include <lwip/sockets.h>
#include <lwip/netdb.h>

#define SERVER_HOST		"172.20.10.4"
#define SERVER_PORT		8082
#define RESOURCE		""
#define BUFFER_SIZE		2048
#define RECV_TO		5000	// ms

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

#define FILE_NAME "AudioFlash.mp3"    //Specify the file name you wish to play that is present in the SDCARD

//------------------------------------- ---CONFIG Parameters-----------------------------------------------//



#define INPUT_FRAME_SIZE 1500

SDRAM_DATA_SECTION unsigned char MP3_Flash_Buf[INPUT_FRAME_SIZE]; 
SDRAM_DATA_SECTION signed short WAV_Flash_Buf[I2S_DMA_PAGE_SIZE];

#define I2S_DMA_PAGE_NUM	2   // Vaild number is 2~4

static u8 i2s_tx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];
static u8 i2s_rx_buf[I2S_DMA_PAGE_SIZE*I2S_DMA_PAGE_NUM];

#define I2S_SCLK_PIN		PC_1
#define I2S_WS_PIN			PC_0
#define I2S_SD_PIN			PC_2

static i2s_t i2s_obj;

static void i2s_tx_complete(void *data, char *pbuf)
{
	//
}

static void i2s_rx_complete(void *data, char* pbuf)
{
	//
}


void audio_play_flash_mp3(u8* filename){
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
	int bw;

	//WavHeader pwavHeader;
	//u32 wav_length = 0;
	//u32 wav_offset = 0;
	int *ptx_buf;

	drv_num = FATFS_RegisterDiskDriver(&FLASH_disk_Driver);
	if(drv_num < 0){
		printf("Rigester Flash disk driver to FATFS fail.\n");
		return;
	}else{
		logical_drv[0] = drv_num + '0';
		logical_drv[1] = ':';
		logical_drv[2] = '/';
		logical_drv[3] = 0;
	}

	// Register work area using mount 
	res = f_mount(&m_fs, logical_drv, 1);
	printf("res value mount: %d\n", res);
	if(res){
		printf("FATFS mount logical drive fail.\n");
		// Create FAT volume using mkfs 
		res = f_mkfs(logical_drv, 1, 4096);
		printf("res value mount mkfs: %d\n", res);
		if(res)
			goto unreg;
		if(f_mount(&m_fs, logical_drv, 0)!= FR_OK){
			printf("FATFS mount logical drive fail.\n");
			goto unreg;
		}
	}
	 
	memset(abs_path, 0x00, sizeof(abs_path));
	strcpy(abs_path, logical_drv);
	sprintf(&abs_path[strlen(abs_path)],"%s", filename);

	//Open source file
	res = f_open(&m_file, abs_path, FA_OPEN_EXISTING | FA_READ); // open read only file
	if((res == FR_NO_FILE) || (f_size(&m_file) == 0)){
		//Open new source file
		res = f_open(&m_file, abs_path, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
		printf("res value open file: %d\n", res);
		if(res){
			printf("open file (%s) fail.\n", filename);
			goto unmount;
		}
		
		printf("Test file name:%s\n\n",filename);
		res = f_lseek(&m_file, f_size(&m_file)); 
		if(res)
			printf("New file.\n");
		
		//1 Receive audio file through http
		int server_fd = -1, ret;
		struct sockaddr_in server_addr;
		struct hostent *server_host;
		
		// Delay to wait for IP by DHCP
		vTaskDelay(10000);
		
		if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			printf("ERROR: socket\n");
			goto exit;
		}
		else {
			int recv_timeout_ms = RECV_TO;
#if defined(LWIP_SO_SNDRCVTIMEO_NONSTANDARD) && (LWIP_SO_SNDRCVTIMEO_NONSTANDARD == 0)	// lwip 1.5.0
			struct timeval recv_timeout;
			recv_timeout.tv_sec = recv_timeout_ms / 1000;
			recv_timeout.tv_usec = recv_timeout_ms % 1000 * 1000;
			setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
#else	// lwip 1.4.1
			setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout_ms, sizeof(recv_timeout_ms));
#endif
		}
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(SERVER_PORT);
		
		// Support SERVER_HOST in IP or domain name
		server_host = gethostbyname(SERVER_HOST);
		memcpy((void *) &server_addr.sin_addr, (void *) server_host->h_addr, server_host->h_length);

		if(connect(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == 0) {
			unsigned char buf[BUFFER_SIZE + 1];
			int pos = 0, read_size = 0, resource_size = 0, content_len = 0, header_removed = 0;

			sprintf(buf, "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n", RESOURCE, SERVER_HOST);
			write(server_fd, buf, strlen(buf));

			while((read_size = read(server_fd, buf + pos, BUFFER_SIZE - pos)) > 0) {
				// printf("Body: %s\n", buf);
				if(header_removed == 0) {
					char *header = NULL;

					pos += read_size;
					buf[pos] = 0;
					header = strstr(buf, "\r\n\r\n");

					if(header) {
						char *body, *content_len_pos;

						body = header + strlen("\r\n\r\n");
						*(body - 2) = 0;
						header_removed = 1;
						printf("\nHTTP Header: %s\n", buf);

						// Remove header size to get first read size of data from body head
						read_size = pos - ((unsigned char *) body - buf);
						pos = 0;

						content_len_pos = strstr(buf, "Content-Length: ");
						if(content_len_pos) {
							content_len_pos += strlen("Content-Length: ");
							*(char*)(strstr(content_len_pos, "\r\n")) = 0;
							content_len = atoi(content_len_pos);
						}
					}
					else {
						if(pos >= BUFFER_SIZE){
							printf("ERROR: HTTP header\n");
							goto exit;
						}
						continue;
					}
				}
				printf("read resource %d bytes\n", read_size);
				resource_size += read_size;

// Write to Flash
				bw = 0;
				while(bw < read_size){
 					f_lseek(&m_file, f_size(&m_file));
					res = f_write(&m_file, buf, read_size, (u32*)&bw);
					printf("\nres value write file: %d\n", res);
					printf("\nbw value write file: %d\n", bw);
					if(res){
						f_lseek(&m_file, 0); 
						printf("Write error.\n");
					}
					else if(res == 0 && bw == 0){
						printf("Not enough space in Flash\n");
						goto exit;
 					}
					else {
						printf("Write %d bytes.\n", bw);
						//printf("\nWrite content:\n%s\n", buf);
					}
				}

				printf("\nThe size of the file %d\n", f_size(&m_file));
			}

			printf("exit read. ret = %d\n", read_size);
			printf("http content-length = %d bytes, download resource size = %d bytes\n", content_len, resource_size);
		}
		else {
			printf("ERROR: connect\n");
		}
		if(server_fd >= 0)
			close(server_fd);
	}
	else {
		if(res != RES_OK){
			printf("Open source file %s fail.\n", filename);
			goto unmount;
		}
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
			res = f_read(&m_file, MP3_Flash_Buf, INPUT_FRAME_SIZE,(UINT*)&read_length);	
		else if(bytes_left > 0)
			res = f_read(&m_file, MP3_Flash_Buf, bytes_left,(UINT*)&read_length);	
		if((res != FR_OK))
		{
			printf("Wav play done !\n");
			break;
		}
		frame_size = mp3_decode(mp3, MP3_Flash_Buf, INPUT_FRAME_SIZE, WAV_Flash_Buf, &info);
		bytes_left = bytes_left - frame_size;
		f_lseek(&m_file, (file_size-bytes_left));
		//printf("frame size [%d], Decoded bytes [%d]\n",frame_size,info.audio_bytes);
		
		retry:
		ptx_buf = i2s_get_tx_page(&i2s_obj);
		if(ptx_buf){
			if(info.audio_bytes < I2S_DMA_PAGE_SIZE){
				/* if valid audio data short than a page, make sure the rest of the page set to 0*/
				memset((void*)ptx_buf, 0x00, I2S_DMA_PAGE_SIZE);
				memcpy((void*)ptx_buf, (void*)WAV_Flash_Buf, info.audio_bytes);
			}else
				memcpy((void*)ptx_buf, (void*)WAV_Flash_Buf, info.audio_bytes);
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

unmount:
	if(f_mount(NULL, logical_drv, 1) != FR_OK){
		printf("FATFS unmount logical drive fail.\n");
	}

unreg:	
	if(FATFS_UnRegisterDiskDriver(drv_num))	
		printf("Unregister disk driver from FATFS fail.\n");
	
}

void example_flash_mp3_thread(void* param){
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
	audio_play_flash_mp3(wav);
exit:
	vTaskDelete(NULL);
}

void example_flash_mp3(void)
{
	if(xTaskCreate(example_flash_mp3_thread, ((const char*)"example_flash_mp3_thread"), 4096, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_flash_mp3_thread) failed", __FUNCTION__);
}

