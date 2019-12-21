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

#define TAG "lightduer_jni_controlpoint"

#include "stdbool.h"
#include "lightduer_jni.h"
#include "lightduer_jni_controlpoint.h"
#include "lightduer_jni_log.h"
#include "lightduer_jni_utils.h"
#include "lightduer_memory.h"

static jobject s_resource_listener = NULL;

int duer_jni_controlpoint_init(JNIEnv *jenv, jobject obj)
{
    jclass lightduerResourceClass = (*jenv)->GetObjectClass(jenv, obj);
    if (lightduerResourceClass == NULL) {
        LOGE("get class for LightduerResource fail!\n");
        return DUER_ERR_FAILED;
    }
    jfieldID fieldDynamicCallback =
            (*jenv)->GetFieldID(jenv, lightduerResourceClass,
                            "dynamicCallback",
                            "L"PACKAGE"/controlpoint/LightduerResourceListener;");
    if (fieldDynamicCallback == NULL) {
        LOGE("get field dynamicCallback failed\n");
        return DUER_ERR_FAILED;
    }

    if (s_resource_listener == NULL) {
        LOGD("set the java resource callback for dynamic resource!!\n");
        jobject cb = (*jenv)->GetObjectField(jenv, obj, fieldDynamicCallback);
        s_resource_listener = (*jenv)->NewGlobalRef(jenv, cb);
    }
    return DUER_OK;
}

int duer_jni_call_callback_LightduerResourceListener(JNIEnv *jenv,
                                                     jobject context,
                                                     jobject message,
                                                     jobject address)
{

    if (s_resource_listener == NULL) {
        LOGE("s_resource_listener is NULL!\n");
        return DUER_ERR_FAILED;
    }

    do {
        jclass resourceListenerClass = (*jenv)->GetObjectClass(jenv, s_resource_listener);
        jmethodID callback = (*jenv)->GetMethodID(jenv, resourceListenerClass, "callback",
                "(L"PACKAGE"/utils/LightduerContext;L"PACKAGE"/utils/LightduerMessage;L"PACKAGE"/utils/LightduerAddress;)I");

        jint result = (*jenv)->CallIntMethod(jenv, s_resource_listener, callback,
                                             context, message, address);
        LOGI("result %d\n", result);
    } while (0);

    return DUER_OK;
}

static duer_status_t coap_dynamic_resource_callback_for_java(
        duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    LOGD(" in coap_dynamic_resource_callback_for_java\n");

    int needDetach = 0;

    jobject context = NULL;
    jobject message = NULL;
    jobject address = NULL;
    duer_status_t result = DUER_OK;

    JNIEnv *jenv = duer_jni_get_JNIEnv(&needDetach);

    do {
        if (jenv == NULL) {
            LOGE("can't get jenv\n");
            result = DUER_ERR_FAILED;
            break;
        }

        context = duer_jni_get_object_LightduerContext(jenv, ctx);
        if (context == NULL) {
            LOGE("context create fail\n");
            result =  DUER_ERR_FAILED;
            break;
        }

        message = duer_jni_get_object_LightduerMessage(jenv, msg);
        if (message == NULL) {
            LOGE("message create fail\n");
            result = DUER_ERR_FAILED;
            break;
        }

        address = duer_jni_get_object_LightduerAddress(jenv, addr);
        if (address == NULL) {
            LOGE("address create fail\n");
            result =  DUER_ERR_FAILED;
            break;
        }

        duer_jni_call_callback_LightduerResourceListener(jenv, context, message, address);
    } while (0);

    if (context) {
        (*jenv)->DeleteLocalRef(jenv, context);
    }
    if (message) {
        (*jenv)->DeleteLocalRef(jenv, message);
    }
    if (address) {
        (*jenv)->DeleteLocalRef(jenv, address);
    }

    if (needDetach) {
        duer_jni_detach_current_thread();
    }

    return result;
}

int duer_jni_get_native_duer_resources(JNIEnv *jenv,
                                       jobjectArray objArray,
                                       duer_res_t *resources,
                                       jsize arraylength)
{
    int result = DUER_ERR_FAILED;
    jobject lightduerResource = NULL;
    jclass lightduerResourceClass = NULL;
    bool error_happen = true;

    do {
        lightduerResource = (*jenv)->GetObjectArrayElement(jenv, objArray, 0);
        if (lightduerResource == NULL) {
            LOGE("get element 0 fail!\n");
            break;
        }

        lightduerResourceClass = (*jenv)->GetObjectClass(jenv, lightduerResource);
        if (lightduerResourceClass == NULL) {
            LOGE("get class for LightduerResource fail!\n");
            break;
        }

        jfieldID fieldMode = (*jenv)->GetFieldID(jenv, lightduerResourceClass, "mode", "I");
        if (fieldMode == NULL) {
            LOGE("get field mode failed!\n");
            break;
        }

        jfieldID fieldAllowed = (*jenv)->GetFieldID(jenv, lightduerResourceClass, "allowed", "I");
        if (fieldAllowed == NULL) {
            LOGE("get field allowed failed!\n");
            break;
        }

        jfieldID fieldPath = (*jenv)->GetFieldID(
                    jenv, lightduerResourceClass, "path", "Ljava/lang/String;");
        if (fieldPath == NULL) {
            LOGE("get field path failed!\n");
            break;
        }
        jfieldID  fieldStaticData = (*jenv)->GetFieldID(
                    jenv, lightduerResourceClass, "staticData", "[B");
        if (fieldStaticData == NULL) {
            LOGE("get field staticData failed!\n");
            break;
        }

        error_happen = false;
        for (int i = 0; i < arraylength; ++i) {
            jobject resource = (*jenv)->GetObjectArrayElement(jenv, objArray, i);
            jint mode = (*jenv)->GetIntField(jenv, resource, fieldMode);
            resources[i].mode = (duer_u8_t )mode;
            jint allowed = (*jenv)->GetIntField(jenv, resource, fieldAllowed);
            resources[i].allowed = (duer_u8_t )allowed;
            jstring path = (jstring) (*jenv)->GetObjectField(jenv, resource, fieldPath);
            //const char* c_path = (*jenv)->GetStringUTFChars(jenv, path, NULL);
            //LOGI("c_path:%s", c_path);
            //(*jenv)->ReleaseStringUTFChars(jenv, path, c_path);
            jsize pathLength = (*jenv)->GetStringUTFLength(jenv, path);
            char *newPath = (char *) DUER_MALLOC(pathLength + 1);
            if (newPath == NULL) {
                LOGE("alloc fail! %d\n", i);
                error_happen = true;
                break;
            }

            (*jenv)->GetStringUTFRegion(jenv, path, 0, pathLength, newPath);
            newPath[pathLength] = 0;
            resources[i].path = newPath;
            LOGD("newData: %d, path:%s\n", i, resources[i].path);
            if (mode == DUER_RES_MODE_STATIC) {
                jbyteArray staticData = (jbyteArray) (*jenv)->GetObjectField(jenv, resource, fieldStaticData);
                jsize dataLength = (*jenv)->GetArrayLength(jenv, staticData);
                jbyte *newData = (jbyte *) DUER_MALLOC(dataLength);
                if (newData == NULL) {
                    LOGE("newData alloc fail! %d\n", i);
                    error_happen = true;
                    break;
                }
                //LOGD("newData: %d, p:%p\n", i, newData);
                (*jenv)->GetByteArrayRegion(jenv, staticData, 0, dataLength, newData);
                resources[i].res.s_res.data = newData;
                resources[i].res.s_res.size = dataLength;
                (*jenv)->DeleteLocalRef(jenv, staticData);
            } else if (mode == DUER_RES_MODE_DYNAMIC) {
                resources[i].res.f_res = &coap_dynamic_resource_callback_for_java;
                duer_jni_controlpoint_init(jenv, resource);
            }
            (*jenv)->DeleteLocalRef(jenv, path);
            (*jenv)->DeleteLocalRef(jenv, resource);
        }
        if (!error_happen) {
            result = DUER_OK;
        }
    } while(0);

    if (!error_happen && result != DUER_OK) {
        LOGE("xxxxxxxxxxshould not enter here yyyyyyyyyyyyy\n");
    }

    return result;
}


