#ifndef EXAMPLE_MEDIA_H264_TO_SDCARD_H
#define EXAMPLE_MEDIA_H264_TO_SDCARD_H

#include "h264_api.h"

struct h264_to_sd_card_def_setting {
	uint32_t height;
	uint32_t width;
	H264_RC_MODE rcMode;
	uint32_t bitrate;
	uint32_t fps;
	uint32_t gopLen;
	uint32_t encode_frame_cnt;
	uint32_t output_buffer_size;
	uint8_t isp_stream_id;
	uint32_t isp_hw_slot;
	uint32_t isp_format;
	char fatfs_filename[64];
};

void example_media_h264_to_sdcard(void);

#endif /* EXAMPLE_MEDIA_H264_TO_SDCARD_H */
