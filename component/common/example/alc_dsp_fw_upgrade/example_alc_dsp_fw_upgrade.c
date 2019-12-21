#include <FreeRTOS.h>
#include <task.h>
#include "device.h"
#include "gpio_api.h"   // mbed
#include <platform/platform_stdlib.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include "alc5680.h"

#include "wifi_conf.h"
#include "tftp/tftp.h"

#define GPIO_ALC_RESET_PIN      PA_3

#define ALC_DSP_FIRMWARE_NAME "alc_fw_upgrade.bin"
#define TFTP_HOST_IP_ADDR "192.168.1.100"
#define TFTP_HOST_PORT  69
#define TFTP_MODE       "octet"

#define ALC_5680_RESET 1
#define FORCE_UPGRADE  0

#define INTERNAL_TEST 0

#if INTERNAL_TEST
extern char upgrade_ip[64];
#endif

static void check_wifi_connection()
{
    while(1){
        vTaskDelay(1000);
#if INTERNAL_TEST
        if( wifi_is_ready_to_transceive(RTW_STA_INTERFACE) == RTW_SUCCESS && upgrade_ip[0]!=0) {
          printf("wifi is connected\r\n");
          printf("wifi is connected ip = %s\r\n",upgrade_ip);
          break;
        }
#else
        if( wifi_is_ready_to_transceive(RTW_STA_INTERFACE) == RTW_SUCCESS ){
              printf("wifi is connected\r\n");
              break;
        }
#endif
    }
}



static void example_alc_codec_fw_upgrade_thread(void *param)
{
        tftp tftp_info;
        unsigned int value = 0;
        unsigned int test = 0;
        gpio_t gpio_alc_reset;
        memset(&tftp_info,0,sizeof(tftp));
        
        check_wifi_connection();
        
         // Init alc reset control pin
#if ALC_5680_RESET    
        printf("GPIO RESET\r\n");
        gpio_init(&gpio_alc_reset, GPIO_ALC_RESET_PIN);
        gpio_dir(&gpio_alc_reset, PIN_OUTPUT);    // Direction: Output
        gpio_mode(&gpio_alc_reset, PullNone);     // No pull  
        gpio_write(&gpio_alc_reset, 1);
#endif
        vTaskDelay(1000);//Wait for alc codec reset
        

        printf("ALC568W_FW_UPGRADE\r\n");
        alc5680_i2c_init();       
        alc5680_status();
        alc5680_reg_read_16(0x1800C0CA,&value);
        if(value == 0x1fc){    
#if FORCE_UPGRADE
            printf("The firmware already exist and force upgrade now...\r\n");
#else
            printf("The firmware already exist\r\n");
            goto EXIT;
#endif
        }
        
       
        alc5680_erase();
        alc5680_reg_read_32(0x70000000,&value);
        printf("addr 0x70000000 = %x\r\n",value);
        alc5680_status();
        tftp_info.recv_handle = alc5680_handler_function;
        

        tftp_info.tftp_file_name=ALC_DSP_FIRMWARE_NAME;

       
        printf("upgrade file name = %s\r\n",tftp_info.tftp_file_name);
        tftp_info.tftp_mode = TFTP_MODE;
        tftp_info.tftp_port = TFTP_HOST_PORT;
#if INTERNAL_TEST
        tftp_info.tftp_host = upgrade_ip;
#else
        tftp_info.tftp_host = TFTP_HOST_IP_ADDR;
#endif
        tftp_info.tftp_op = RRQ;//FOR READ OPERATION
        tftp_info.tftp_retry_num = 5;
        tftp_info.tftp_timeout = 5;//second
        if(tftp_client_start(&tftp_info) == 0)
          printf("Firmware upgrade successful\r\n");
        else
          printf("Firmware upgrade fail\r\n");
        alc5680_reg_read_32(0x70000000,&value);
        printf("addr 0x70000000 = %x\r\n",value);
        alc5680_check_sum();
#if ALC_5680_RESET  
        printf("GPIO RESET\r\n");
        gpio_write(&gpio_alc_reset, 1);
        vTaskDelay(100);
        gpio_write(&gpio_alc_reset, 0);
        vTaskDelay(100);
        gpio_write(&gpio_alc_reset, 1);
        vTaskDelay(1000);
        alc5680_status();
#endif

EXIT:
        vTaskDelete(NULL);
}

void example_alc_dsp_fw_upgrade(void)
{
	if(xTaskCreate(example_alc_codec_fw_upgrade_thread, ((const char*)"example_http_download_thread"), 4096, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}