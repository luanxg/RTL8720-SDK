 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "sip/re/inc/re.h"
#include "sip/re/inc/baresip.h"
#include "sip/re/inc/test.h"

#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include "wifi_conf.h"


const char *Register_URI  = "sip:54.37.202.229";
const char *My_URI  = "sip:amebatest01@sip.linphone.org;auth_user=amebatest01;auth_pass=RealtekSG;regint=600";



static void ua_exit_handler(int arg)
{
	(int)arg;

	printf("ua exited -- stopping main runloop\n");

	ua_stop_all(true);
	ua_sip_close();
	
	re_cancel();
}

static void sip_ua_register_thread(void *param)
{
	bool prefer_ipv6 = false;
	struct config *config;
	size_t i, ntests;
	bool verbose = false;
	int err;
	struct sa nsv[16];
	struct dnsc *dnsc = NULL;
	struct sa laddr;
	uint32_t nsc;
	struct ua *ua= NULL;
	
	 while (wifi_is_ready_to_transceive(RTW_STA_INTERFACE) != RTW_SUCCESS) {
        vTaskDelay(1000);
    };
	err = libre_init();
	if (err)
		goto out;

	
	/* note: run SIP-traffic on localhost */
	config = conf_config();
	if (!config) {
		err = ENOENT;
		goto out;
	}

	
	/*
	 * Initialise the top-level baresip struct, must be
	 * done AFTER configuration is complete.
	 */
	err = baresip_init(config, prefer_ipv6);
	if (err) {
		printf("main: baresip init failed (%d)\n", err);
		goto out;
	}
	
	/* Initialise User Agents */
	err = ua_init("MMFV2 UA Test" ,
		      true, true, false, false);
	if (err)
		{
		printf("main: ua_init failed (%d)\n", err);
		goto out;
	}
 
	err = ua_alloc(&ua, My_URI,Register_URI,TRUE);
	if (err)
		{
		printf("main: ua_alloc failed (%d)\n", err);
		goto out;
	}
	

	printf("\r\n\x1b[32mMMFV2 UA module SIP Register Start\x1b[;m\n");
	/* main loop */
	err = re_main(ua_exit_handler);
 out:
	if (err) {
		printf("test failed (%d)\n", err);
		
	}
	
	ua_stop_all(true);
	ua_sip_close();

	baresip_close();

	libre_close();

	tmr_debug();
	mem_debug();
	vTaskDelete(NULL);
	
}

void example_ua_register(void)
{
    if(xTaskCreate(sip_ua_register_thread, ((const char*)"sip_ua_register_thread"), 2048, NULL, tskIDLE_PRIORITY+1, NULL) != pdPASS)
        printf("\n\r%s xTaskCreate(sip_ua_register_thread) failed\n", __FUNCTION__);
    return;
}


