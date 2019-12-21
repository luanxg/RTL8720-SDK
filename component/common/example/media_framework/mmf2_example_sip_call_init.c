/******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "example_media_framework.h"

#define ENABLE_SIP_VIDEO

#ifdef ENABLE_SIP_MMFV2

// Target Uniform Resource Identifier (URI)
#define To_URI    "sip:qiudanqing@192.168.0.100:5060"
//#define To_URI    "sip:amebatest01@sip.linphone.org:5060"
#define My_URI "<sip:Ameba@192.168.0.102:5060>;audio_codecs=PCMU/8000/1,PCMA/8000/1"



void mmf2_example_sip_call_init(void)
{
		
	//#ifdef ENABLE_SIP_VIDEO
		printf("\r\nmmf2_example_sip_call_init");
	
		// ------ Channel 1 video--------------
		isp_v1_ctx = mm_module_open(&isp_module);
		if(isp_v1_ctx){
			mm_module_ctrl(isp_v1_ctx, CMD_ISP_SET_PARAMS, (int)&isp_v1_params);
			mm_module_ctrl(isp_v1_ctx, MM_CMD_SET_QUEUE_LEN, V1_SW_SLOT);
			mm_module_ctrl(isp_v1_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
			mm_module_ctrl(isp_v1_ctx, CMD_ISP_APPLY, 0);	// start channel 0
		}else{
			rt_printf("ISP open fail\n\r");
			goto mmf2_exmaple_ua_fail;
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
			goto mmf2_exmaple_ua_fail;
		}
	//#endif
	
		audio_ctx = mm_module_open(&audio_module);
		if(audio_ctx){
			
			mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
			mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
			mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
			mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
		}else{
			rt_printf("audio open fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
	
		g711e_ctx = mm_module_open(&g711_module);
		if(g711e_ctx){
			mm_module_ctrl(g711e_ctx, CMD_G711_SET_PARAMS, (int)&g711e_params);
			mm_module_ctrl(g711e_ctx, MM_CMD_SET_QUEUE_LEN, 6);
			mm_module_ctrl(g711e_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
			mm_module_ctrl(g711e_ctx, CMD_G711_APPLY, 0);
		
		}else{
			rt_printf("G711 open fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
	
		
		ua2_ctx = mm_module_open(&ua2_module);
		if(ua2_ctx){
			mm_module_ctrl(ua2_ctx, CMD_UA2_SET_TO_URI, (int)To_URI);		
			mm_module_ctrl(ua2_ctx, CMD_UA2_SET_FROM_URI, (int)My_URI);
			mm_module_ctrl(ua2_ctx, CMD_UA2_SELECT_STREAM, 0);
			mm_module_ctrl(ua2_ctx, CMD_UA2_SET_PARAMS, (int)&ua2_v_params);
			mm_module_ctrl(ua2_ctx, CMD_UA2_SET_APPLY, 0);
		
			mm_module_ctrl(ua2_ctx, CMD_UA2_SELECT_STREAM, 1);
			mm_module_ctrl(ua2_ctx, CMD_UA2_SET_PARAMS, (int)&ua2_a_params);
			mm_module_ctrl(ua2_ctx, CMD_UA2_SET_APPLY, 0);
			
			
			mm_module_ctrl(ua2_ctx, CMD_UA2_SET_STREAMMING, ON);
			
		}else{
			rt_printf("UA2 open fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
		
		
	
		//--------------Link video h264 and audio ---------------------------
		siso_isp_h264_v1 = siso_create();
		if(siso_isp_h264_v1){
			siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_INPUT, (uint32_t)isp_v1_ctx, 0);
			siso_ctrl(siso_isp_h264_v1, MMIC_CMD_ADD_OUTPUT, (uint32_t)h264_v1_ctx, 0);
			siso_start(siso_isp_h264_v1);
		}else{
			rt_printf("siso_isp_h264_v1 open fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
		
		
	
	
		siso_audio_g711 = siso_create();
		if(siso_audio_g711){
			siso_ctrl(siso_audio_g711, MMIC_CMD_ADD_INPUT, (uint32_t)audio_ctx, 0);
			siso_ctrl(siso_audio_g711, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711e_ctx, 0);
			siso_start(siso_audio_g711);
		}else{
			rt_printf("siso1 open fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
		
		
	
	
		miso_h264_g711_ua = miso_create();
		if(miso_h264_g711_ua){
			miso_ctrl(miso_h264_g711_ua, MMIC_CMD_ADD_INPUT0, (uint32_t)h264_v1_ctx, 0);
			miso_ctrl(miso_h264_g711_ua, MMIC_CMD_ADD_INPUT1, (uint32_t)g711e_ctx, 0);
			miso_ctrl(miso_h264_g711_ua, MMIC_CMD_ADD_OUTPUT, (uint32_t)ua2_ctx, 0);
			miso_start(miso_h264_g711_ua);
		}else{
			rt_printf("miso_h264_g711_rtsp fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
		
		
		
		rtp_ctx = mm_module_open(&rtp_module);
		if(rtp_ctx){
			mm_module_ctrl(rtp_ctx, CMD_RTP_SET_PARAMS, (int)&rtp_g711d_params);
			mm_module_ctrl(rtp_ctx, MM_CMD_SET_QUEUE_LEN, 6);
			mm_module_ctrl(rtp_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
			mm_module_ctrl(rtp_ctx, CMD_RTP_APPLY, 0);
			mm_module_ctrl(rtp_ctx, CMD_RTP_STREAMING, 1);	// streamming on
		}else{
			rt_printf("RTP open fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
		
		// G711D
		g711d_ctx = mm_module_open(&g711_module);
		if(g711d_ctx){
			mm_module_ctrl(g711d_ctx, CMD_G711_SET_PARAMS, (int)&g711d_params);
			mm_module_ctrl(g711d_ctx, MM_CMD_SET_QUEUE_LEN, 6);
			mm_module_ctrl(g711d_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
			mm_module_ctrl(g711d_ctx, CMD_G711_APPLY, 0);
		}else{
			rt_printf("G711 open fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
	
		siso_rtp_g711d = siso_create();
		if(siso_rtp_g711d){
			siso_ctrl(siso_rtp_g711d, MMIC_CMD_ADD_INPUT, (uint32_t)rtp_ctx, 0);
			siso_ctrl(siso_rtp_g711d, MMIC_CMD_ADD_OUTPUT, (uint32_t)g711d_ctx, 0);
			siso_start(siso_rtp_g711d);
		}else{
			rt_printf("siso1 open fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
		
		
	
		siso_g711d_audio = siso_create();
		if(siso_g711d_audio){
			siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_INPUT, (uint32_t)g711d_ctx, 0);
			siso_ctrl(siso_g711d_audio, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
			siso_start(siso_g711d_audio);
		}else{
			rt_printf("siso2 open fail\n\r");
			goto mmf2_exmaple_ua_fail;
		}
		
		
	
	
		
		return;
	mmf2_exmaple_ua_fail:
		
		return;
	}


#endif

