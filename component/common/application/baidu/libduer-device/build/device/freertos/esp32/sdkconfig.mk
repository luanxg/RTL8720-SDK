#
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

CUSTOMER ?= esp32

COMPILER := xtensa

TOOLCHAIN ?= xtensa-esp32-elf-

ifeq ($(strip $(IDF_PATH)),)
$(error please config the environment variable IDF_PATH \
        which point to the IDF root directory)
endif

ifeq ($(strip $(ESP32_TARGET_PATH)),)
$(error please config the environment variable ESP32_TARGET_PATH\
        which point to the esp32 project root directory which will\
        use the libduer-device.a)
endif

#CFLAGS += -DDUER_DEBUG_AFTER_SEND
#CFLAGS += -DDUER_HEAP_MONITOR
#CFLAGS += -DDUER_HEAP_MONITOR_DEBUG
##CFLAGS += -DDUER_SEND_WITHOUT_FIRST_CACHE // need more research
CFLAGS += -DDUER_COAP_HEADER_SEND_FIRST
CFLAGS += -DDUER_BJSON_PRINT_WITH_ESTIMATED_SIZE
CFLAGS += -DDUER_BJSON_PREALLOC_ITEM
CFLAGS += -DDUER_EVENT_HANDLER

CFLAGS += -DDUER_PLATFORM_ESPRESSIF -DCONFIG_TCP_OVERSIZE_MSS -DFIXED_POINT -DIDF_3_0
CFLAGS += -DDUER_COMPRESS_QUALITY=5 -DDUER_ASSIGN_TASK_NO=0
CFLAGS += -mlongcalls
CFLAGS += -DESP_PLATFORM
#CFLAGS += -DMBEDTLS_CONFIG_FILE=\"baidu_ca_mbedtls_config.h\"
#CFLAGS += -DDUER_HTTP_DNS_SUPPORT
#ENABLE_LIGHTDUER_SNPRINTF to avoid huge stack usage about snprintf on the esp32
CFLAGS += -DENABLE_LIGHTDUER_SNPRINTF
CFLAGS += -mfix-esp32-psram-cache-issue
CFLAGS += -DDUER_STATISTICS_E2E

CFLAGS += -DMBEDTLS_CONFIG_FILE=\"mbedtls/esp_config.h\"
CFLAGS += -I$(IDF_PATH)/components/mbedtls/port/include \
          -I$(IDF_PATH)/components/mbedtls/include \
          -I$(IDF_PATH)/components/esp32/include \
          -I$(ESP32_TARGET_PATH)/build/include


# open this if want to use the AES-CBC encrypted communication
#COM_DEFS += NET_TRANS_ENCRYPTED_BY_AES_CBC

#=====start modules select=======#
modules_module_Device_Info=y
modules_module_System_Info=y
modules_module_HTTP=y
modules_module_OTA=y
modules_module_flash_strings=y
modules_module_coap=y
modules_module_connagent=y
modules_module_dcs=y
modules_module_ntp=y
modules_module_voice_engine=y
modules_module_bind_device=y

platform_module_platform=n
platform_module_port-freertos=y
platform_module_port-linux=n
platform_module_port-lpb100=n

module_framework=y

external_module_mbedtls=n
external_module_nsdl=y
external_module_cjson=y
external_module_speex=y
external_module_device_vad=y
external_module_Zliblite=y
#=====end  modules select=======#
