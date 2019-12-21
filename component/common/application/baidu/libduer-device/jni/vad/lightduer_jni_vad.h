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

#ifndef BAIDU_DUER_LIGHTDUER_JNI_VAD_H
#define BAIDU_DUER_LIGHTDUER_JNI_VAD_H

#include "lightduer_jni.h"

#ifdef __cplusplus
extern "C" {
#endif

jint lightduer_os_jni_init_device_vad(JNIEnv *env, jclass clazz, jobject jlistener);

jint lightduer_os_jni_set_vad_status(JNIEnv *env, jclass clazz, jint jstatus);

jint lightduer_os_jni_device_vad(JNIEnv *env, jclass clazz, jbyteArray jdata);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIGHTDUER_JNI_VAD_H
