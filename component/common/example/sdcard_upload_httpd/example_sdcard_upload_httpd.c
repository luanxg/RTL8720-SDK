#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include "basic_types.h"
#include "platform_opts.h"
#include "section_config.h"

#include <httpd/httpd.h>

#if CONFIG_SDCARD_UPLOAD_HTTPD

#include "ff.h"
#include <fatfs_ext/inc/ff_driver.h>

#include "sdio_combine.h"
#include "sdio_host.h"
#include <disk_if/inc/sdcard.h>
#define STACK_SIZE		2048

//#if (_USE_MKFS != 1)
//	#error define _USE_MKFS MACRO to 1 in ffconf.h to enable f_mkfs() which creates FATFS volume on Flash.
//#endif


#include "sdio_combine.h"
#define	_DBG_SDIO_          	0x00000400
extern phal_sdio_host_adapter_t psdioh_adapter;

FATFS 	fs_sd;
FIL     m_file;
FILINFO fno = {0};
char path[64];

#define READ_SIZE 2048

#define MP4_NAME "AMEBA_0.mp4"

int fatfs_init(char *file_name)
{ 
	char sd_drv[4]; /* root diretory */
	int sd_drv_num;
	int br,bw;
	int Fatfs_ok = 0;
	FRESULT res; 
	
	sdio_driver_init();
  	  
	printf("Register disk driver to Fatfs.\n\r");
	sd_drv_num = FATFS_RegisterDiskDriver(&SD_disk_Driver);
	
	if(sd_drv_num < 0){
		printf("Rigester disk driver to FATFS fail.\n\r");
	}else{
		Fatfs_ok = 1;
		
		sd_drv[0] = sd_drv_num + '0';
		sd_drv[1] = ':';
		sd_drv[2] = '/';
		sd_drv[3] = 0;
	}
	
	if(Fatfs_ok){
		res = f_mount(&fs_sd, sd_drv, 1);
		
		if(res){
#if 0
			if(f_mkfs(sd_drv, 1, 4096)!= FR_OK){
				printf("Create FAT volume on Flash fail.\n\r");
				goto exit;
			}
#endif
			if(f_mount(&fs_sd, sd_drv, 0)!= FR_OK){
				printf("FATFS mount logical drive on Flash fail.\n\r");
				goto exit;
			}
		}else{
			printf("f_mount OK\n\r");
		} 
		
		strcpy(path, sd_drv);
		sprintf(&path[strlen(path)],"%s",file_name);
		
		//FILINFO fno;
		
		res = f_stat(path, &fno);
		switch (res) {
			
			case FR_OK:
				printf("Size: %lu\n", fno.fsize);
				printf("Timestamp: %u/%02u/%02u, %02u:%02u\n",
							(fno.fdate >> 9) + 1980, fno.fdate >> 5 & 15, fno.fdate & 31,
							fno.ftime >> 11, fno.ftime >> 5 & 63);
				printf("Attributes: %c%c%c%c%c\n",
							(fno.fattrib & AM_DIR) ? 'D' : '-',
							(fno.fattrib & AM_RDO) ? 'R' : '-',
							(fno.fattrib & AM_HID) ? 'H' : '-',
							(fno.fattrib & AM_SYS) ? 'S' : '-',
							(fno.fattrib & AM_ARC) ? 'A' : '-');
				break;
				
			case FR_NO_FILE:
				printf("It is not exist.\n");
				break;
				
			default:
				printf("An error occured. (%d)\n", res);
		}
		//3.open 
		res = f_open(&m_file, path, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
		if(res){
			printf("open file (%s) fail. res = %d\n\r", file_name,res);
			goto exit;
		}
	}  
	return 0;
exit:
	return -1;
}



void fatfs_homepage_cb(struct httpd_conn *conn)
{
	char *user_agent = NULL;
	FRESULT res; 
	unsigned char *buf_ptr= malloc(READ_SIZE);
	int ret = 0;
	unsigned int time_1 = 0;
	unsigned int time_2 = 0;
	
	memset(buf_ptr,0,READ_SIZE);
	
	httpd_conn_dump_header(conn);
	
	if(httpd_request_get_header_field(conn, "User-Agent", &user_agent) != -1) {
		printf("\nUser-Agent=[%s]\n", user_agent);
		httpd_free(user_agent);
	}
	
	if(httpd_request_is_method(conn, "GET")) {
		httpd_response_write_header_start(conn, "200 OK", "application/octet-stream", fno.fsize);
		httpd_response_write_header(conn,"Content-Disposition","attachment; filename=""test.mp4""");
		httpd_response_write_header(conn, "Connection", "close");
		httpd_response_write_header_finish(conn);
		
		int i = 0;
		
		int br,bw;
		
		time_1 = rtw_get_current_time();
		printf("start time =%d\r\n",time_1);
		for(i=0;i<fno.fsize/READ_SIZE;i++){
			res = f_read(&m_file, buf_ptr, READ_SIZE, (u32*)&br);
			if(res){
				printf("Read error. re = %d\n\r",res);
				f_lseek(&m_file, 0);
				res = f_close(&m_file);
				goto exit;
			}
                        //ret = httpd_response_write_data(conn, buf_temp_ptr, READ_SIZE);
                        ret = httpd_response_write_data(conn, buf_ptr, READ_SIZE);
		}
                if(fno.fsize%READ_SIZE){
                        res = f_read(&m_file, buf_ptr, i%fno.fsize, (u32*)&br);
                        ret = httpd_response_write_data(conn, buf_ptr, fno.fsize%READ_SIZE);
                }
		
	}else {
		// HTTP/1.1 405 Method Not Allowed
		httpd_response_method_not_allowed(conn, NULL);
	}
	
	time_2 = rtw_get_current_time();
	printf("start time =%d ms end time =%d ms cost = %d ms  speed = %f MB/s\r\n",time_1,time_2,(time_2-time_1),(float)((float)(fno.fsize)/((float)(time_2-time_1))/1000.f));
	
	if(res){
		printf("close file (%s) fail.\n\r", MP4_NAME);
	}
	
	if(buf_ptr)
		free(buf_ptr);
	
	res = f_close(&m_file);
	
exit:
	httpd_conn_close(conn);
}

#include "wifi_constants.h"
#include "wifi_structures.h"
#include "lwip_netconf.h"

static void wifi_init()
{
	rtw_mdelay_os(2000);
	
	printf("\r\nuse ATW0, ATW1, ATWC to make wifi connection\r\n");
	printf("wait for wifi connection...\r\n");
	
	while (wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS){
		vTaskDelay(1000);
	}
}


void example_sdcard_upload_httpd_main(void *parm)
{
	int ret = 0;
	wifi_init();
	ret = fatfs_init(MP4_NAME);
	if(ret >= 0){
		printf("open file %s successful\r\n",MP4_NAME);
		httpd_reg_page_callback("/", fatfs_homepage_cb);
		httpd_reg_page_callback("/index.htm", fatfs_homepage_cb);
		if(httpd_start(80, 5, 4096, HTTPD_THREAD_SINGLE, HTTPD_SECURE_NONE) != 0) {
			printf("ERROR: httpd_start");
			httpd_clear_page_callbacks();
		}
	}else{
		printf("open %s failed\r\n",MP4_NAME);
	}
	
exit:
	vTaskDelete(NULL);
}

void example_sdcard_upload_httpd(void)
{
	if(xTaskCreate(example_sdcard_upload_httpd_main, ((const char*)"example_sdcard_upload_httpd_main"), STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_sdcard_upload_httpd_main) failed", __FUNCTION__);
}

#endif