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

CUSTOMER ?= mtk

ifeq ($(strip $(OECORE_PATH)),)
$(error please config the environment variable OECORE_PATH \
        which point to the root directory of the cross-compiler poky)
endif

COMPILER := poky
CFLAGS := --sysroot=$(OECORE_PATH)/sysroots/aarch64-poky-linux
LDFLAGS := --sysroot=$(OECORE_PATH)/sysroots/aarch64-poky-linux

COM_DEFS := MBEDTLS_CONFIG_FILE=\"baidu_ca_mbedtls_config.h\"

TOOLCHAIN ?= aarch64-poky-linux-
CFLAGS += -D__COMPILER_AARCH64__
CFLAGS += -std=c99
CFLAGS += -DDUER_COMPRESS_QUALITY=5
CFLAGS += -D_GNU_SOURCE -lrt
CFLAGS += -DDISABLE_REPORT_SYSTEM_INFO

TARGET := tracker


# open this if want to use the AES-CBC encrypted communication
#COM_DEFS += NET_TRANS_ENCRYPTED_BY_AES_CBC

#=====start modules select=======#
modules_module_Device_Info=y
modules_module_System_Info=y
modules_module_HTTP=y
modules_module_OTA=y
modules_module_coap=y
modules_module_connagent=y
modules_module_dcs=y
modules_module_ntp=y
modules_module_voice_engine=y

platform_module_platform=n
platform_module_port-freertos=n
platform_module_port-linux=y
platform_module_port-lpb100=n

module_framework=y

external_module_mbedtls=y
external_module_nsdl=y
external_module_cjson=y
external_module_speex=y
external_module_device_vad=y
#=====end modules select=======#
