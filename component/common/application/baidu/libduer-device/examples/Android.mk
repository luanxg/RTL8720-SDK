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
##
# Build for demo
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := android-demo

LOCAL_STATIC_LIBRARIES := connagent coap \
                          voice_engine ntp platform jni dcs \
                          device_status System_Info framework \
                          nsdl speex mbedtls cjson

#LOCAL_SHARED_LIBRARIES := liblog

LOCAL_LDLIBS += -llog

MY_SRC_FILES := \
    $(LOCAL_PATH)/linux/duerapp_args.c \
    $(LOCAL_PATH)/linux/duerapp.c \
    $(LOCAL_PATH)/linux/duerapp_recorder.c \
    $(LOCAL_PATH)/linux/duerapp_profile_config.c

LOCAL_SRC_FILES := $(MY_SRC_FILES:$(LOCAL_PATH)/%=%)

include $(BUILD_EXECUTABLE)

