// Copyright (2017) Baidu Inc. All rights reserved.
/**
 * File: duerapp_play_event.c
 * Auth: Zhong Shuai(zhongshuai@baidu.com)
 * Desc: Duer Play event
 */

#include "lightduer_play_event.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "duerapp_config.h"
#include "lightduer_types.h"
#include "lightduer_voice.h"
#include "lightduer_key.h"
//#include "duerapp_key.h"
#include "baidu_json.h"

#define VOICE_RESULT_QUEUE_LEN 3

extern TaskHandle_t duer_test_task_handle;

static TaskHandle_t play_media_task_handle = NULL;
static TaskHandle_t triger_gpio_task_handle = NULL;
QueueHandle_t voice_result_queue;

static void play_meida(void *arg)
{
    int ret = 0;
    char *voice_str = NULL;
    BaseType_t status;

    for (;;) {
        DUER_LOGI("Start play_media task");

        baidu_json *result = baidu_json_CreateObject();
        if (result == NULL) {
            DUER_LOGE("Create baidu json object failed");

            continue;
        }

        status = xQueueReceive(voice_result_queue, result, portMAX_DELAY);
        if (status != pdTRUE) {
            DUER_LOGE("Receive voice result failed");

            goto err;
        }

        voice_str = baidu_json_PrintUnformatted(result);
        if (voice_str != NULL) {
            DUER_LOGI("voice result:%s", voice_str);
        }

        ret = duer_set_play_info(result);
        if (ret != DUER_OK) {
            DUER_LOGI("Set play info failed ret: %d", ret);

            goto err;
        }

        ret = duer_report_play_status(START_PLAY);
        if (ret != DUER_OK) {
            DUER_LOGI("Report start play failed ret: %d", ret);

            goto err;
        }
#ifndef TEST_PLAYLIST
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        vTaskResume(triger_gpio_task_handle);

        vTaskDelay(5000 / portTICK_PERIOD_MS);

        ret = duer_report_play_status(START_PLAY);
        if (ret != DUER_OK) {
            DUER_LOGI("Report start play failed ret: %d", ret);

            goto err;
        }

#endif
        vTaskDelay(5000 / portTICK_PERIOD_MS);

        ret = duer_report_play_status(END_PLAY);
        if (ret != DUER_OK) {
            DUER_LOGI("Report end play failed ret: %d", ret);

            goto err;
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
err:
        baidu_json_Delete(result);

        ////if (duer_test_task_handle) {
        //    vTaskResume(duer_test_task_handle);
        //}
    }
}

static void triger_gpio_task(void *arg)
{
    static int cnt = 0;

    for (;;) {
        vTaskSuspend(NULL);

        DUER_LOGI("Set GPIO[] level %d", ++cnt % 2);

        //gpio_set_level(GPIO_OUTPUT_IO_0, cnt % 2);
    }
}

int duer_init_play(void)
{
    int status = DUER_ERR_FAILED;
    BaseType_t ret;

    // Initialize key hanlder framework
    status = duer_init_key_handler();
    if (status == DUER_ERR_FAILED) {
        DUER_LOGE("Init key handler failed");
    }

    // Initialize play event framework
    status = duer_init_play_event();
    if (status == DUER_ERR_FAILED) {
        DUER_LOGE("Init paly event failed");
    }

    voice_result_queue = xQueueCreate(VOICE_RESULT_QUEUE_LEN, sizeof(baidu_json));
    if (!voice_result_queue) {
        status = DUER_ERR_FAILED;

        DUER_LOGE("Create voice result queue failed");
    }

    // Create a task to triger GPIO IRQ, simulate stop key press
    ret = xTaskCreate(triger_gpio_task, "triger_gpio", 2 * 1024, NULL, 4, &triger_gpio_task_handle);
    if (ret != pdPASS) {
        status = DUER_ERR_FAILED;

        DUER_LOGE("Create triger_gpio_task task failed");
    }

    // Create a task to play media
    ret = xTaskCreate(play_meida, "play_media", 1024 * 4, NULL, 5, &play_media_task_handle);
    if (ret != pdPASS) {
        status = DUER_ERR_FAILED;

        DUER_LOGE("Create play media task failed");
    }

    return status;
}
