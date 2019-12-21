// Copyright (2017) Baidu Inc. All rights reserved.
/**
 * File: duerapp.c
 * Auth: Su Hao(suhao@baidu.com)
 * Desc: Duer Application Wi-Fi configuration.
 */

#include "FreeRTOS.h"
#include "task.h"

#include "duerapp_config.h"
#include "wifi_conf.h"
#include "wlan_fast_connect/example_wlan_fast_connect.h"
#include "lwip_netconf.h"
#include "flash_api.h"
#include "device_lock.h"

#define EXAMPLE_WIFI_SSID "iottest"
#define EXAMPLE_WIFI_PASS "12345678"

static int Erase_Fastconnect_data(){
	flash_t flash;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&flash, FAST_RECONNECT_DATA);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return 0;
}
void initialize_wifi(void)
{
#if CONFIG_INCLUDE_SIMPLE_CONFIG
	extern u8 psk_essid[NET_IF_NUM][NDIS_802_11_LENGTH_SSID+4];
	extern u8 psk_passphrase[NET_IF_NUM][IW_PASSPHRASE_MAX_SIZE + 1];
	extern u8 psk_passphrase64[IW_PASSPHRASE_MAX_SIZE + 1];
	int wait_cnt = 20;//20*1s = 20s

	vTaskDelay(500);
	DUER_LOGI("Stored Essid %s, password %s(%s)\n", psk_essid[0], psk_passphrase[0], psk_passphrase64);
	if(psk_essid[0][0] == '\0' && (psk_passphrase[0][0] == '\0' || (psk_passphrase64[0] == '\0')))
	{
SimpleConfig:	
#if CONFIG_AUTO_RECONNECT
		wifi_set_autoreconnect(0);
#endif
		DUER_LOGI("Start Simple Config......\n");
		duerapp_gpio_led_mode(DUER_LED_FAST_TWINKLE_150);
		cmd_simple_config(0, NULL);
		wait_cnt = 3;
	}
	while(wait_cnt > 0)
	{
		duerapp_gpio_led_mode(DUER_LED_FAST_TWINKLE_300);
		if(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) == RTW_SUCCESS)
			break;
		vTaskDelay(1000);
		wait_cnt --;
		if(wait_cnt == 0){
			Erase_Fastconnect_data();
			goto SimpleConfig;
		}
	}
	DUER_LOGI("Wi-Fi has been connected to AP.\n");
#if CONFIG_AUTO_RECONNECT
	//setup reconnection flag
	wifi_set_autoreconnect(1);
	wifi_config_autoreconnect(1, 20, 5);
#endif

#else //CONFIG_INCLUDE_SIMPLE_CONFIG
	while(1)
	{
		if(wifi_is_ready_to_transceive(RTW_STA_INTERFACE) == RTW_SUCCESS)
			break;
		vTaskDelay(1000);
	}
#endif //CONFIG_INCLUDE_SIMPLE_CONFIG

}
