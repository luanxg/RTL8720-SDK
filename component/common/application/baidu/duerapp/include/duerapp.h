#ifndef BAIDU_DUER_DUERAPP_H
#define BAIDU_DUER_DUERAPP_H

#include <FreeRTOS.h>
#include "semphr.h"
#include "stdbool.h"

#ifdef	CONFIG_PLATFORM_8195A
#ifndef SDRAM_BSS_SECTION
#define SDRAM_BSS_SECTION	SECTION(".sdram.bss")
#define SAFE_FREE(x)	{if (x)	DUER_FREE(x);	x = NULL;}	/* helper macro */
#define DUER_MALLOC_SDRAM(_s) pvPortMallocByType(_s,2)
#endif
#define SDRAM_DATA_SECTION	SECTION(".sdram.data")
#else
#define SDRAM_BSS_SECTION
#define SDRAM_DATA_SECTION
#define DUER_MALLOC_SDRAM(_s) pvPortMalloc(_s)
#define SAFE_FREE(x)	{if (x)	vPortFree(x);	x = NULL;}	/* helper macro */
#endif

typedef enum{
	RECORDER_START,
	RECORDER_STOP,
}duer_rec_state_t;

typedef struct
{
	bool started;
	bool mute;
	bool muteResetFlag;
	bool is_resume_play;
	bool duerStopByPlayThread;
	bool duerPlayStuttered;
	
	uint8_t volume;
	uint8_t voice_is_triggered;  // in voice triggered and playing tone is 1, when recorder started set to 0
 	uint8_t audio_is_playing;    // http audio is playing
	uint8_t audio_is_pocessing; 	// in recording set to 1
	uint8_t duer_play_stop_flag; // media stop flag in state audio_is_playing
	uint8_t stop_by_voice_irq;   // stop by voice irq but not button
	uint8_t audio_has_inited;

	duer_rec_state_t duer_rec_state;
	SemaphoreHandle_t xSemaphoreHTTPAudioThread;
	SemaphoreHandle_t xSemaphoreMediaPlayDone;
 	SemaphoreHandle_t xSemaphoreHttpDownloadDone;
} T_APP_DUER;

#define MIN(x, y) ((x) > (y) ? (y) : (x))


void initialize_duerapp();
#endif

