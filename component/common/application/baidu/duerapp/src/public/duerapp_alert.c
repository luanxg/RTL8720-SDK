// Copyright (2017) Baidu Inc. All rights reserved.
/**
 * File: duerapp_alarm.c
 * Auth: Gang Chen(chengang12@baidu.com)
 * Desc: Duer alarm functions.
 */
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "lightduer_dcs.h"
#include "lightduer_memory.h"
#include "lightduer_dcs_alert.h"

#include "duerapp.h"
#include "duerapp_config.h"
#include "lightduer_net_ntp.h"
#include "duerapp_media.h"
#include "duerapp_audio.h"
#include "lightduer_types.h"
#include "lightduer_timers.h"
#include "lightduer_mutex.h"
#include "duerapp_alert.h"
#include "lightduer_coap_defs.h"

typedef struct _duerapp_alert_node {
	char *token;
	char *type;
	char *time;
	bool isbell;
	duer_timer_handler handle;
} duerapp_alert_node;

typedef struct _duer_alert_list {
	struct _duer_alert_list *next;
	duerapp_alert_node *data;
} duer_alert_list_t;
 

static duer_alert_list_t s_alert_head;
static duer_mutex_t s_alert_mutex = NULL;
static duer_mutex_t s_bell_mutex = NULL;
//static char *s_ring_path = NULL;
static bool s_is_bell = false;

static char *s_ring_path = "http://other.web.rf01.sycdn.kuwo.cn/resource/n2/0/28/2423995397.mp3";
SDRAM_BSS_SECTION static duer_media_param_t alert_param = {0};

extern T_APP_DUER *pDuerAppInfo;

extern void duer_http_audio_thread(void *param);

bool duer_alert_bell()
{
	return s_is_bell;
}


void duer_set_alert_ring(char *ring)
{
	s_ring_path = ring;
}

char *duer_get_alert_ring()
{
	return s_ring_path;
}

static void duer_alert_start(const char *token)
{
	size_t len = 0;
	
	if (!s_ring_path) {
		DUER_LOGE ("not found mp3 path!");
		return;
	}
	duer_media_audio_pause();
	duer_media_speak_stop();

	while (pDuerAppInfo->audio_is_playing) {
		duer_sleep(10);
	}

	if (pDuerAppInfo->voice_is_triggered || pDuerAppInfo->audio_is_pocessing) {
		DUER_LOGW("(voice_irq %d,playing %d,recording %d)",
		          pDuerAppInfo->voice_is_triggered, pDuerAppInfo->audio_is_playing, pDuerAppInfo->audio_is_pocessing);
		return;
	}

	memset(&alert_param, 0x00, sizeof(alert_param));
	len = strlen(s_ring_path) + 1;
	SAFE_FREE(alert_param.download_url);
	alert_param.download_url = (char *)DUER_MALLOC_SDRAM(len);
	if (!alert_param.download_url) {
		return;
	}
	snprintf(alert_param.download_url, len, "%s", s_ring_path);
	strcpy(alert_param.token, token);
	alert_param.ddt = ALERT;

	pDuerAppInfo->duer_play_stop_flag = 1;
	s_is_bell = true;

	DUER_LOGI("Alert Play Start: %s\n", alert_param.download_url);
	if (xTaskCreate(duer_http_audio_thread, ((const char*)"alert_thread"), 1024, &alert_param, tskIDLE_PRIORITY + 4, NULL) != pdPASS)
		DUER_LOGE("Create Speak thread failed!");
}

static void duer_alert_callback(void *param)
{
	duerapp_alert_node *alert = (duerapp_alert_node *)param;

	DUER_LOGI("alert started: token: %s", alert->token);

	duer_dcs_report_alert_event(alert->token, ALERT_START);
	// play url
	duer_alert_start(alert);
}

static duer_errcode_t duer_alert_list_push(duerapp_alert_node *data)
{
	duer_errcode_t rt = DUER_OK;
	duer_alert_list_t *new_node = NULL;
	duer_alert_list_t *tail = &s_alert_head;

	do {
		new_node = (duer_alert_list_t *)DUER_MALLOC_SDRAM(sizeof(duer_alert_list_t));
		if (!new_node) {
			DUER_LOGE("Memory too low");
			rt = DUER_ERR_MEMORY_OVERLOW;
			break;
		}
		new_node->next = NULL;
		new_node->data = data;

		while (tail->next) {
			tail = tail->next;
		}

		tail->next = new_node;
	} while (0);

	return rt;
}

static duer_errcode_t duer_alert_list_remove(duerapp_alert_node *data)
{
	duer_alert_list_t *pre = &s_alert_head;
	duer_alert_list_t *cur = NULL;
	duer_errcode_t rt = DUER_ERR_FAILED;

	while (pre->next) {
		cur = pre->next;
		if (cur->data == data) {
			pre->next = cur->next;
			DUER_FREE(cur);
			rt = DUER_OK;
			break;
		}
		pre = pre->next;
	}

	return rt;
}

static duerapp_alert_node *duer_create_alert_node(const char *token,
        const char *type,
        const char *time,
        duer_timer_handler handle)
{
	size_t len = 0;
	duerapp_alert_node *alert = NULL;

	do {
		alert = (duerapp_alert_node *)DUER_MALLOC_SDRAM(sizeof(duerapp_alert_node));
		if (!alert) {
			break;
		}

		memset(alert, 0, sizeof(duerapp_alert_node));

		len = strlen(token) + 1;
		alert->token = (char *)DUER_MALLOC_SDRAM(len);
		if (!alert->token) {
			break;
		}
		snprintf(alert->token, len, "%s", token);

		len = strlen(type) + 1;
		alert->type = (char *)DUER_MALLOC_SDRAM(len);
		if (!alert->type) {
			break;
		}
		snprintf(alert->type, len, "%s", type);

		len = strlen(time) + 1;
		alert->time = (char *)DUER_MALLOC_SDRAM(len);
		if (!alert->time) {
			break;
		}
		snprintf(alert->time, len, "%s", time);

		alert->isbell = false;

		alert->handle = handle;

		return alert;
	} while (0);

	DUER_LOGE("Memory too low");
	if (alert) {
		if (alert->token) {
			SAFE_FREE(alert->token);
		}

		if (alert->type) {
			SAFE_FREE(alert->type);
		}

		if (alert->time) {
			SAFE_FREE(alert->time);
		}

		SAFE_FREE(alert);
	}

	return NULL;
}


static void duer_free_alert_node(duerapp_alert_node *alert)
{
	if (alert) {
		if (alert->token) {
			SAFE_FREE(alert->token);
		}

		if (alert->type) {
			SAFE_FREE(alert->type);
		}

		if (alert->time) {
			SAFE_FREE(alert->time);
		}

		if (alert->handle) {
			duer_timer_release(alert->handle);
			alert->handle = NULL;
		}

		SAFE_FREE(alert);
	}
}

static time_t duer_dcs_get_time_stamp(const char *time)
{
	struct tm tm_;
	time_t time_;

	//struct tm tt;

	memset(&tm_, 0, sizeof(tm_)); //2018-01-18T11:26:35+0000
	tm_.tm_year = atoi(time) - 1900;
	tm_.tm_mon	= atoi(time + 5) - 1;
	tm_.tm_mday = atoi(time + 8);
	tm_.tm_hour = atoi(time + 11);
	tm_.tm_min	= atoi(time + 14);
	tm_.tm_sec	= atoi(time + 17);

	time_  = mktime(&tm_);

	printf("time as second %d\n",time_);
	return time_;
}

static void duer_alert_set_thread(void *param)
{
	int ret = 0;
	duerapp_alert_node *alert = NULL;
	DuerTime cur_time;
	size_t delay = 0;
	int rs = DUER_OK;
	time_t time_stamp = 0;

	duer_dcs_alert_info_type *node_param = NULL;
	node_param = (duer_dcs_alert_info_type *)param;

	DUER_LOGI("set alert: addr %p, scheduled_time: %s, token: %s\n",node_param, node_param->time, node_param->token);
	do {
		time_stamp = duer_dcs_get_time_stamp(node_param->time);
		if (time_stamp < 0) {
			duer_free_alert_node(alert);
			duer_dcs_report_alert_event(node_param->token, SET_ALERT_FAIL);
			break;
		}

		DUER_LOGI("time_stamp: %d\n", time_stamp);

		alert = duer_create_alert_node(node_param->token, node_param->type, node_param->time, NULL);
		if (!alert) {
			duer_free_alert_node(alert);
			duer_dcs_report_alert_event(node_param->token, SET_ALERT_FAIL);
			break;
		}

		ret = duer_ntp_client(NULL, 0, &cur_time, NULL);
		if (ret < 0) {
			DUER_LOGE("Failed to get NTP time.ret = %d", ret);
			duer_free_alert_node(alert);
			duer_dcs_report_alert_event(node_param->token, SET_ALERT_FAIL);
			break;
		}

		if (time_stamp <= cur_time.sec) {
			DUER_LOGE("The alert is expired\n");
			duer_free_alert_node(alert);
			duer_dcs_report_alert_event(node_param->token, SET_ALERT_FAIL);
			break;
		}

		delay = (time_stamp - cur_time.sec) * 1000 + cur_time.usec / 1000;

		alert->handle = duer_timer_acquire(duer_alert_callback, alert, DUER_TIMER_ONCE);
		if (!alert->handle) {
			DUER_LOGE("Failed to set alert: failed to create timer\n");
			duer_free_alert_node(alert);
			duer_dcs_report_alert_event(node_param->token, SET_ALERT_FAIL);
			break;
		}

		rs = duer_timer_start(alert->handle, delay);
		if (rs != DUER_OK) {
			DUER_LOGE("Failed to set alert: failed to start timer\n");
			duer_free_alert_node(alert);
			duer_dcs_report_alert_event(node_param->token, SET_ALERT_FAIL);
			break;
		}

		duer_mutex_lock(s_alert_mutex);
		duer_alert_list_push(alert);
		duer_mutex_unlock(s_alert_mutex);
		duer_dcs_report_alert_event(node_param->token, SET_ALERT_SUCCESS);
	} while (0);

	if (node_param) {
		SAFE_FREE(node_param->token);
		SAFE_FREE(node_param->time);
		SAFE_FREE(node_param->type);
		SAFE_FREE(node_param);
	}
	vTaskDelete(NULL);
}

void duer_dcs_alert_set_handler(const duer_dcs_alert_info_type *alert_info)
{
	duer_dcs_alert_info_type *param = NULL;
	int len = sizeof(duer_dcs_alert_info_type);
	param = (duer_dcs_alert_info_type *)DUER_MALLOC_SDRAM(len);
	do {
		if (!param) {
			break;
		}
		param->token = (char *)DUER_MALLOC_SDRAM(strlen(alert_info->token) + 1);
		param->time = (char *)DUER_MALLOC_SDRAM(strlen(alert_info->time) + 1);
		param->type = (char *)DUER_MALLOC_SDRAM(strlen(alert_info->type) + 1);
		if (!(param->token && param->time && param->type)) {
			break;
		}
		snprintf(param->token, strlen(alert_info->token) + 1, "%s", alert_info->token);
		snprintf(param->time, strlen(alert_info->time) + 1, "%s", alert_info->time);
		snprintf(param->type, strlen(alert_info->type) + 1, "%s", alert_info->type);

		//DUER_LOGI("set alert: addr %p, scheduled_time: %s, token: %s\n",param, param->time, param->token);
			
		if (xTaskCreate(duer_alert_set_thread, ((const char*)"alert_set_thread"), 1024, param, tskIDLE_PRIORITY + 5, NULL) != pdPASS) {
			DUER_LOGE("Create alert pthread failed!");
			duer_dcs_report_alert_event(alert_info->token, SET_ALERT_FAIL);
		}
		duer_dcs_report_alert_event(alert_info->token, SET_ALERT_SUCCESS);
		return;
	} while (0);

	DUER_LOGE("malloc param failed!");
	duer_dcs_report_alert_event(alert_info->token, SET_ALERT_FAIL);
	
	if (param) {
		SAFE_FREE(param->token);
		SAFE_FREE(param->time);
		SAFE_FREE(param->type);
		SAFE_FREE(param);
	}
}

duer_status_t duer_dcs_tone_alert_set_handler(const baidu_json *directive)
{
	int ret = DUER_OK;
	baidu_json *payload = NULL;
    baidu_json *scheduled_time = NULL;
    baidu_json *token = NULL;
    baidu_json *type = NULL;
    duer_dcs_alert_info_type alert_info;

    payload = baidu_json_GetObjectItem(directive, "payload");
    if (!payload) {
        DUER_LOGE("Failed to get payload");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    token = baidu_json_GetObjectItem(payload, "token");
    if (!token) {
        DUER_LOGE("Failed to get token");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    scheduled_time = baidu_json_GetObjectItem(payload, "scheduledTime");
    if (!scheduled_time) {
        DUER_LOGE("Failed to get scheduledTime");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    type = baidu_json_GetObjectItem(payload, "type");
    if (!type) {
        DUER_LOGE("Failed to get alert type");
        ret = DUER_MSG_RSP_BAD_REQUEST;
        goto RET;
    }

    alert_info.time = scheduled_time->valuestring;
    alert_info.token = token->valuestring;
    alert_info.type = type->valuestring;
    duer_dcs_alert_set_handler(&alert_info);

RET:
    return ret;
}

static duerapp_alert_node *duer_find_target_alert(const char *token)
{
	duer_alert_list_t *node = s_alert_head.next;
	while (node) {
		if (node->data) {
			if (strcmp(token, node->data->token) == 0) {
				return node->data;
			}
		}

		node = node->next;
	}

	return NULL;
}

void duer_dcs_alert_delete_handler(const char *token)
{
	duerapp_alert_node *target_alert = NULL;

	DUER_LOGI("delete alert: token %s", token);

	duer_mutex_lock(s_alert_mutex);

	target_alert = duer_find_target_alert(token);
	if (!target_alert) {
		DUER_LOGE("Cannot find the target alert\n");
		duer_mutex_unlock(s_alert_mutex);
		duer_dcs_report_alert_event(token, DELETE_ALERT_FAIL);
		return;
	}

	duer_alert_list_remove(target_alert);
	duer_free_alert_node(target_alert);

	duer_mutex_unlock(s_alert_mutex);

	duer_dcs_report_alert_event(token, DELETE_ALERT_SUCCESS);
}


void duer_dcs_get_all_alert(baidu_json *alert_array)
{
	duer_alert_list_t *alert_node = s_alert_head.next;
	duer_dcs_alert_info_type alert_info;
	bool is_active = false;

	while (alert_node) {
		alert_info.token = alert_node->data->token;
		alert_info.type = alert_node->data->type;
		alert_info.time = alert_node->data->time;
		duer_insert_alert_list(alert_array, &alert_info, alert_node->data->isbell);
		alert_node = alert_node->next;
	}
}

void duer_dcs_alert_on_finish(char *token) {
	s_is_bell = false;
	duer_dcs_report_alert_event(token, ALERT_STOP);
}


void duer_alert_init()
{
	if (!s_alert_mutex) {
		s_alert_mutex = duer_mutex_create();
	}
	if (!s_bell_mutex) {
		s_bell_mutex = duer_mutex_create();
	}
	duer_dcs_alert_init();
}
