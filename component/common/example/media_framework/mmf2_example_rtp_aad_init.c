 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/

#include "platform_opts.h"
#include "module_isp.h"
#include "module_h264.h"
#include "module_jpeg.h"
#include "module_rtsp2.h"
#include "module_mp4.h"
#include "module_audio.h"
#include "module_aac.h"
#include "module_aad.h"
#include "module_g711.h"
#include "module_rtp.h"
#include "module_array.h"

#include "mmf2_link.h"
#include "mmf2_siso.h"
#include "mmf2_simo.h"
#include "mmf2_miso.h"
#include "mmf2_mimo.h"
#include "example_media_framework.h"

#include "video_common_api.h"
#include "sensor_service.h"
#include "jpeg_snapshot.h"
#include "snapshot_tftp_handler.h"
#include "sample_doorbell_pcmu.h"
#include "sample_pcmu.h"
#include "sample_aac.h"
#include "sample_h264.h"
#include "isp_boot.h"
#include "sensor.h"

#include "platform_autoconf.h"

void mmf2_example_rtp_aad_init(void)
{
	rtp_ctx = mm_module_open(&rtp_module);
	if(rtp_ctx){
		mm_module_ctrl(rtp_ctx, CMD_RTP_SET_PARAMS, (int)&rtp_aad_params);
		mm_module_ctrl(rtp_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(rtp_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(rtp_ctx, CMD_RTP_APPLY, 0);
		mm_module_ctrl(rtp_ctx, CMD_RTP_STREAMING, 1);	// streamming on
	}else{
		rt_printf("RTP open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}

	aad_ctx = mm_module_open(&aad_module);
	if(aad_ctx){
		mm_module_ctrl(aad_ctx, CMD_AAD_SET_PARAMS, (int)&aad_rtp_params);
		mm_module_ctrl(aad_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		mm_module_ctrl(aad_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(aad_ctx, CMD_AAD_APPLY, 0);
	}else{
		rt_printf("AAD open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}
	
	audio_ctx = mm_module_open(&audio_module);
	if(audio_ctx){
		mm_module_ctrl(audio_ctx, CMD_AUDIO_SET_PARAMS, (int)&audio_params);
		//mm_module_ctrl(audio_ctx, MM_CMD_SET_QUEUE_LEN, 6);
		//mm_module_ctrl(audio_ctx, MM_CMD_INIT_QUEUE_ITEMS, MMQI_FLAG_STATIC);
		mm_module_ctrl(audio_ctx, CMD_AUDIO_APPLY, 0);
	}else{
		rt_printf("audio open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}
	
	siso_rtp_aad = siso_create();
	if(siso_rtp_aad){
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_INPUT, (uint32_t)rtp_ctx, 0);
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_OUTPUT, (uint32_t)aad_ctx, 0);
		siso_start(siso_rtp_aad);
	}else{
		rt_printf("siso1 open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}
	
	rt_printf("siso1 started\n\r");

	siso_rtp_aad = siso_create();
	if(siso_rtp_aad){
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_INPUT, (uint32_t)aad_ctx, 0);
		siso_ctrl(siso_rtp_aad, MMIC_CMD_ADD_OUTPUT, (uint32_t)audio_ctx, 0);
		siso_start(siso_rtp_aad);
	}else{
		rt_printf("siso2 open fail\n\r");
		goto mmf2_exmaple_rtp_aad_fail;
	}
	
	rt_printf("siso2 started\n\r");

	return;
mmf2_exmaple_rtp_aad_fail:
	
	return;

}