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

# config the dependency relationship between the modules
# config the exclusive relationship between the modules

#=====relationship between modules =======#
#dependency for CA
ifeq ($(strip $(modules_module_connagent)),y)
ifeq ($(origin $(modules_module_device_status)), undefined)
modules_module_device_status=y
endif
endif

#exclusive relationship for platform
platform_select=0
ifeq ($(strip $(platform_module_port-freertos)),y)
$(eval platform_select=$(shell echo $$(($(platform_select)+1))))
endif
ifeq ($(strip $(platform_module_port-linux)),y)
$(eval platform_select=$(shell echo $$(($(platform_select)+1))))
endif
ifeq ($(strip $(platform_module_port-lpb100)),y)
$(eval platform_select=$(shell echo $$(($(platform_select)+1))))
endif
ifneq ($(platform_select), 1)
$(warning platform selected, $(platform_select))
$(error exactly one platform should be selected)
endif

#=====relationship between modules=======#