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
# Build for mbedtls
#

ifneq ($(strip $(MBEDTLS_SUPPORT)),)

include $(CLEAR_VARS)

LOCAL_MODULE := mbedtls

LOCAL_STATIC_LIBRARIES := framework

MY_SRC_FILES := $(wildcard $(LOCAL_PATH)/mbedtls/library/*.c)

LOCAL_SRC_FILES := $(MY_SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_CFLAGS += -DMBEDTLS_CONFIG_FILE=\"baidu_ca_mbedtls_config.h\"

LOCAL_C_INCLUDES := $(LOCAL_PATH)/mbedtls/library \
                  $(LOCAL_PATH)/mbedtls/include \
                  $(LOCAL_PATH)/mbedtls-port

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/mbedtls/library \
                  $(LOCAL_PATH)/mbedtls/include \
                  $(LOCAL_PATH)/mbedtls-port

ifeq ($(filter dtls,$(MBEDTLS_SUPPORT)),dtls)
  LOCAL_CFLAGS += -DMBEDTLS_DTLS
endif

ifeq ($(filter tls,$(MBEDTLS_SUPPORT)),tls)
  LOCAL_CFLAGS += -DMBEDTLS_TLS
endif

include $(BUILD_STATIC_LIBRARY)

endif

##
# Build for mbed-client-c
#

include $(CLEAR_VARS)

LOCAL_MODULE := nsdl

#LOCAL_STATIC_LIBRARIES := common

MY_SRC_FILES := $(wildcard $(LOCAL_PATH)/mbed-client-c/source/libCoap/src/*.c)
MY_SRC_FILES += $(wildcard $(LOCAL_PATH)/mbed-client-c/source/libNsdl/src/*.c)

LOCAL_SRC_FILES := $(MY_SRC_FILES:$(LOCAL_PATH)/%=%)

ifneq ($(strip $(DUER_NSDL_DEBUG)),false)
  LOCAL_SRC_FILES += $(LOCAL_PATH)/mbed-trace/source/mbed_trace.c
endif

#add this flag to enable inline, more info see ns_list.h
LOCAL_CFLAGS += -O3

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/mbed-client-c/nsdl-c \
    $(LOCAL_PATH)/mbed-client-c/source/libCoap/src/include \
    $(LOCAL_PATH)/mbed-client-c/source/libNsdl/src/include \
    $(LOCAL_PATH)/mbed-trace \
    $(LOCAL_PATH)/mbed-client-c-port

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/mbed-client-c/nsdl-c \
    $(LOCAL_PATH)/mbed-client-c/source/libCoap/src/include \
    $(LOCAL_PATH)/mbed-trace \
    $(LOCAL_PATH)/mbed-client-c-port

include $(BUILD_STATIC_LIBRARY)

##
# Build for cJSON
#

include $(CLEAR_VARS)

MODULE_PATH := baidu_json

LOCAL_MODULE := cjson

LOCAL_SRC_FILES := $(MODULE_PATH)/baidu_json.c

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/$(MODULE_PATH)

include $(BUILD_STATIC_LIBRARY)

#
# Build for speex
#
include $(CLEAR_VARS)

MODULE_PATH := speex

LOCAL_MODULE := speex

LOCAL_STATIC_LIBRARIES := framework

MY_SRC_FILES := \
    $(wildcard $(LOCAL_PATH)/$(MODULE_PATH)/libspeex/*.c) \
    $(wildcard $(LOCAL_PATH)/speex-port/*.c)

LOCAL_SRC_FILES := $(MY_SRC_FILES:$(LOCAL_PATH)/%=%)

#$(warning $(LOCAL_SRC_FILES))

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/speex/include \
    $(LOCAL_PATH)/$(MODULE_PATH)-port

LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/speex/include

LOCAL_CFLAGS += -DHAVE_CONFIG_H

include $(BUILD_STATIC_LIBRARY)

#
# Build for vad
#
include $(CLEAR_VARS)

LOCAL_MODULE := device_vad

#LOCAL_STATIC_LIBRARIES := connagent

MY_SRC_FILES := \
    $(wildcard $(LOCAL_PATH)/device_vad/*.c)

LOCAL_SRC_FILES := $(MY_SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/device_vad

include $(BUILD_STATIC_LIBRARY)
