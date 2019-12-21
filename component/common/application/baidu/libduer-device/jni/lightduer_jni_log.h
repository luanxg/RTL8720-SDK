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

#ifndef BAIDU_DUER_LIGHTDUER_JNI_LOG_H_
#define BAIDU_DUER_LIGHTDUER_JNI_LOG_H_

#include "android/log.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LOGE(...) \
          ((void)__android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__))

#define LOGW(...) \
          ((void)__android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__))

#define LOGI(...) \
          ((void)__android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__))

#define LOGD(...) \
          ((void)__android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__))

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIGHTDUER_JNI_LOG_H_
