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

#include "lightduer_jni_vad.h"
#include "lightduer_jni_log.h"
#include "lightduer_types.h"
#include "device_vad.h"

#define TAG "lightduer_jni_vad"

static jobject s_device_vad_listener = NULL;

jint lightduer_os_jni_init_device_vad(JNIEnv *env, jclass clazz, jobject jlistener)
{
    if (s_device_vad_listener != NULL) {
        (*env)->DeleteGlobalRef(env, s_device_vad_listener);
    }

    s_device_vad_listener = (*env)->NewGlobalRef(env, jlistener);
    if (s_device_vad_listener == NULL) {
        LOGE("initDeviceVad failed");
        return DUER_ERR_FAILED;
    }

    return DUER_OK;
}

jint lightduer_os_jni_set_vad_status(JNIEnv *env, jclass clazz, jint jstatus)
{
    switch (jstatus) {
    case 0:
        vad_set_wakeup();
        break;
    case 1:
        vad_set_playing();
        break;
    default:
        vad_set_wakeup();
        break;
    }

    return 0;
}

static void duer_vad_listener_callback(JNIEnv *env, const char *method_name)
{
    int need_detach = 0;
    jclass clazz = NULL;
    jmethodID method = NULL;

    do {
        clazz = (*env)->GetObjectClass(env, s_device_vad_listener);
        if (clazz == NULL) {
            LOGE("cant't get class of s_device_vad_listener");
            break;
        }

        method = (*env)->GetMethodID(env, clazz, method_name, "()V");
        if (method == NULL) {
            LOGE("method %s is not found!!\n", method_name);
            break;
        }

        (*env)->CallVoidMethod(env, s_device_vad_listener, method);
    } while (0);

    if (clazz != NULL) {
        (*env)->DeleteLocalRef(env, clazz);
    }
}

jint lightduer_os_jni_device_vad(JNIEnv *env, jclass clazz, jbyteArray jdata)
{
    jbyte *data = (*env)->GetByteArrayElements(env, jdata, 0);
    jsize size = (*env)->GetArrayLength(env, jdata);

    jint result = device_vad((char *)data, size);


    if (vad_stop_speak()){
        duer_vad_listener_callback(env, "onSpeakStop");
        vad_stop_speak_done();
    }

    (*env)->ReleaseByteArrayElements(env, jdata, data, 0);

    return result;
}
