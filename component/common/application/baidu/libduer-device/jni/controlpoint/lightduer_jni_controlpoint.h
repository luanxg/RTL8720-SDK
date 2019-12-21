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

#ifndef BAIDU_DUER_LIGHTDUER_JNI_CONTROLPOINT_H
#define BAIDU_DUER_LIGHTDUER_JNI_CONTROLPOINT_H

#include <jni.h>
#include "lightduer_coap_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @param jenv
 *  @param obj, an object of type LightduerResourceListener
 *  @return, success DUER_OK, fail DUER_ERR_FAILED
 */
int duer_jni_controlpoint_init(JNIEnv *jenv, jobject obj);

int duer_jni_call_callback_LightduerResourceListener(JNIEnv *jenv,
                                                     jobject context,
                                                     jobject message,
                                                     jobject address);

/**
 *  @param jenv
 *  @param objArray, array of objects LightduerResource
 *  @param resources, output parameter
 *  @param arraylength, the length of the resources in resources and objArray
 *  @return success DUER_OK, fail DUER_ERR_FAILED
 *  @Note, the caller should release the new allocated memeory in resources.
 */
int duer_jni_get_native_duer_resources(JNIEnv *jenv,
                                       jobjectArray objArray,
                                       duer_res_t *resources,
                                       jsize arraylength);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIGHTDUER_JNI_CONTROLPOINT_H