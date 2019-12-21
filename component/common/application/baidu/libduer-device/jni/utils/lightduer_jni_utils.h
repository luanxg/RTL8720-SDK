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

#ifndef BAIDU_DUER_LIGHTDUER_JNI_UTILS_H
#define BAIDU_DUER_LIGHTDUER_JNI_UTILS_H

#include <jni.h>
#include "lightduer_coap_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * try to find the class object of LightduerContext/LightduerMessage/LightduerAddress
 *  @param env
 *  @return, success DUER_OK, fail DUER_ERR_FAILED
 */
int duer_jni_utils_init(JNIEnv *jenv);

/**
 * get an java object of LightduerContext from the native object
 * @param jenv
 * @param ctx native object
 * @return java object when success, or NULL
 */
jobject duer_jni_get_object_LightduerContext(JNIEnv *jenv, duer_context ctx);

/**
 * get an java object of LightduerMessage from the native object
 * @param jenv
 * @param msg native object
 * @return java object when success, or NULL
 */
jobject duer_jni_get_object_LightduerMessage(JNIEnv *jenv, duer_msg_t *msg);

/**
 * get an java object of LightduerAddress from the native object
 * @param jenv
 * @param addr native object
 * @return java object when success, or NULL
 */
jobject duer_jni_get_object_LightduerAddress(JNIEnv *jenv, duer_addr_t *addr);

/**
 * to create a native duer_msg_t object from the java object
 *  @param jenv
 *  @message, the java object, type is LightduerMessage
 *  @msg, the native duer_msg_t type, output parameter
 *  @return success DUER_OK, or DUER_ERR_FAILED .
 *  @Note the caller should take the responsibilility to release the below memory of msg
 *        msg->path, msg->query, msg->token, msg->payload
 */
int duer_jni_get_native_duer_message(JNIEnv *jenv, jobject message, duer_msg_t *msg);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIGHTDUER_JNI_UTILS_H