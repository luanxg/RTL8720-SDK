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
# Build for demo
#

include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/examples/linux

LOCAL_MODULE := linux-demo

selected_modules := $(call get_current_selected_modules,$(lightduer_supported_modules))

LOCAL_STATIC_LIBRARIES := $(selected_modules)

LOCAL_SRC_FILES := \
    $(MODULE_PATH)/duerapp_args.c \
    $(MODULE_PATH)/duerapp.c \
    $(MODULE_PATH)/duerapp_recorder.c \
    $(MODULE_PATH)/duerapp_profile_config.c \
    $(MODULE_PATH)/duerapp_dcs.c

#    $(MODULE_PATH)/duerapp_aes_test.c \
#    $(MODULE_PATH)/duerapp.c \

LOCAL_INCLUDES += $(BASE_DIR)/platform/include
LOCAL_INCLUDES += $(BASE_DIR)/module/dcs

LOCAL_LDFLAGS := -lm -lrt -lpthread

include $(BUILD_EXECUTABLE)
