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

#define TAG "lightduer_jni_voice"

#include "lightduer_jni_voice.h"
#include "lightduer_jni_log.h"
#include "lightduer_types.h"

struct VoiceResultCallback {
    jobject obj;
    jmethodID method;
};

static struct VoiceResultCallback s_voiceResultCB;

void duer_jni_voice_call_callback(JNIEnv *jenv, jstring jstr_result)
{
    (*jenv)->CallVoidMethod(jenv, s_voiceResultCB.obj, s_voiceResultCB.method, jstr_result);
}

int duer_jni_voice_init(JNIEnv *jenv, jobject obj)
{
    jclass vocice_result_callback_class = (*jenv)->GetObjectClass(jenv, obj);
    jmethodID callback = (*jenv)->GetMethodID(jenv, vocice_result_callback_class, "callback",
                                          "(Ljava/lang/String;)V");
    if (callback == NULL) {
        LOGE("method callback is not found!!\n");
        return DUER_ERR_FAILED;
    }

    s_voiceResultCB.obj = (*jenv)->NewGlobalRef(jenv, obj);
    s_voiceResultCB.method = callback;

    (*jenv)->DeleteLocalRef(jenv, vocice_result_callback_class);
    return DUER_OK;
}
