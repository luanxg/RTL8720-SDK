// Copyright (2017) Baidu Inc. All rights reserveed.
/**
 * File: duerapp_config.h
 * Auth: Su Hao (suhao@baidu.com)
 * Desc: Duer Configuration.
 */

#ifndef BAIDU_DUER_DUERAPP_DUERAPP_CONFIG_H
#define BAIDU_DUER_DUERAPP_DUERAPP_CONFIG_H

#include "platform_stdlib.h"

#define SDCARD_PATH         "0:/"
#define RECORD_SAMPLE_RATE  (16000)
#define RECORD_FARME_SIZE   (RECORD_SAMPLE_RATE / 50 * 2)

#define SDCARD_RECORD  0

#define DUER_PLATFORM_DEBUG
#if defined(DUER_PLATFORM_DEBUG)
#define DUER_DEBUG_LEVEL	DUER_INFO
enum{
	DUER_ERROR = 0,
	DUER_WARNNING,
	DUER_INFO,
	DUER_DEBUG,
	DUER_DUMP,
};

#define DUER_TAG            "duerapp: "
#define DUER_PRINTF(level, fmt, arg...)     \
do {\
	if (level <= DUER_DEBUG_LEVEL) {\
		printf("\r\n"DUER_TAG fmt, ##arg);\
	}\
}while(0)

#define DUER_LOGV(...)      DUER_PRINTF(DUER_DUMP, __VA_ARGS__)
#define DUER_LOGD(...)      DUER_PRINTF(DUER_DEBUG, __VA_ARGS__)
#define DUER_LOGI(...)      DUER_PRINTF(DUER_INFO, __VA_ARGS__)
#define DUER_LOGW(...)      DUER_PRINTF(DUER_WARNNING, __VA_ARGS__)
#define DUER_LOGE(...)      DUER_PRINTF(DUER_ERROR, __VA_ARGS__)

//http log
#define HTTP_DEBUG_LEVEL	DUER_INFO
#define HTTP_PRINTF(level, fmt, arg...)     \
do {\
	if (level <= HTTP_DEBUG_LEVEL) {\
		printf("\r\n"DUER_TAG fmt, ##arg);\
	}\
}while(0)


#define HTTP_LOGV(...)      HTTP_PRINTF(DUER_DUMP, __VA_ARGS__)
#define HTTP_LOGD(...)      HTTP_PRINTF(DUER_DEBUG, __VA_ARGS__)
#define HTTP_LOGI(...)      HTTP_PRINTF(DUER_INFO, __VA_ARGS__)
#define HTTP_LOGW(...)      HTTP_PRINTF(DUER_WARNNING, __VA_ARGS__)
#define HTTP_LOGE(...)      HTTP_PRINTF(DUER_ERROR, __VA_ARGS__)

#else

#define DUER_LOGV(...)
#define DUER_LOGD(...)
#define DUER_LOGI(...)
#define DUER_LOGW(...)
#define DUER_LOGE(...)

#define HTTP_LOGV(...)
#define HTTP_LOGD(...)
#define HTTP_LOGI(...)
#define HTTP_LOGW(...)
#define HTTP_LOGE(...)


#endif

void initialize_wifi(void);

int initialize_sdcard(void);
int finalize_sdcard(void);

const char *duer_load_profile(const char *path);

/*
 * Recorder APIS
 */

enum duer_record_events_enum {
    REC_START,
    REC_DATA,
    REC_STOP
};

typedef enum
{
	DUER_LED_ON = 0,
	DUER_LED_OFF = 1,
	DUER_LED_FAST_TWINKLE_150 = 2,
	DUER_LED_FAST_TWINKLE_300 = 3,
	DUER_LED_SLOW_TWINKLE = 4,
}duer_led_mode_t;

typedef void (*duer_record_event_func)(int status, const void *data, size_t size);

void duer_record_set_event_callback(duer_record_event_func func);

void duer_record_all();

void duerapp_gpio_led_mode(duer_led_mode_t mode);

#endif/*BAIDU_DUER_DUERAPP_DUERAPP_CONFIG_H*/
