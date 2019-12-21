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
# Build for OTA
#
include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/modules/OTA

LOCAL_MODULE := OTA

LOCAL_STATIC_LIBRARIES := framework coap mbedtls cjson connagent Device_Info Zliblite HTTP System_Info platform

LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/Unpacker/*.c)
LOCAL_SRC_FILES += $(wildcard $(MODULE_PATH)/Downloader/*.c)
LOCAL_SRC_FILES += $(wildcard $(MODULE_PATH)/Updater/*.c)
LOCAL_SRC_FILES += $(wildcard $(MODULE_PATH)/Notifier/*.c)
LOCAL_SRC_FILES += $(wildcard $(MODULE_PATH)/Installer/*.c)
LOCAL_SRC_FILES += $(wildcard $(MODULE_PATH)/Verifier/*.c)
LOCAL_SRC_FILES += $(wildcard $(MODULE_PATH)/Decompression/*.c)

LOCAL_INCLUDES += $(MODULE_PATH) \
                  $(MODULE_PATH)/Unpacker \
                  $(MODULE_PATH)/Updater \
                  $(MODULE_PATH)/Downloader \
                  $(MODULE_PATH)/Installer \
                  $(MODULE_PATH)/Verifier \
                  $(MODULE_PATH)/Installer \
                  $(MODULE_PATH)/Decompression \
                  $(MODULE_PATH)/Notifier

ifeq ($(strip $(CUSTOMER)),esp32)
LOCAL_INCLUDES += $(IDF_PATH)/components/freertos/include \
                  $(IDF_PATH)/components/esp32/include \
                  $(IDF_PATH)/components/log/include \
                  $(BASE_DIR)/platform/source-freertos/include-esp32
endif

ifeq ($(strip $(CUSTOMER)),mw300)
LOCAL_INCLUDES += $(BASE_DIR)/platform/source-freertos/include-mw300
endif

include $(BUILD_STATIC_LIB)
