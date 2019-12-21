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

CUSTOMER ?= RTK

COMPILER := arm
TOOLCHAIN ?= arm-none-eabi-

COM_DEFS := DUER_PLATFORM_RTK
CFLAGS := -DM3 -DGCC_ARMCM3 -mcpu=cortex-m3 -mthumb -g2 -w -Os -Wno-pointer-sign
CFLAGS += -fno-common -fmessage-length=0 -ffunction-sections -fdata-sections
CFLAGS += -fomit-frame-pointer -fno-short-enums -DF_CPU=166000000L -std=gnu99 -fsigned-char

CFLAGS += -std=c99
CFLAGS += -DDUER_COMPRESS_QUALITY=5

# open this if want to use the AES-CBC encrypted communication
#COM_DEFS += NET_TRANS_ENCRYPTED_BY_AES_CBC

#=====start modules select=======#
modules_module_Device_Info=y
modules_module_HTTP=y
modules_module_OTA=y
modules_module_coap=y
modules_module_connagent=y
modules_module_dcs=y
modules_module_ntp=y
modules_module_voice_engine=y

platform_module_platform=n
platform_module_port-freertos=y
platform_module_port-linux=n
platform_module_port-lpb100=n

module_framework=y

external_module_mbedtls=n
external_module_nsdl=y
external_module_cjson=y
external_module_speex=y
external_module_Zliblite=y
external_module_device_vad=y
#=====end  modules select=======#
