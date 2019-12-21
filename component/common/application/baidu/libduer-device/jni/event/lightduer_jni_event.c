/**
 * copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define TAG "lightduer_jni_event"

#include "lightduer_jni.h"
#include "lightduer_jni_event.h"
#include "lightduer_jni_log.h"

struct EventCallback {
    jobject obj;
    jmethodID method;
    jobject lightDuerEvent;
    jobject lightDuerEventId;
};

static struct EventCallback s_eventCB;

static jobject init_lightduerevent(JNIEnv *jenv)
{
    jclass lightduereventclass = (*jenv)->FindClass(jenv, PACKAGE"/event/LightduerEvent");
    if (lightduereventclass == NULL) {
        LOGE("lightduereventclass get fail\n");
        return NULL;
    }

    jmethodID constructor =
            (*jenv)->GetMethodID(jenv, lightduereventclass, "<init>", "()V");
    if (constructor == NULL) {
        LOGE("get constructor failed\n");
        return NULL;
    }

    jobject result = (*jenv)->NewObject(jenv, lightduereventclass, constructor);
    if (!result) {
        LOGE("create object failed\n");
        return NULL;
    }

    return result;

}

static jobject init_lightduereventid(JNIEnv *jenv)
{
    jclass jduereventid = (*jenv)->FindClass(jenv, PACKAGE"/event/LightduerEventId");
    if (jduereventid == NULL) {
        LOGE("get jduereventid failed\n");
        return NULL;
    }

    jfieldID eventIdField = (*jenv)->GetStaticFieldID(jenv, jduereventid, "DUER_EVENT_STARTED",
                                                  "L"PACKAGE"/event/LightduerEventId;");

    if (eventIdField == NULL) {
        LOGE("get eventIdField failed\n");
        return NULL;
    }

    jobject eventId = (*jenv)->GetStaticObjectField(jenv, jduereventid, eventIdField);
    if (eventId == NULL) {
        LOGE("get eventId failed\n");
        return NULL;
    }

    return eventId;
}

int duer_jni_event_init(JNIEnv *jenv, jobject obj)
{
    jclass event_callback_class = (*jenv)->GetObjectClass(jenv, obj);
    jmethodID callback = (*jenv)->GetMethodID(jenv, event_callback_class, "callback",
                                          "(L"PACKAGE"/event/LightduerEvent;)V");
    if (callback == NULL) {
        LOGE("method callback is not found!!\n");
        return DUER_ERR_FAILED;
    }

    s_eventCB.method = callback;
    s_eventCB.obj = (*jenv)->NewGlobalRef(jenv, obj);
    s_eventCB.lightDuerEvent = (*jenv)->NewGlobalRef(jenv, init_lightduerevent(jenv));
    s_eventCB.lightDuerEventId = (*jenv)->NewGlobalRef(jenv, init_lightduereventid(jenv));
    if (!s_eventCB.obj || !s_eventCB.lightDuerEvent || !s_eventCB.lightDuerEventId) {
        LOGE("init event failed, %p, %p, %p\n", s_eventCB.obj, s_eventCB.lightDuerEvent, s_eventCB.lightDuerEventId);
        return DUER_ERR_FAILED;
    }
    return DUER_OK;
}

static jclass duer_jni_get_class_LightduerEvent(JNIEnv *jenv)
{
    return (*jenv)->GetObjectClass(jenv, s_eventCB.lightDuerEvent);
}

static jclass duer_jni_get_class_LightduerEventId(JNIEnv *jenv)
{
    return (*jenv)->GetObjectClass(jenv, s_eventCB.lightDuerEventId);
}

void duer_jni_event_call_callback(JNIEnv *jenv, duer_event_t *event)
{
    int result = DUER_ERR_FAILED;

    do {
        jclass jduereventclass = duer_jni_get_class_LightduerEvent(jenv);
        if (jduereventclass == NULL) {
            LOGE("LightduerEvent not found!\n");
            break;
        }
        jmethodID constru =
                (*jenv)->GetMethodID(jenv, jduereventclass, "<init>",
                                  "(L"PACKAGE"/event/LightduerEventId;ILjava/lang/Object;)V");
        if (constru == NULL) {
            LOGE("get constructor failed\n");
            break;
        }

        jclass jduereventid = duer_jni_get_class_LightduerEventId(jenv);
        if (jduereventid == NULL) {
            LOGE("get jduereventid failed\n");
            break;
        }

        jfieldID eventIdField = 0;
        switch (event->_event) {
            case DUER_EVENT_STARTED:
                eventIdField = (*jenv)->GetStaticFieldID(jenv, jduereventid, "DUER_EVENT_STARTED",
                                                      "L"PACKAGE"/event/LightduerEventId;");
                break;
            case DUER_EVENT_STOPPED:
                eventIdField = (*jenv)->GetStaticFieldID(jenv, jduereventid, "DUER_EVENT_STOPPED",
                                                      "L"PACKAGE"/event/LightduerEventId;");
                break;
            default:
                LOGE("unknow event\n");
                break;
        }
        if (eventIdField == NULL) {
            LOGE("get eventIdField failed\n");
            break;
        }
        jobject eventId = (*jenv)->GetStaticObjectField(jenv, jduereventid, eventIdField);
        if (eventId == NULL) {
            LOGE("get eventId failed\n");
            break;
        }
        if (event->_object != NULL) {
            //TODO
            LOGE("TODO try to transfer the object to java layer!\n");
        }
        jobject duerevent =
                (*jenv)->NewObject(jenv, jduereventclass, constru, eventId, event->_code, NULL);
        if (duerevent == NULL) {
            LOGE("get duerevent failed\n");
            break;
        }
        (*jenv)->CallVoidMethod(jenv, s_eventCB.obj, s_eventCB.method, duerevent);
        result = DUER_OK;
    } while(0);

    if (result != DUER_OK) {
        LOGE("call failed!\n");
    }
}
