/******************************************************************************
 *
 * Copyright(c) 2007 - 2015 Realtek Corporation. All rights reserved.
 *
 *
 ******************************************************************************/
#include <platform_opts.h>
#include <wifi_manager/example_wifi_manager.h>
#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include <wifi/wifi_conf.h>
#include "semphr.h"
#include <autoconf.h>

#define MAX_RETRY_TIMEOUT 60000
#define RETRY_NUM 5

extern int wlan_init_done_callback();

xSemaphoreHandle wm_assoc_sema,wm_discon_sema;

int retry_timeout[RETRY_NUM] = {1000,5000,10000,30000,MAX_RETRY_TIMEOUT};

//For this example, must disable auto-reconnect in driver layer because we do reconnect in up layer.
//#define CONFIG_AUTO_RECONNECT 0 , in autoconf.h
static void wifi_manager_disconnect_hdl(void)
{
    static int retry_count = 0;
        
    if(retry_count >= RETRY_NUM){
      vTaskDelay(retry_timeout[RETRY_NUM-1]);
    }
    else{
      vTaskDelay(retry_timeout[retry_count]);
    }
    retry_count++;
    
    printf("\n%s : Reconnect to AP (%d)\n",__func__,retry_count);
    //Do fast connection to reconnect
    wlan_init_done_callback();
    
    return;
}

static void wifi_manager_thread(void *param)
{
    //Register wifi event callback function
    wifi_reg_event_handler(WIFI_EVENT_NO_NETWORK,example_wifi_manager_no_network_cb,NULL);
    wifi_reg_event_handler(WIFI_EVENT_CONNECT, example_wifi_manager_connect_cb, NULL);
    wifi_reg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, example_wifi_manager_fourway_done_cb, NULL);
    wifi_reg_event_handler(WIFI_EVENT_DISCONNECT, example_wifi_manager_disconnect_cb, NULL);

    //Can use semaphore to wait wifi event happen
    //The following example shows that we wait for wifi association
    rtw_init_sema(&wm_assoc_sema, 0);
    if(!wm_assoc_sema){
       printf("\nInit wm_assoc_sema failed\n");
    }
    if(rtw_down_timeout_sema(&wm_assoc_sema, WM_ASSOC_SEMA_TIMEOUT) == RTW_FALSE) {
       rtw_free_sema(&wm_assoc_sema);
    }

    rtw_init_sema(&wm_discon_sema, 0);
    if(!wm_discon_sema){
       printf("\nInit wm_discon_sema failed\n");
    }
    
    int condition = 1;
    while(condition != 0){
        //Example: When wifi disconnect, call disconnect handle function.
        if(rtw_down_sema(&wm_discon_sema) == RTW_FALSE) {
            rtw_free_sema(&wm_discon_sema);
        }
        wifi_manager_disconnect_hdl();
    }

    //Unregister wifi event callback function
    wifi_unreg_event_handler(WIFI_EVENT_NO_NETWORK,example_wifi_manager_no_network_cb);
    wifi_unreg_event_handler(WIFI_EVENT_CONNECT, example_wifi_manager_connect_cb);
    wifi_unreg_event_handler(WIFI_EVENT_FOURWAY_HANDSHAKE_DONE, example_wifi_manager_fourway_done_cb);
    wifi_unreg_event_handler(WIFI_EVENT_DISCONNECT, example_wifi_manager_disconnect_cb);

    if(wm_assoc_sema){
       rtw_free_sema(&wm_assoc_sema);
    }
    if(wm_discon_sema){
       rtw_free_sema(&wm_discon_sema);
    }

    vTaskDelete(NULL);
}

//Wifi no network callback
void example_wifi_manager_no_network_cb(char* buf, int buf_len, int flags, void* userdata)
{
    printf("\n\rCan not find network!!\n");
    return;
}

//Wifi association done callback
void example_wifi_manager_connect_cb(char* buf, int buf_len, int flags, void* userdata)
{
    rtw_up_sema(&wm_assoc_sema);
    printf("\n\rWifi association done!!\n");
    return;
}

//Wifi four-way handshake done callback
void example_wifi_manager_fourway_done_cb(char* buf, int buf_len, int flags, void* userdata)
{
    printf("\n\rWifi four-way handshake done!!\n");
    return;
}

//Wifi disconnect callback
void example_wifi_manager_disconnect_cb(char* buf, int buf_len, int flags, void* userdata)
{
    rtw_up_sema(&wm_discon_sema);
    printf("\n\rWifi disconnect!!\n");
    return;
}

void example_wifi_manager(void)
{
    if(xTaskCreate(wifi_manager_thread, ((const char*)"wifi_manager_thread"), 1024, NULL, tskIDLE_PRIORITY , NULL) != pdPASS)
        printf("\n\r%s xTaskCreate(wifi_manager_thread) failed", __FUNCTION__);

    return;
}

