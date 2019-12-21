
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

ifeq ($(strip $(TARGET_COMPILE_ENV_OR_PRODUCT)),)
$(error please run source build/envsetup.sh and then use lightduerconfig to select the setting)
endif

ifeq ($(strip $(TARGET_PLATFORM_OR_VENDOR)),)
$(error please run source build/envsetup.sh and then use lightduerconfig to select the setting)
endif

ifeq ($(strip $(TARGET_BUILD_VARIANT)),)
$(error please run source build/envsetup.sh and then use lightduerconfig to select the setting)
endif

#pay attention to the order of the below module list
lightduer_supported_modules := \
                     modules_module_Device_Info \
                     modules_module_connagent \
                     modules_module_HTTP \
                     modules_module_OTA \
                     modules_module_flash_strings \
                     modules_module_coap \
                     modules_module_dcs \
                     modules_module_ntp \
                     modules_module_voice_engine \
                     modules_module_device_vad \
                     modules_module_device_status \
                     modules_module_interactive_class \
                     modules_module_bind_device \
                     platform_module_platform \
                     platform_module_port-freertos \
                     platform_module_port-linux \
                     platform_module_port-lpb100 \
                     platform_module_port-lpt230 \
                     modules_module_System_Info \
                     external_module_cjson \
                     module_framework \
                     external_module_mbedtls \
                     external_module_nsdl \
                     external_module_device_vad \
                     external_module_speex \
                     external_module_Zliblite

#BASE_DIR := $(shell pwd)
BASE_DIR := .

include $(BASE_DIR)/build/device/$(TARGET_PLATFORM_OR_VENDOR)/$(TARGET_COMPILE_ENV_OR_PRODUCT)/$(TARGET_BUILD_VARIANT).mk
include $(BASE_DIR)/build/device/moduleconfig.mk
OUT_DIR := $(BASE_DIR)/out/$(TARGET_PLATFORM_OR_VENDOR)/$(TARGET_COMPILE_ENV_OR_PRODUCT)/$(TARGET_BUILD_VARIANT)
CC := gcc
TARGET ?= duer-device

COM_DEFS += __TARGET_$(shell echo $(strip $(TARGET)) | tr a-z A-Z | sed 's/[^a-zA-Z0-9]/_/g')__

DEBUG ?= false
HIDE ?= @

DUER_DEBUG_LEVEL ?= 3
DUER_MEMORY_DEBUG ?= false
DUER_NSDL_DEBUG ?= false
DUER_MBEDTLS_DEBUG ?= 0

ifeq ($(strip $(external_module_mbedtls)),y)
MBEDTLS_SUPPORT := dtls tls
endif

BUILD_SYSTEM := $(BASE_DIR)/build/core
CLEAR_VAR := $(BUILD_SYSTEM)/clear_var.mak
BUILD_PRE := $(BUILD_SYSTEM)/build_pre.mak
BUILD_STATIC_LIB := $(BUILD_SYSTEM)/build_static_library.mak
BUILD_EXECUTABLE := $(BUILD_SYSTEM)/build_executable.mak

#MODULES := mbedtls nsdl

CFLAGS += -O2 -Wno-pointer-to-int-cast

# Config Zlib Use DUER_MALLOC & DUER_FREE
CFLAGS += -DZ_SOLO
# Config z_const as const
CFLAGS += -DZLIB_CONST

DUER_DEBUG_LEVEL_SUPPORT := 0 1 2 3 4 5

ifneq ($(filter $(DUER_DEBUG_LEVEL),$(DUER_DEBUG_LEVEL_SUPPORT)),)
COM_DEFS += DUER_DEBUG_LEVEL=$(strip $(DUER_DEBUG_LEVEL))

ifeq ($(strip $(DUER_MEMORY_DEBUG)),true)
COM_DEFS += DUER_MEMORY_DEBUG DUER_MEMORY_USAGE
endif

ifeq ($(strip $(DUER_NSDL_DEBUG)),true)
COM_DEFS += MBED_CONF_MBED_TRACE_ENABLE YOTTA_CFG_MBED_TRACE YOTTA_CFG_MBED_TRACE_FEA_IPV6=0
endif

ifneq ($(filter $(DUER_MBEDTLS_DEBUG),$(DUER_DEBUG_LEVEL_SUPPORT)),)
COM_DEFS += DUER_MBEDTLS_DEBUG=$(strip $(DUER_MBEDTLS_DEBUG))
else
$(error Please set the DUER_MBEDTLS_DEBUG with $(DUER_DEBUG_LEVEL_SUPPORT))
endif

CFLAGS += -g
else
$(warning The DUER_DEBUG_LEVEL not set, disable all debug features!!!)
endif

COM_DEFS += MBED_CONF_MBED_CLIENT_SN_COAP_MAX_BLOCKWISE_PAYLOAD_SIZE=1024

# open this if want to use the AES-CBC encrypted communication
#COM_DEFS += NET_TRANS_ENCRYPTED_BY_AES_CBC

ifneq ($(strip $(COM_DEFS)),)
COM_DEFS := $(foreach d,$(COM_DEFS),-D$(d))
endif

#print the selected modules
# $(1) the supported modulelist
define print_current_selected_modules
$(foreach module, $(1), \
    $(if $(filter y,$(value $(module))), \
        $(info $(shell echo $(module) | sed 's/.*module_//' ), $(value $(module)))) \
)
endef

#get the selected modules
# $(1) the supported modulelist
define get_current_selected_modules
$(foreach module, $(1), \
    $(if $(filter y,$(value $(module))), \
        $(shell echo $(module) | sed 's/.*module_//' )) \
)
endef

#print the unselected modules
# $(1) the supported modulelist
# 1. 'if' get the indicated 'n' modules
# 2. 'if' get the undefined modules
define print_current_unselected_modules
$(foreach module, $(1), \
    $(if $(filter n,$(value $(module))), \
        $(info $(shell echo $(module) | sed 's/.*module_//' ), $(value $(module)))) \
    $(if $(filter undefined,$(origin $(module))), \
        $(info $(shell echo $(module) | sed 's/.*module_//' ), $(value $(module)))) \
)
endef

.PHONY: clean run clean_all modules nomodules

all: $(TARGET)

run: run_$(TARGET)

clean: clean_$(TARGET)

clean_all:
	$(HIDE)rm -rf $(OUT_DIR)

modules:
	$(call print_current_selected_modules,$(lightduer_supported_modules))

nomodules:
	$(call print_current_unselected_modules,$(lightduer_supported_modules))

$(foreach mak,$(shell find $(BASE_DIR) -name duer.mak),$(eval include $(strip $(mak))))

