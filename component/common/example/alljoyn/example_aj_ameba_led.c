/*REF: https://github.com/fonlabs/ajtcl/blob/master/test/clientlite.c */

#include <stdio.h>
#include <stdlib.h>
#include <ajtcl/aj_debug.h>
#include <ajtcl/alljoyn.h>
#include <ajtcl/aj_msg.h>

#include "FreeRTOS.h"
#include "task.h"
#include "Extensions.h"

//GPIO 
#include "device.h"
#include "gpio_api.h"

#define  LED1   PC_0 //D10 on J21
#define  LED2   PC_2 //D11 on J21
#define  LED3   PC_3 //D12 on J21
#define  PB1    PC_1 //D13 on J21
#define  PB2    PC_4 //I2C_SDA on J21
#define  PB3    PC_5 //I2C_SCL on J21

gpio_t led1,led2,led3,pb1,pb2,pb3;

static const char InterfaceName[] = "com.rtk.ameba.led.interface";
static const char serviceName[] =   "com.rtk.ameba";
static const char servicePath[] =   "/led";
static const uint16_t servicePort = 25;

static char fullServiceName[AJ_MAX_SERVICE_NAME_SIZE];

static const char* const sampleInterface[] = {
    InterfaceName,
    "@Val=i",      /*must be "Val" (can't be "val") to match android side "getVal()" and "setVal()"*/      
    NULL
};

static const AJ_InterfaceDescription sampleInterfaces[] = {
    sampleInterface,
    AJ_PropertiesIface, /*notice the order of 'sampleInterface' and 'AJ_PropertiesIface'*/
    NULL
};


static const AJ_Object AppObjects[] = {
    { servicePath, sampleInterfaces },
    { NULL }
};

#define BUS_PROPERTY    AJ_PRX_PROPERTY_ID(0, 0, 0) //'Val' property is at index 0
#define GET_VAL         AJ_PRX_MESSAGE_ID(0, 1, AJ_PROP_GET)
#define SET_VAL         AJ_PRX_MESSAGE_ID(0, 1, AJ_PROP_SET) 

#define CONNECT_TIMEOUT    (1000 * 60)
#define METHOD_TIMEOUT     (100 * 10)
#define UNMARSHAL_TIMEOUT  (1000 * 5)


static AJ_BusAttachment busAttachment;
static  AJ_Message msg;
static uint32_t sessionId = 0;
static int val = 0;

void turnOnLED (){
    
    if (1 == val){
    	gpio_write(&led1,1);
    	gpio_write(&led2,0);
    	gpio_write(&led3,0);
    }
    if(2 == val){
    	gpio_write(&led1,0);
    	gpio_write(&led2,1);
    	gpio_write(&led3,0);
    }
    if(3 == val){
    	gpio_write(&led1,0);
    	gpio_write(&led2,0);
    	gpio_write(&led3,1);
    }
}

AJ_Status setVal()
{
    AJ_Status status;
    status = AJ_MarshalMethodCall(&busAttachment, &msg, SET_VAL, fullServiceName, sessionId, 0, METHOD_TIMEOUT);

    if (status == AJ_OK) {
        status = AJ_MarshalPropertyArgs(&msg, BUS_PROPERTY);

        if (status == AJ_OK) {
            status = AJ_MarshalArgs(&msg, "i", val);
        }

        if (status == AJ_OK) {
            status = AJ_DeliverMsg(&msg);
            log("setVal: %d", val);
        }
    }
    return status;
}

AJ_Status getVal() {
    AJ_Status status;
    status = AJ_MarshalMethodCall(&busAttachment, &msg, GET_VAL, fullServiceName, sessionId, 0, METHOD_TIMEOUT);

    if (status == AJ_OK) {
        AJ_MarshalPropertyArgs(&msg, BUS_PROPERTY);
        status = AJ_DeliverMsg(&msg);
    }
    return status;
}

static void aj_ameba_led (void *param) {
    
    vTaskDelay(5*1000);
    log("\n*** Ameba LED AllJoyn (Bus Property) Example***");

    gpio_t* pLED[] = {&led1, &led2, &led3};
    gpio_t* pBtn[] = {&pb1, &pb2, &pb3};
    PinName LED[] = {LED1, LED2, LED3};
    PinName btn[] = {PB1, PB2, PB3};
    
    for(int i=0 ; i<3 ; ++i){
        
        //Init LED control pin
        gpio_init(pLED[i], LED[i]);
        gpio_dir(pLED[i], PIN_OUTPUT); 
        gpio_mode(pLED[i], PullNone);    
        
        //Initi Push Button pin
        gpio_init(pBtn[i], btn[i]);
        gpio_dir(pBtn[i], PIN_INPUT);    
        gpio_mode(pBtn[i], PullDown);       
    }


    uint8_t connected = FALSE;
    AJ_Status status = AJ_OK;

    AJ_Initialize();
    AJ_RegisterObjects(NULL, AppObjects);

    while (TRUE) {

        uint8_t pbPressed = FALSE;

        if (gpio_read(&pb1)) {
            val = 1;
            pbPressed = TRUE;
        }

        if (gpio_read(&pb2)) {
            val = 2;
            pbPressed = TRUE;
        }

        if (gpio_read(&pb3)) {
            val = 3;
            pbPressed = TRUE;
        }

        if (!connected) {
            log("Starting Client ...");
            status = AJ_StartClientByName(&busAttachment,NULL, CONNECT_TIMEOUT,FALSE,serviceName, servicePort,&sessionId,NULL,fullServiceName);

            if (status != AJ_OK) {
                continue;
            }

            log("Start Client returned %s", AJ_StatusText(status));
            connected = TRUE;
        }

        turnOnLED();
        
        status = pbPressed ? setVal() : getVal();

        if (AJ_OK == status) {

            status = AJ_UnmarshalMsg(&busAttachment, &msg, UNMARSHAL_TIMEOUT);

            if (AJ_OK == status) {
                switch (msg.msgId)
                {
                case AJ_METHOD_ACCEPT_SESSION:
                {
                    uint16_t port;
                    char* joiner;
                    uint32_t sessionId;

                    AJ_UnmarshalArgs(&msg, "qus", &port, &sessionId, &joiner);
                    status = AJ_BusReplyAcceptSession(&msg, TRUE);
                    log("AJ_METHOD_ACCEPT_SESSION Port=%u, session_id=%u joiner='%s'.\n", port, sessionId, joiner);
                    break;
                }
                case AJ_REPLY_ID(SET_VAL):
                {
                    log("AJ_REPLY_ID(SET_VAL)");
                    break;
                }
                case AJ_REPLY_ID(GET_VAL):
                {
                    /*the 'sig' is 'i', but use AJ_UnmarshalArgs(&msg, "i", &val) directlly will lead to error*/
                    const char* sig;
                    status = AJ_UnmarshalVariant(&msg, &sig);
                    if (status == AJ_OK) {
                        status = AJ_UnmarshalArgs(&msg, sig, &val);
                        if (status == AJ_OK) {
                            log("getVal: %d", val);
                        }
                    }

                    log("AJ_REPLY_ID(GET_VAL)");
                    break;
                }
                case AJ_SIGNAL_SESSION_LOST_WITH_REASON:
                {
                    uint32_t id, reason;
                    AJ_UnmarshalArgs(&msg, "uu", &id, &reason);
                    log("Session lost. ID = %u, reason = %u", id, reason);
                    break;
                }

                default:
                {
                    status = AJ_BusHandleBusMessage(&msg);
                    break;
                }
                }
            }

        }


        /* Messages MUST be discarded to free resources. */
        AJ_CloseMsg(&msg);

        if ((status != AJ_OK) && (status != AJ_ERR_TIMEOUT)) {
            log("AllJoyn Disconnect: %s", AJ_StatusText(status));
            AJ_Disconnect(&busAttachment);
            connected = FALSE;
        }
        
       vTaskDelay(100);
    }
    
    vTaskDelete(NULL);
}


void example_aj_ameba_led(void)
{
    if (xTaskCreate(aj_ameba_led, ((const char*)"aj_ameba_led_example"), 3072, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        log("\n\r%s xTaskCreate failed", __FUNCTION__);
    }
        
}
