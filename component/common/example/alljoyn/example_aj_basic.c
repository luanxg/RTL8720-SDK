/**
 * @file
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/
#define AJ_SERVICE_APP 0
#define AJ_CLIENT_APP  !AJ_SERVICE_APP

#include <stdio.h>
#include <stdlib.h>
#include <ajtcl/aj_debug.h>
#include <ajtcl/alljoyn.h>

#include "FreeRTOS.h"
#include "task.h"

#define CONNECT_ATTEMPTS   10
static const char ServiceName[] = "org.alljoyn.Bus.sample";
static const char ServicePath[] = "/sample";
static const uint16_t ServicePort = 25;

#if AJ_SERVICE_APP

#define AJ_MODULE BASIC_SERVICE

uint8_t dbgBASIC_SERVICE = 0;
/**
 * The interface name followed by the method signatures.
 *
 * See also .\inc\aj_introspect.h
 */
static const char* const sampleInterface[] = {
    "org.alljoyn.Bus.sample",   /* The first entry is the interface name. */
    "?Dummy foo<i",             /* This is just a dummy entry at index 0 for illustration purposes. */
    "?cat inStr1<s inStr2<s outStr>s", /* Method at index 1. */
    NULL
};

/**
 * A NULL terminated collection of all interfaces.
 */
static const AJ_InterfaceDescription sampleInterfaces[] = {
    sampleInterface,
    NULL
};

/**
 * Objects implemented by the application. The first member in the AJ_Object structure is the path.
 * The second is the collection of all interfaces at that path.
 */
static const AJ_Object AppObjects[] = {
    { ServicePath, sampleInterfaces },
    { NULL }
};

/*
 * The value of the arguments are the indices of the
 * object path in AppObjects (above), interface in sampleInterfaces (above), and
 * member indices in the interface.
 * The 'cat' index is 1 because the first entry in sampleInterface is the interface name.
 * This makes the first index (index 0 of the methods) the second string in
 * sampleInterface[] which, for illustration purposes is a dummy entry.
 * The index of the method we implement for basic_service, 'cat', is 1 which is the third string
 * in the array of strings sampleInterface[].
 *
 * See also .\inc\aj_introspect.h
 */
#define BASIC_SERVICE_CAT AJ_APP_MESSAGE_ID(0, 0, 1)

/*
 * Use async version of API for reply
 */
static uint8_t asyncForm = TRUE;

static AJ_Status AppHandleCat(AJ_Message* msg)
{
#define BUFFER_SIZE 256
    const char* string0;
    const char* string1;
    char buffer[BUFFER_SIZE];
    AJ_Message reply;
    AJ_Arg replyArg;

    AJ_UnmarshalArgs(msg, "ss", &string0, &string1);

    /* We have the arguments. Now do the concatenation. */
    strncpy(buffer, string0, BUFFER_SIZE);
    buffer[BUFFER_SIZE - 1] = '\0';
    strncat(buffer, string1, BUFFER_SIZE - strlen(buffer));
    buffer[BUFFER_SIZE - 1] = '\0';
    if (asyncForm) {
        AJ_MsgReplyContext replyCtx;
        AJ_CloseMsgAndSaveReplyContext(msg, &replyCtx);
        AJ_MarshalReplyMsgAsync(&replyCtx, &reply);
    } else {
        AJ_MarshalReplyMsg(msg, &reply);
    }
    AJ_InitArg(&replyArg, AJ_ARG_STRING, 0, buffer, 0);
    AJ_MarshalArg(&reply, &replyArg);

    return AJ_DeliverMsg(&reply);

#undef BUFFER_SIZE
}

/* All times are expressed in milliseconds. */
#define CONNECT_TIMEOUT     (1000 * 60)
#define UNMARSHAL_TIMEOUT   (1000 * 5)
#define SLEEP_TIME          (1000 * 2)

static void aj_basic_example(void *param)
{
    vTaskDelay(5*1000);
    
    AJ_AlwaysPrintf(("\n\n====================AJ_BASIC_SERVICE====================\n"));
    
    AJ_Status status = AJ_OK;
    AJ_BusAttachment bus;
    uint8_t connected = FALSE;
    uint32_t sessionId = 0;

    /* One time initialization before calling any other AllJoyn APIs. */
    AJ_Initialize();

    /* This is for debug purposes and is optional. */
    AJ_PrintXML(AppObjects);

    AJ_RegisterObjects(AppObjects, NULL);

    while (TRUE) {
        AJ_Message msg;

        if (!connected) {
            status = AJ_StartService(&bus,
                                     NULL,
                                     CONNECT_TIMEOUT,
                                     FALSE,
                                     ServicePort,
                                     ServiceName,
                                     AJ_NAME_REQ_DO_NOT_QUEUE,
                                     NULL);

            if (status != AJ_OK) {
                continue;
            }

            AJ_InfoPrintf(("StartService returned %d, session_id=%u\n", status, sessionId));
            connected = TRUE;
        }

        status = AJ_UnmarshalMsg(&bus, &msg, UNMARSHAL_TIMEOUT);

        if (AJ_ERR_TIMEOUT == status) {
            continue;
        }

        if (AJ_OK == status) {
            switch (msg.msgId) {
            case AJ_METHOD_ACCEPT_SESSION:
                {
                    uint16_t port;
                    char* joiner;
                    AJ_UnmarshalArgs(&msg, "qus", &port, &sessionId, &joiner);
                    status = AJ_BusReplyAcceptSession(&msg, TRUE);
                    AJ_InfoPrintf(("Accepted session session_id=%u joiner=%s\n", sessionId, joiner));
                }
                break;

            case BASIC_SERVICE_CAT:
                status = AppHandleCat(&msg);
                break;

            case AJ_SIGNAL_SESSION_LOST_WITH_REASON:
                {
                    uint32_t id, reason;
                    AJ_UnmarshalArgs(&msg, "uu", &id, &reason);
                    AJ_AlwaysPrintf(("Session lost. ID = %u, reason = %u\n", id, reason));
                }
                break;

            default:
                /* Pass to the built-in handlers. */
                status = AJ_BusHandleBusMessage(&msg);
                break;
            }
        }

        /* Messages MUST be discarded to free resources. */
        AJ_CloseMsg(&msg);

        if ((status == AJ_ERR_READ) || (status == AJ_ERR_WRITE)) {
            AJ_AlwaysPrintf(("AllJoyn disconnect.\n"));
            AJ_Disconnect(&bus);
            connected = FALSE;

            /* Sleep a little while before trying to reconnect. */
            AJ_Sleep(SLEEP_TIME);
        }
    }

    AJ_AlwaysPrintf(("Basic service exiting with status %d.\n", status));
    
    vTaskDelete(NULL);
}

#endif //AJ_SERVICE_APP

#if AJ_CLIENT_APP

#define AJ_MODULE BASIC_CLIENT

#define  ALLJOYN_PLATFORM 1

/*
 * Buffer to hold the full service name. This buffer must be big enough to hold
 * a possible 255 characters plus a null terminator (256 bytes)
 */
static char fullServiceName[AJ_MAX_SERVICE_NAME_SIZE];

uint8_t dbgBASIC_CLIENT = 0;
/**
 * The interface name followed by the method signatures.
 *
 * See also .\inc\aj_introspect.h
 */
static const char* const sampleInterface[] = {
    "org.alljoyn.Bus.sample",   /* The first entry is the interface name. */
    "?Dummy foo<i",             /* This is just a dummy entry at index 0 for illustration purposes. */
    "?Dummy2 fee<i",            /* This is just a dummy entry at index 1 for illustration purposes. */
    "?cat inStr1<s inStr2<s outStr>s", /* Method at index 2. */
    NULL
};

/**
 * A NULL terminated collection of all interfaces.
 */
static const AJ_InterfaceDescription sampleInterfaces[] = {
    sampleInterface,
    NULL
};

/**
 * Objects implemented by the application. The first member in the AJ_Object structure is the path.
 * The second is the collection of all interfaces at that path.
 */
static const AJ_Object AppObjects[] = {
    { ServicePath, sampleInterfaces },
    { NULL }
};

/*
 * The value of the arguments are the indices of the object path in AppObjects (above),
 * interface in sampleInterfaces (above), and member indices in the interface.
 * The 'cat' index is 2. The reason for this is as follows: The first entry in sampleInterface
 * is the interface name. This makes the first index (index 0 of the methods) the second string in
 * sampleInterface[]. The two dummy entries are indices 0 and 1. The index of the method we
 * implement for basic_client, 'cat', is 2 which is the fourth string in the array of strings
 * sampleInterface[].
 *
 * See also .\inc\aj_introspect.h
 */
#define BASIC_CLIENT_CAT AJ_PRX_MESSAGE_ID(0, 0, 2)

#define CONNECT_TIMEOUT    (1000 * 60)
#define UNMARSHAL_TIMEOUT  (1000 * 5)
#define METHOD_TIMEOUT     (100 * 10)

void MakeMethodCall(AJ_BusAttachment* bus, uint32_t sessionId)
{
    AJ_Status status;
    AJ_Message msg;

    status = AJ_MarshalMethodCall(bus, &msg, BASIC_CLIENT_CAT, fullServiceName, sessionId, 0, METHOD_TIMEOUT);

    if (status == AJ_OK) {
        status = AJ_MarshalArgs(&msg, "ss", "Hello ", "World!");
    }

    if (status == AJ_OK) {
        status = AJ_DeliverMsg(&msg);
    }

    AJ_InfoPrintf(("MakeMethodCall() resulted in a status of 0x%04x.\n", status));
}

static void aj_basic_example(void *param)
{
    vTaskDelay(5*1000);
    
    AJ_AlwaysPrintf(("\n\n====================AJ_BASIC_CLIENT====================\n"));
    
    AJ_Status status = AJ_OK;
    AJ_BusAttachment bus;
    uint8_t connected = FALSE;
    uint8_t done = FALSE;
    uint32_t sessionId = 0;

    /*
     * One time initialization before calling any other AllJoyn APIs
     */
    AJ_Initialize();
    AJ_PrintXML(AppObjects);
    AJ_RegisterObjects(NULL, AppObjects);

    while (!done) {
        AJ_Message msg;

        if (!connected) {
            status = AJ_StartClientByName(&bus,
                                          NULL,
                                          CONNECT_TIMEOUT,
                                          FALSE,
                                          ServiceName,
                                          ServicePort,
                                          &sessionId,
                                          NULL,
                                          fullServiceName);

            if (status == AJ_OK) {
                AJ_InfoPrintf(("StartClient returned %d, sessionId=%u.\n", status, sessionId));
                connected = TRUE;

                MakeMethodCall(&bus, sessionId);
            } else {
                AJ_InfoPrintf(("StartClient returned 0x%04x.\n", status));
                break;
            }
        }

        status = AJ_UnmarshalMsg(&bus, &msg, UNMARSHAL_TIMEOUT);

        if (AJ_ERR_TIMEOUT == status) {
            continue;
        }

        if (AJ_OK == status) {
            switch (msg.msgId) {
            case AJ_REPLY_ID(BASIC_CLIENT_CAT):
                if (msg.hdr->msgType == AJ_MSG_METHOD_RET) {
                    AJ_Arg arg;

                    status = AJ_UnmarshalArg(&msg, &arg);

                    if (AJ_OK == status) {
                        AJ_AlwaysPrintf(("'%s.%s' (path='%s') returned '%s'.\n", fullServiceName, "cat",
                                         ServicePath, arg.val.v_string));
                        done = TRUE;
                    } else {
                        AJ_InfoPrintf(("AJ_UnmarshalArg() returned status %d.\n", status));
                        /* Try again because of the failure. */
                        MakeMethodCall(&bus, sessionId);
                    }
                } else {
                    const char* info = "";
                    if (AJ_UnmarshalArgs(&msg, "s", &info) == AJ_OK) {
                        AJ_AlwaysPrintf(("Method call returned error %s (%s)\n", msg.error, info));
                    } else {
                        AJ_AlwaysPrintf(("Method call returned error %s\n", msg.error));
                    }
                    done = TRUE;
                }
                break;

            case AJ_SIGNAL_SESSION_LOST_WITH_REASON:
                /* A session was lost so return error to force a disconnect. */
                {
                    uint32_t id, reason;
                    AJ_UnmarshalArgs(&msg, "uu", &id, &reason);
                    AJ_AlwaysPrintf(("Session lost. ID = %u, reason = %u\n", id, reason));
                }
                status = AJ_ERR_SESSION_LOST;
                break;

            default:
                /* Pass to the built-in handlers. */
                status = AJ_BusHandleBusMessage(&msg);
                break;
            }
        }

        /* Messages MUST be discarded to free resources. */
        AJ_CloseMsg(&msg);

        if ((status == AJ_ERR_READ) || (status == AJ_ERR_WRITE) || (status == AJ_ERR_SESSION_LOST)) {
            AJ_AlwaysPrintf(("AllJoyn disconnect.\n"));
            AJ_Disconnect(&bus);
            exit(0);
        }
    }

    AJ_AlwaysPrintf(("Basic client exiting with status %d.\n", status));
    
    vTaskDelete(NULL);
}

#endif //AJ_CLIENT_APP



void example_aj_basic(void)
{
    if(xTaskCreate(aj_basic_example, ((const char*)"aj_basic_client"), 3072, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
        printf("\n\r%s xTaskCreate failed", __FUNCTION__);
}
