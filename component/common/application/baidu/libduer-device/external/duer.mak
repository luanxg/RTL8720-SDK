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
# Build for mbedtls
#

ifneq ($(strip $(MBEDTLS_SUPPORT)),)

include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/external/mbedtls

LOCAL_MODULE := mbedtls

LOCAL_STATIC_LIBRARIES := framework

LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/library/*.c)

LOCAL_CDEFS := MBEDTLS_CONFIG_FILE=\"baidu_ca_mbedtls_config.h\"

LOCAL_INCLUDES := $(MODULE_PATH)/library \
                  $(MODULE_PATH)/include \
                  $(MODULE_PATH)-port

ifeq ($(filter dtls,$(MBEDTLS_SUPPORT)),dtls)
  LOCAL_CDEFS += MBEDTLS_DTLS
endif

ifeq ($(filter tls,$(MBEDTLS_SUPPORT)),tls)
  LOCAL_CDEFS += MBEDTLS_TLS
endif

include $(BUILD_STATIC_LIB)

endif

##
# Build for mbed-client-c
#

include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/external/mbed-client-c

LOCAL_MODULE := nsdl

#LOCAL_STATIC_LIBRARIES := common

LOCAL_SRC_FILES := \
  $(shell find $(MODULE_PATH)/source -name *.c)

ifneq ($(strip $(DUER_NSDL_DEBUG)),false)
  LOCAL_SRC_FILES += $(MODULE_PATH)/../mbed-trace/source/mbed_trace.c
endif

#add this flag to enable inline, more info see ns_list.h
#LOCAL_CFLAGS += -O2
LOCAL_SRC_FILES += $(MODULE_PATH)/../mbed-client-c-port/ns_list.c

LOCAL_INCLUDES := \
  $(MODULE_PATH)/nsdl-c \
  $(MODULE_PATH)/source/libCoap/src/include \
  $(MODULE_PATH)/source/libNsdl/src/include \
  $(MODULE_PATH)/../mbed-trace \
  $(MODULE_PATH)-port

include $(BUILD_STATIC_LIB)

##
# Build for cJSON
#

include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/external/baidu_json

LOCAL_MODULE := cjson

LOCAL_STATIC_LIBRARIES := framework

LOCAL_SRC_FILES := $(MODULE_PATH)/baidu_json.c

LOCAL_INCLUDES := $(MODULE_PATH)

include $(BUILD_STATIC_LIB)

#
# Build for device_vad
#

include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/external/device_vad

LOCAL_MODULE := device_vad

LOCAL_STATIC_LIBRARIES := framework

LOCAL_SRC_FILES := \
    $(wildcard $(MODULE_PATH)/*.c) \

LOCAL_INCLUDES := $(MODULE_PATH)

include $(BUILD_STATIC_LIB)

#
# Build for speex
#
include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/external/speex

LOCAL_MODULE := speex

LOCAL_STATIC_LIBRARIES := framework

LOCAL_SRC_FILES := \
    $(wildcard $(MODULE_PATH)/libspeex/*.c) \
    $(wildcard $(MODULE_PATH)/../speex-port/*.c)

LOCAL_INCLUDES := \
    $(MODULE_PATH)/include \
    $(MODULE_PATH)-port

LOCAL_CDEFS += HAVE_CONFIG_H

include $(BUILD_STATIC_LIB)

#
# Build for Zliblite
#
include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/external/Zliblite

LOCAL_MODULE := Zliblite

LOCAL_STATIC_LIBRARIES := framework

LOCAL_SRC_FILES := \
    $(wildcard $(MODULE_PATH)/*.c)

LOCAL_INCLUDES := $(MODULE_PATH)

include $(BUILD_STATIC_LIB)
