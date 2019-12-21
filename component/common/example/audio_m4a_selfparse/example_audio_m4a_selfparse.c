#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "wifi_conf.h"
#include "lwip/arch.h"
#include "i2s_api.h"
#include "analogin_api.h"
#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>
#include <disk_if/inc/sdcard.h>
#include "section_config.h"
#include "libAACdec/include/aacdecoder_lib.h"
#include <libavformat/avformat.h>
#include <libavformat/url.h>

#define FILE_NAME "sound.m4a"    //Specify the file name you wish to play that is present in the SDCARD

void audio_play_sd_m4a_selfparse(u8* filename)
{
	int drv_num = 0;
	FRESULT res; 
	FATFS 	m_fs;
	FIL		m_file;  
	char	logical_drv[4]; //root diretor
	char abs_path[32]; //Path to input file
	DWORD bytes_left;
	DWORD file_size;
		u32 start_time = 0;
        
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
        
        check_m4a_file(&m_file);
        parse_esds(&m_file);
	parse_stsc(&m_file);
	parse_stsz(&m_file);
	parse_stco(&m_file);
	start_time = rtw_get_current_time();
	play_raw_adts(m_file);
	printf("decoding finished (Time passed: %dms)\n", rtw_get_current_time() - start_time);
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

void example_audio_m4a_selfparse_thread(void* param){

	printf("Audio codec demo begin......\n");
	audio_play_sd_m4a_selfparse(FILE_NAME);
exit:
	vTaskDelete(NULL);
}

void example_audio_m4a_selfparse(void)
{
	if(xTaskCreate(example_audio_m4a_selfparse_thread, ((const char*)"example_audio_m4a_selfparse_thread"), 4000, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_audio_m4a_selfparse_thread) failed", __FUNCTION__);
}

