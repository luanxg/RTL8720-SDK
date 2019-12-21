 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "example_media_framework.h"

void mmf2_example_v3_init(void)
{
	isp_v3_ctx = mm_module_open(&isp_module);
	if(isp_v3_ctx){
		mm_module_ctrl(isp_v3_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v3_params);
		mm_module_ctrl(isp_v3_ctx, MM_CMD_SET_QUEUE_LEN, V3_SW_SLOT);
		mm_module_ctrl(isp_v3_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(isp_v3_ctx, CMD_ISP_APPLY, 2);	// start channel 1
	}else{
		rt_printf("ISP open fail\n\r");
		goto mmf2_exmaple_v3_fail;
	}
	
	jpeg_v3_ctx = mm_module_open(&jpeg_module);
	if(jpeg_v3_ctx){
		mm_module_ctrl(jpeg_v3_ctx, CMD_JPEG_SET_PARAMS, (int)&jpeg_v3_params);
		mm_module_ctrl(jpeg_v3_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		mm_module_ctrl(jpeg_v3_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_DYNAMIC);
		mm_module_ctrl(jpeg_v3_ctx, CMD_JPEG_INIT_MEM_POOL, 0);
		mm_module_ctrl(jpeg_v3_ctx, CMD_JPEG_APPLY, 0);
	}else{
		rt_printf("JPEG open fail\n\r");
		goto mmf2_exmaple_v3_fail;
	}
	
	rtsp2_v3_ctx = mm_module_open(&rtsp2_module);
	if(rtsp2_v3_ctx){
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SELECT_STREAM, 0);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_PARAMS, (int)&rtsp2_v3_params);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_APPLY, 0);
		mm_module_ctrl(rtsp2_v3_ctx, CMD_RTSP2_SET_STREAMMING, ON);
		//mm_module_ctrl(rtsp2_v3_ctx, MM_CMD_SET_QUEUE_LEN, 3);
		//mm_module_ctrl(rtsp2_v3_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
	}else{
		rt_printf("RTSP2 open fail\n\r");
		goto mmf2_exmaple_v3_fail;
	}
	
	siso_isp_jpeg_v3 = siso_create();
	if(siso_isp_jpeg_v3){
		siso_ctrl(siso_isp_jpeg_v3, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v3_ctx, 0);
		siso_ctrl(siso_isp_jpeg_v3, MMIC_CMD_ADD_OUTPUT, (uint32_t)jpeg_v3_ctx, 0);
		siso_start(siso_isp_jpeg_v3);
	}else{
		rt_printf("siso3 open fail\n\r");
		goto mmf2_exmaple_v3_fail;
	}
	
	siso_jpeg_rtsp_v3 = siso_create();
	if(siso_jpeg_rtsp_v3){
		siso_ctrl(siso_jpeg_rtsp_v3, MMIC_CMD_ADD_INPUT, (uint32_t)jpeg_v3_ctx, 0);
		siso_ctrl(siso_jpeg_rtsp_v3, MMIC_CMD_ADD_OUTPUT, (uint32_t)rtsp2_v3_ctx, 0);
		siso_start(siso_jpeg_rtsp_v3);
	}else{
		rt_printf("siso4 open fail\n\r");
		goto mmf2_exmaple_v3_fail;
	}
	
	return;
mmf2_exmaple_v3_fail:
	
	return;
}