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

CUSTOMER ?= linux

COMPILER := linux

CFLAGS += -D_GNU_SOURCE -lrt
CFLAGS += -std=c99

TARGET := speex-encoder

COM_DEFS := MBEDTLS_CONFIG_FILE=\"baidu_ca_mbedtls_config.h\"

CFLAGS += -DDISABLE_REPORT_SYSTEM_INFO

# open this if want to use the AES-CBC encrypted communication
#COM_DEFS += NET_TRANS_ENCRYPTED_BY_AES_CBC

#=====start modules select=======#
modules_module_Device_Info=n
modules_module_HTTP=n
modules_module_OTA=n
modules_module_coap=y
modules_module_connagent=y
modules_module_dcs=n
modules_module_ntp=n
modules_module_voice_engine=y
modules_module_device_status=y

platform_module_platform=n
platform_module_port-freertos=n
platform_module_port-linux=y
platform_module_port-lpb100=n

module_framework=y

external_module_mbedtls=y
external_module_nsdl=n
external_module_cjson=y
external_module_speex=y
external_module_device_vad=y
#=====end modules select=======#
