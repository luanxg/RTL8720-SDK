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
# Build for framework
#

include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/framework

LOCAL_MODULE := framework

LOCAL_STATIC_LIBRARIES := cjson mbedtls platform

#LOCAL_CDEFS := MBEDTLS_CONFIG_FILE=\"baidu_ca_mbedtls_config.h\"

LOCAL_SRC_FILES := \
    $(wildcard $(MODULE_PATH)/core/*.c) \
    $(wildcard $(MODULE_PATH)/utils/*.c)

LOCAL_INCLUDES := \
    $(MODULE_PATH)/include \
    $(MODULE_PATH)/core \
    $(MODULE_PATH)/utils

include $(BUILD_STATIC_LIB)
