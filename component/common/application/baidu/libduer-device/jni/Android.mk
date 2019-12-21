#
# Copyright (2017) Baidu Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

LOCAL_PATH := $(call my-dir)

##
# Build for jni
#

include $(CLEAR_VARS)

LOCAL_MODULE := jni

MY_SRC_FILES := \
    $(wildcard $(LOCAL_PATH)/*.c) \
    $(wildcard $(LOCAL_PATH)/controlpoint/*.c) \
    $(wildcard $(LOCAL_PATH)/dcs/*.c) \
    $(wildcard $(LOCAL_PATH)/event/*.c) \
    $(wildcard $(LOCAL_PATH)/utils/*.c) \
    $(wildcard $(LOCAL_PATH)/voice/*.c) \
    $(wildcard $(LOCAL_PATH)/vad/*.c)

LOCAL_SRC_FILES := $(MY_SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/controlpoint \
    $(LOCAL_PATH)/dcs \
    $(LOCAL_PATH)/event \
    $(LOCAL_PATH)/utils \
    $(LOCAL_PATH)/voice \
    $(LOCAL_PATH)/vad \

LOCAL_STATIC_LIBRARIES := connagent dcs voice_engine device_vad

include $(BUILD_STATIC_LIBRARY)
