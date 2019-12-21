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

#define TAG "lightduer_jni"

#include "lightduer_jni.h"
#include "lightduer_jni_log.h"

static JavaVM *s_javavm = NULL;

extern int register_java_com_baidu_lightduer_lib_jni_LightduerOsJni(JNIEnv* env);

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    s_javavm = vm;
    JNIEnv *env = NULL;

    if (s_javavm) {
        LOGD("s_javavm init success");
    } else {
        LOGE("s_javavm init failed");
        return -1;
    }

    if ((*s_javavm)->GetEnv(s_javavm, (void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("get env fail!");
        return -1;
    }

    register_java_com_baidu_lightduer_lib_jni_LightduerOsJni(env);

    return JNI_VERSION_1_4;
}

JNIEnv* duer_jni_get_JNIEnv(int *neesDetach)
{
    JNIEnv *env = NULL;

    if ((*s_javavm)->GetEnv(s_javavm, (void**)&env, JNI_VERSION_1_4) != JNI_OK) {
        int status = (*s_javavm)->AttachCurrentThread(s_javavm, &env, 0);
        if (status < 0) {
            LOGE("failed to attach current thread!!");
            return env;
        }
        *neesDetach = 1;
    }

    return env;
}

void duer_jni_detach_current_thread()
{
    (*s_javavm)->DetachCurrentThread(s_javavm);
}
