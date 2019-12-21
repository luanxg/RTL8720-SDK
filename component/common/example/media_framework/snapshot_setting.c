 /******************************************************************************
 *
 * Copyright(c) 2007 - 2018 Realtek Corporation. All rights reserved.
 *
 ******************************************************************************/
#include "example_media_framework.h"

#if ENABLE_SNAPSHOT
u8* snapshot_buffer = NULL;
#endif

#if ENABLE_SNAPSHOT
void snapshot_setting()
{
	snapshot_buffer = malloc(SNAPSHOT_BUFFER_SIZE);
	if(snapshot_buffer == NULL){
		printf("malloc snapshot_buffer fail\r\n");
		while(1);
	}
#if ENABLE_V1_SNAPSHOT
	mm_module_ctrl(h264_v1_ctx,CMD_SNAPSHOT_ENCODE_CB, (int)jpeg_snapshot_encode_cb);
#elif ENABLE_V3_SNAPSHOT_WHEN_STREAMING
	mm_module_ctrl(jpeg_v3_ctx, CMD_SNAPSHOT_CB, (int)jpeg_snapshot_cb);
#endif

#if ENABLE_V3_SNAPSHOT_WHEN_STREAMING
	void* jpeg_encoder = NULL;
	mm_module_ctrl(jpeg_v3_ctx, CMD_JPEG_GET_ENCODER, (int)&jpeg_encoder);
	jpeg_snapshot_initial_with_instance(jpeg_encoder, (u32)snapshot_buffer, SNAPSHOT_BUFFER_SIZE,1);
#else
	jpeg_snapshot_initial(SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, SNAPSHOT_FPS, SNAPSHOT_LEVEL, (u32)snapshot_buffer, SNAPSHOT_BUFFER_SIZE);
#endif

#if ENABLE_V3_JPEG == V3_JPEG_SNAPSHOT
	jpeg_snapshot_isp_config(2);
	printf("Video 3 JPEG_LEVEL %d, ISP_STREAMID 2 (SNAPSHOT MODE)\n\r",V3_JPEG_LEVEL);
#endif
#ifdef SNAPSHOT_TFTP_TYPE
	jpeg_snapshot_create_tftp_thread();
#else
	jpeg_snapshot_create_sd_thread();
#endif
}
#endif