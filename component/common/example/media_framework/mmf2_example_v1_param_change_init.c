 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "example_media_framework.h"
static void change_resolution_parameter(void *parm)
{
        //Change the resolution from 1080p to 720p
        isp_v1_params.width = 1920;
        isp_v1_params.height = 1080;
        isp_v2_params.width = 1280;
        isp_v2_params.height = 720;
        h264_v1_params.width = 1920;
        h264_v1_params.height = 1080;
        h264_v2_params.width = 1280;
        h264_v2_params.height = 720;
}
void mmf2_example_v1_param_change_init(void)
{
        int sw = 0;
        change_resolution_parameter(NULL);
	isp_v1_ctx = mm_module_open(&isp_module);
        
	if(isp_v1_ctx){
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v1_params);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_SW_SLOT);
		mm_module_ctrl(isp_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_APPLY, 0);	// start channel 0
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_exmaple_v1_param_change_fail;
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
		goto mmf2_exmaple_v1_param_change_fail;
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
		goto mmf2_exmaple_v1_param_change_fail;
	}
	
	siso_isp_h264_v1 = siso_create();
	if(siso_isp_h264_v1){
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v1_ctx, 0);
		siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v1_ctx, 0);
		siso_start(siso_isp_h264_v1);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_v1_param_change_fail;
	}

	siso_h264_rtsp_v1 = siso_create();
	if(siso_h264_rtsp_v1){
		siso_ctrl(siso_h264_rtsp_v1, MMIC_CMD_ADD_INPUT, (uint32_t)h264_v1_ctx, 0);
		siso_ctrl(siso_h264_rtsp_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)rtsp2_v1_ctx, 0);
		siso_start(siso_h264_rtsp_v1);
	}else{
		rt_printf("siso2 open fail\n\r");
		goto mmf2_exmaple_v1_param_change_fail;
	}

	rt_printf("changing resolution and fps test\n\r");
	for(int i=0;i<10;i++){
		sw++;
		sw &= 1;
		// wait 10 seconds, change resolution
		vTaskDelay(10000);
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_STREAM_STOP, 0);
		siso_pause(siso_isp_h264_v1);
		siso_pause(siso_h264_rtsp_v1);
		
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_PARAMS,(sw==1)?(int)&isp_v2_params:(int)&isp_v1_params);
		mm_module_ctrl(isp_v1_ctx, CMD_ISP_UPDATE, 0);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_SET_PARAMS, (sw==1)?(int)&h264_v2_params:(int)&h264_v1_params);
		mm_module_ctrl(h264_v1_ctx, CMD_H264_UPDATE, 0);
		
		siso_resume(siso_isp_h264_v1);
		siso_resume(siso_h264_rtsp_v1);
	}
	
	rt_printf("changing rate control mode test\n\r");
	mm_module_ctrl(h264_v1_ctx, CMD_H264_BITRATE, 500*1000);
	for(int i=0;i<10;i++){
		sw++;
		sw &= 1;
		// wait 10 seconds, change resolution
		vTaskDelay(5000);

		rt_printf("RCMODE : %s\n\r", (sw==1)?"CBR":"VBR");
		mm_module_ctrl(h264_v1_ctx, CMD_H264_RCMODE, (sw==1)?(int)RC_MODE_H264CBR:RC_MODE_H264VBR);
	}	
	
	rt_printf("changing bit rate test\n\r");
	for(int i=0;i<10;i++){
		sw++;
		sw &= 1;
		// wait 10 seconds, change resolution
		vTaskDelay(5000);

		mm_module_ctrl(h264_v1_ctx, CMD_H264_BITRATE, (1000+100*i)*1000);
	}
	
	mm_module_ctrl(h264_v1_ctx, CMD_H264_FORCE_IFRAME, 0);
	vTaskDelay(10);
	mm_module_ctrl(h264_v1_ctx, CMD_H264_FORCE_IFRAME, 0);
	vTaskDelay(10);
	mm_module_ctrl(h264_v1_ctx, CMD_H264_FORCE_IFRAME, 0);
	
	return;
mmf2_exmaple_v1_param_change_fail:
	
	return;
}