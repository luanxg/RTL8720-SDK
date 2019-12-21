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
CFLAGS += -DDUER_COMPRESS_QUALITY=5
#CFLAGS += -DDUER_VOICE_SEND_ASYNC
CFLAGS += -DENABLE_RECV_TIMEOUT_THRESHOLD
#CFLAGS += -DDISABLE_TCP_NODELAY
CFLAGS += -DDUER_DEBUG_AFTER_SEND
#default should not be 32 mode, or compile fail in CI env
#CFLAGS +=-m32
#LDFLAGS += -m32

# need more research
##CFLAGS += -DDUER_SEND_WITHOUT_FIRST_CACHE
#CFLAGS += -DDUER_HEAP_MONITOR
#CFLAGS += -DDUER_HEAP_MONITOR_DEBUG
CFLAGS += -DDUER_COAP_HEADER_SEND_FIRST
CFLAGS += -DDUER_BJSON_PRINT_WITH_ESTIMATED_SIZE
CFLAGS += -DDUER_BJSON_PREALLOC_ITEM
CFLAGS += -DDUER_EVENT_HANDLER
#CFLAGS += -DDUER_ENABLE_TASK_PRIORITY

#CFLAGS += -DLINUX_DEMO_TEST_ONLY_ONCE

TARGET := linux-demo

COM_DEFS := MBEDTLS_CONFIG_FILE=\"baidu_ca_mbedtls_config.h\"
COM_DEFS += DISABLE_REPORT_SYSTEM_INFO

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
modules_module_interactive_class=y

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

# open this if want to use the AES-CBC encrypted communication
#COM_DEFS += NET_TRANS_ENCRYPTED_BY_AES_CBC
