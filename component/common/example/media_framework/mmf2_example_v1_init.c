 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "example_media_framework.h"

#if ISP_BOOT_MODE_ENABLE
extern isp_boot_cfg_t isp_boot_cfg_global;
extern void common_init();
#define VIDEO_RTSP_QUEUE_LEN	300 //It the buffer is bigger enought that it will keep 300 frame.
int mmf_stop = 0;
int mmf_suspend(void *arg)
{
	//mm_module_ctrl(rtsp2_v2_ctx, CMD_RTSP2_SET_BLOCK_TYPE, RTSP2_NON_BLOCK_TYPE);
	//mm_module_ctrl(rtsp2_v2_ctx,CMD_RTSP2_SET_STREAMMING,OFF);
	mmf_stop = 1;
}
int mmf_start(void *arg)
{
	
}

#include "wifi_structures.h"
void wifi_sleep_func()
{
	wowlan_pattern_t test_pattern;
	memset(&test_pattern,0,sizeof(wowlan_pattern_t));
	
	char buf[32];
	char mac[6];
	char ip_protocol[2] = {0x08, 0x06};
	u8 arp_mask[5] = {0x3f, 0x70, 0x00, 0x00, 0x00};
	
	// MAC DA
	wifi_get_mac_address(buf);
	sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	rtw_memcpy(test_pattern.eth_da, mac, 6);
	// IP
	rtw_memcpy(test_pattern.eth_proto_type, ip_protocol, 2);
	// ARP mask
	rtw_memcpy(test_pattern.mask, arp_mask, 5);
	
	wifi_wowlan_set_pattern(test_pattern);

}
void mmf2_example_v1_init(void)
{
	int i = 0;
	extern unsigned int GlobalDebugEnable;
	GlobalDebugEnable = 0;//Disable the wifi debug info to spped the wifi boot time
	isp_v1_ctx = mm_module_open(&isp_module);
        isp_v1_params.boot_mode = ISP_FAST_BOOT;
	if(isp_v1_ctx){
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v1_params);
		for(i=0;i<isp_boot_cfg_global.isp_config.hw_slot_num;i++)
			mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_SELF_BUF, isp_boot_cfg_global.isp_buffer[i]);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_SW_SLOT);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_APPLY, 0);	// start channel 0
                isp_v1_params.boot_mode = ISP_NORMAL_BOOT;
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}

	h264_v1_ctx = mm_module_open(&h264_module);
	if(h264_v1_ctx){
		mm_module_ctrl(h264_v1_ctx, CMD_H264_SET_PARAMS, (int)&h264_v1_params);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_SET_QUEUE_LEN, VIDEO_RTSP_QUEUE_LEN);//V2_H264_QUEUE_LEN);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_INIT_MEM_POOL, 0);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_APPLY, 0);
	}else{
		rt_printf("H264 open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}
	
	siso_isp_h264_v1 = siso_create();
	if(siso_isp_h264_v1){
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v1_ctx, 0);
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v1_ctx, 0);
		siso_start(siso_isp_h264_v1);
	}else{
		rt_printf("siso_isp_h264_v1 open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}
	
	//vTaskDelay(200);
	pre_example_entry();
	
#if defined(CONFIG_WIFI_NORMAL) && defined(CONFIG_NETWORK)
	wlan_network();
#endif
	common_init();
		
#if V1_LOW_POWER_ACTIVE_MODE
	wifi_set_lps_thresh(1);
#endif
	
	rtsp2_v1_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v1_ctx){
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v1_params);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_APPLY, 0);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_DROP_TIME, 12*60*60*1000);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_BLOCK_TYPE, RTSP2_BLOCK_TYPE);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_SYNC_MODE, 0);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_v2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_STOP_CB, (int)mmf_suspend);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_START_CB, (int)mmf_start);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}

	siso_h264_rtsp_v1 = siso_create();
	if(siso_h264_rtsp_v1){
		siso_ctrl(siso_h264_rtsp_v1, MMIC_CMD_ADD_INPUT, (uint32_t)h264_v1_ctx, 0);
		siso_ctrl(siso_h264_rtsp_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)rtsp2_v1_ctx, 0);
		siso_start(siso_h264_rtsp_v1);
	}else{
		rt_printf("siso4 open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}
	
	while(1)
	{
		//For power saving mode
		if(mmf_stop){
			mmf_stop = 0;

			printf("disable the video sub system\r\n");
			mm_module_ctrl(isp_v1_ctx, CMD_ISP_STREAM_STOP, 0);
			//printf("stop siso_isp_h264_v2\r\n");
			//siso_stop(siso_isp_h264_v2);
			siso_stop(siso_isp_h264_v1);
			//printf("stop siso_h264_rtsp_v2\r\n");
			
			//siso_stop(siso_h264_rtsp_v2);
			//printf("stop finish\r\n");
			video_subsys_deinit(NULL);
			//wifi_sleep_func();
			rtl8195b_suspend(0);
			printf("\r\nSleepPG\r\n");
			SleepPG(SLP_WLAN, 0, 1, 0);
		}
		vTaskDelay(100);
	}

	return;
mmf2_exmaple_v1_fail:
	
	return;
}
#else
void mmf2_example_v1_init(void)
{
	isp_v1_ctx = mm_module_open(&isp_module);
	if(isp_v1_ctx){
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v1_params);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_SW_SLOT);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_APPLY, 0);	// start channel 0
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}

	h264_v1_ctx = mm_module_open(&h264_module);
	if(h264_v1_ctx){
		mm_module_ctrl(h264_v1_ctx, CMD_H264_SET_PARAMS, (int)&h264_v1_params);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_H264_QUEUE_LEN);
		mm_module_ctrl(h264_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_INIT_MEM_POOL, 0);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_APPLY, 0);
	}else{
		rt_printf("H264 open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}
	
	rtsp2_v1_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v1_ctx){
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v1_params);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_APPLY, 0);
		mm_module_ctrl(rtsp2_v1_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}
	
	siso_isp_h264_v1 = siso_create();
	if(siso_isp_h264_v1){
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v1_ctx, 0);
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v1_ctx, 0);
		siso_start(siso_isp_h264_v1);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}

	siso_h264_rtsp_v1 = siso_create();
	if(siso_h264_rtsp_v1){
		siso_ctrl(siso_h264_rtsp_v1, MMIC_CMD_ADD_INPUT, (uint32_t)h264_v1_ctx, 0);
		siso_ctrl(siso_h264_rtsp_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)rtsp2_v1_ctx, 0);
		siso_start(siso_h264_rtsp_v1);
	}else{
		rt_printf("siso2 open fail\n\r");
		goto mmf2_exmaple_v1_fail;
	}

	return;
mmf2_exmaple_v1_fail:
	
	return;
}
#endif