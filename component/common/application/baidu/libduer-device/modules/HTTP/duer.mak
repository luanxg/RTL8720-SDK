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

#
# Build for HTTP
#
include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/modules/HTTP

LOCAL_MODULE := HTTP

LOCAL_STATIC_LIBRARIES := framework coap connagent cjson voice_engine platform device_status

LOCAL_INCLUDES +=   $(MODULE_PATH) \
                    $(BASE_DIR)/platform/source-freertos \

ifeq ($(CUSTOMER),linux)
LOCAL_SRC_FILES := $(MODULE_PATH)/lightduer_http_client.c
LOCAL_SRC_FILES += $(MODULE_PATH)/lightduer_http_client_ops.c
LOCAL_SRC_FILES += $(MODULE_PATH)/lightduer_http_dns_client_ops.c
endif

ifeq ($(CUSTOMER),RTK)
LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/lightduer_http_client.c)
endif

ifeq ($(CUSTOMER),mtk)
LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/lightduer_http_client.c)
endif

ifeq ($(CUSTOMER),esp32)
LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/*.c)

LOCAL_INCLUDES += $(IDF_PATH)/components/freertos/include \
                  $(IDF_PATH)/components/esp32/include \
                  $(IDF_PATH)/components/log/include \
                  $(IDF_PATH)/components/soc/esp32/include \
                  $(IDF_PATH)/components/lwip/include/lwip \
                  $(IDF_PATH)/components/lwip/include/lwip/port \
                  $(IDF_PATH)/components/vfs/include \
                  $(IDF_PATH)/components/driver/include \
                  $(IDF_PATH)/components/heap/include \
                  $(IDF_PATH)/components/soc/include \
                  $(BASE_DIR)/platform/source-freertos/include-esp32
endif

ifeq ($(CUSTOMER),mw300)

LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/*.c)

LOCAL_INCLUDES += $(BASE_DIR)/platform/source-freertos/include-mw300 \
                  $(BASE_DIR)/platform/source-freertos/include-mw300/freertos \
                  $(BASE_DIR)/platform/source-freertos/include-mw300/port \
                  $(BASE_DIR)/platform/source-freertos/include-mw300/lwip \
                  $(BASE_DIR)/platform/source-freertos/include-mw300/ipv4 \
                  $(BASE_DIR)/platform/source-freertos/include-mw300/ipv6
endif

ifeq ($(CUSTOMER),esp8266)
LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/*.c)

LOCAL_INCLUDES += $(SDK_PATH)/include/lwip \
                  $(SDK_PATH)/include/espressif/ \
                  $(SDK_PATH)/include/lwip/ipv4/ \
                  $(SDK_PATH)/include/lwip/ipv6/ \
                  $(SDK_PATH)/include/ \
                  $(SDK_PATH)/extra_include/
endif

include $(BUILD_STATIC_LIB)
