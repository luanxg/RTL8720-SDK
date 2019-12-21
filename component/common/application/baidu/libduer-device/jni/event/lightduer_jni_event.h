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

#ifndef BAIDU_DUER_LIGHTDUER_JNI_EVENT_H
#define BAIDU_DUER_LIGHTDUER_JNI_EVENT_H

#include <jni.h>
#include "lightduer_connagent.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * init the callback object
 *  @param env
 *  @param obj, an object of type LightduerEventListener
 *  @return, success DUER_OK, fail DUER_ERR_FAILED
 */
int duer_jni_event_init(JNIEnv *jenv, jobject obj);

/**
 * call the callback of LightduerEventListener, which set in duer_jni_event_init
 * @param jenv
 * @param event
 */
void duer_jni_event_call_callback(JNIEnv *jenv, duer_event_t *event);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIGHTDUER_JNI_EVENT_H