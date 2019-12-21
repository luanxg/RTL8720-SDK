#include <stdio.h>
#include <string.h>
#include "stdbool.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_coap_defs.h"
#include "lightduer_connagent.h"
#include "duerapp_config.h"

typedef enum{
	LightSwitch,
	AudioPause,
	AirConditionMod,
	AirConditionTmp,
}Duer_SmartHome_Type_t;

static bool light_on = false;

static int duer_lightSwitch_result(const char *action) {
    baidu_json *content = NULL;
    baidu_json *value = NULL;

    content = baidu_json_Parse(action);
    if (content == NULL) {
        DUER_LOGW("[%d]bad resp", __LINE__);
        return DUER_ERR_FAILED;
    }

    value = baidu_json_GetObjectItem(content, "value");
    if (!value) {
        DUER_LOGW("[%d]bad resp", __LINE__);
        return DUER_ERR_FAILED;
    }

    if (strcmp("1", value->valuestring) == 0 && !light_on) {
        printf("open light\r\n");
        light_on = true;
        duerapp_gpio_led_mode(DUER_LED_ON);

    } else if (strcmp("0", value->valuestring) == 0 && light_on) {
        printf("close light\r\n");
        light_on = false;
        duerapp_gpio_led_mode(DUER_LED_OFF);
    }

    baidu_json_Delete(content);
    return DUER_OK;
}

static int duer_audioPause_result(const char *action) {
    baidu_json *content = NULL;
    baidu_json *value = NULL;

    content = baidu_json_Parse(action);
    if (content == NULL) {
        DUER_LOGW("[%d]bad resp", __LINE__);
        return DUER_ERR_FAILED;
    }

    value = baidu_json_GetObjectItem(content, "value");
    if (!value) {
        DUER_LOGW("[%d]bad resp", __LINE__);
        return DUER_ERR_FAILED;
    }

    if (strcmp("1", value->valuestring) == 0 /*&& !light_on*/) {
        printf("!!pause audio\r\n");

    } else if (strcmp("0", value->valuestring) == 0 /*&& light_on*/) {
        printf("!!resume play\r\n");
    }

    baidu_json_Delete(content);
    return DUER_OK;
}


static void SetAirConditionMode(int mode){

}
static int duer_airConditionMod_result(const char *action) {
    baidu_json *content = NULL;
    baidu_json *value = NULL;

	int mode = 0;

    content = baidu_json_Parse(action);
    if (content == NULL) {
        DUER_LOGW("[%d]bad resp", __LINE__);
        return DUER_ERR_FAILED;
    }

    value = baidu_json_GetObjectItem(content, "value");
    if (!value) {
        DUER_LOGW("[%d]bad resp", __LINE__);
		baidu_json_Delete(content);
        return DUER_ERR_FAILED;
    }

	mode = atoi(value->valuestring);
	printf("air-condition mode %d\n",mode);
	// mode 0, COOL; 1, HEAT; 3, AUTO; 4, FAN; 5,DEHUMIDIFICATION
	// 6, SLEEP
	SetAirConditionMode(mode);

    baidu_json_Delete(content);
    return DUER_OK;
}

static int duer_airConditionTmp_result(const char *action) {
    baidu_json *content = NULL;
    baidu_json *value = NULL;

    content = baidu_json_Parse(action);
    if (content == NULL) {
        DUER_LOGW("[%d]bad resp", __LINE__);
        return DUER_ERR_FAILED;
    }

    value = baidu_json_GetObjectItem(content, "value");
    if (!value) {
        // get air temp

		baidu_json_Delete(content);
    	return DUER_OK;
    }

	if(strstr(value->valuestring,"+")){
		printf("++++");
	} else if(strstr(value->valuestring,"-")){
		printf("----");
	} else{
		printf("00000");
	}

    baidu_json_Delete(content);
    return DUER_OK;
}

static duer_status_t DuerGetRsp(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr, Duer_SmartHome_Type_t type)
{
    int ret = DUER_ERR_FAILED;
    char *action = NULL;
    duer_handler handler = (duer_handler)ctx;

    int msg_code = DUER_MSG_RSP_CHANGED;
    if (handler && msg) {
        if (msg->payload && msg->payload_len > 0) {
            action = (char *)DUER_MALLOC(msg->payload_len + 1);
            if (action) {
                snprintf(action, msg->payload_len + 1, "%s", (char*)msg->payload);
                action[msg->payload_len] = '\0';
                DUER_LOGI("got result: %s", action);
				
                switch (type){
				case LightSwitch:
					
                	ret = duer_lightSwitch_result(action);
					break;
				case AudioPause:
					ret = duer_audioPause_result(action);
					break;
				case AirConditionMod:
					ret = duer_airConditionMod_result(action);
					break;
				case AirConditionTmp:
					ret = duer_airConditionTmp_result(action);
					break;
				default:
					break;
				}
                DUER_FREE(action);
            } else {
                DUER_LOGW("[%s] malloc fail", __func__);
                ret = DUER_ERR_FAILED;
            }
            msg_code = DUER_MSG_RSP_CHANGED;
        } else {
            msg_code = DUER_MSG_RSP_BAD_REQUEST;
        }
    }
    if (ret != DUER_OK)
        duer_response(msg, msg_code, "false", 0);
    else
        duer_response(msg, msg_code, "true", 0);

    return ret;
}

static duer_status_t LightCtrl(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr){
	Duer_SmartHome_Type_t type = LightSwitch;
	DuerGetRsp(ctx,msg,addr,type);
}

static duer_status_t AudioPauseCtrl(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr){
	Duer_SmartHome_Type_t type = AudioPause;
	DuerGetRsp(ctx,msg,addr,type);
}

static duer_status_t AirConditionModCtrl(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr){
	Duer_SmartHome_Type_t type = AirConditionMod;
	DuerGetRsp(ctx,msg,addr,type);
}

static duer_status_t AirConditionTmpCtrl(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr){
	Duer_SmartHome_Type_t type = AirConditionTmp;
	DuerGetRsp(ctx,msg,addr,type);
}

void duer_control_point_init()
{
    duer_res_t res[] = {
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT , "light", .res.f_res = LightCtrl},
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT , "AUDIO_PAUSE", .res.f_res = AudioPauseCtrl},
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT , "AIR_CONDITION_MOD", .res.f_res = AirConditionModCtrl},
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT , "AIR_CONDITION_TMP", .res.f_res = AirConditionTmpCtrl},
    };
    duer_add_resources(res, sizeof(res) / sizeof(res[0]));
}