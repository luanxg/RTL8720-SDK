#ifndef EXAMPLE_MEDIA_UVCD_H
#define EXAMPLE_MEDIA_UVCD_H

#include "video_common_api.h"

#define UVCD_TUNNING_MODE  1// 1 :Contain four formats to support tunning tool (YUV2 NV12 H264 JPEG)
                            // 0 :Contain two formats to support h264 and jpeg format
#define CUSTOMER_AMEBAPRO_ADVANCE 0 //1: Based on the defined "UVCD_TUNNING_MODE", this option "1" is for customer release, "0" is for internal use.
void example_media_uvcd(void);

#endif /* MMF2_EXAMPLE_H */