#ifndef BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_RECORDER_H
#define BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_RECORDER_H

#include "FreeRTOS.h"
#include "task.h"
#include "section_config.h"
#include "osdep_service.h"
#include <platform_stdlib.h>

#include "dma_api.h"
#include "gpio_api.h"
#include "gpio_irq_api.h"
#include "timer_api.h"
#include "duerapp_config.h"
#include "queue.h"

#define RECORD_LEN_SEC 8
#define RECORD_LEN (32*1000*RECORD_LEN_SEC)
#define LED_TIME_PERIOD 500000  //500MS
#define led_off 1
#define led_on  0
#define RECORD_DELAY(_x)    ((_x) * 20 * 10000 / RECORD_FARME_SIZE / 10000)

typedef struct record_s{
	u32 skip_pages;

	long int bkg_avg_vol;
	int volume_max;
	u8 silent_count;
	u8 is_recording;

	long int total_cmd_vol;
	long int cmd_vol_count;

	u32 audio_record_start_timestamp;
	u32 audio_record_timeout;

	u32 record_length;
	u32 i2s_rx_history_idx;
}record_t;


typedef struct{
	int dir;
	int size;
	unsigned int val;
	//snd_pcm_t *handle;
	//snd_pcm_uframes_t frames;
	//snd_pcm_hw_params_t *params;
}duer_rec_config_t;

int duer_recorder_start();
int duer_recorder_stop();

#endif // BAIDU_DUER_LIBDUER_DEVICE_EXAMPLES_DCS3_LINUX_DUERAPP_RECORDER_H
