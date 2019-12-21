#ifndef DUERAPP_AUDIO_H
#define DUERAPP_AUDIO_H
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


#define AUDIO_MAX_DMA_PAGE_SIZE	4608


#define LED_TIME_PERIOD 500000  //500MS
#define led_off 1
#define led_on  0


#define RECORD_DELAY(_x)    ((_x) * 20 * 10000 / RECORD_FARME_SIZE / 10000)
#define AUDIO_RX_IDLE             (0)
#define AUDIO_RX_EARLY_PROCESSING (1)
#define AUDIO_RX_PROCESSING       (2)
#define AUDIO_RX_END              (3)




typedef struct player_s{
    int nb_channels;
    int sample_rate;
	int last_sr;
	int frame_size;
    int bit_rate;

	int audio_bytes;
}player_t;

typedef enum {
	TONE_HELLO = 0,
#ifdef MEMORY_SAVE_TODO
	TONE_CANNOTPLAY = 1,
	TONE_CANNOTHEARD = 2,
#endif
	//TONE_DUERSTOPPED = 3,
	TONE_MAX,
}Tones;

enum _duer_http_error {
	DUER_EXIT_NO_ERROR = 0,
    DUER_EXIT_PAUSE,
    DUER_EXIT_STOP,
	DUER_EXIT_GET_HOST_URL_FAILED,
	DUER_EXIT_MALLOC_FAILED,
	DUER_EXIT_CREATE_SOCKET_FAILED,
	DUER_EXIT_CONNECT_SERVER_FAILED,
	DUER_EXIT_SEND_REQUEST_FAILED,
	DUER_EXIT_GET_HOST_BY_NAME_FAILED,
	DUER_EXIT_CONNECT_FAILED,
	DUER_EXIT_POST_DATA_FAILED,
	DUER_EXIT_READ_DATA_FAILED_OR_CLOSE,
	DUER_EXIT_HTTP_HEADER_PARSE_FAILED,
	DUER_EXIT_MP3_FORMAT_UNSUPPORT,
	DUER_ERROR_CANNOT_FIND_FIRST_FRAME
};


extern int is_audio_need_pause(void);
extern void audio_play_pause_resume(void);
extern void initialize_audio(void);
#endif
