#!/bin/bash

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

#ndk-build NDK_PROJECT_PATH=.  TARGET_PLATFORM=android-14 TARGET_ARCH=arm TARGET_ABI=android-14-armeabi-v7a TARGET_ARCH_ABI=armeabi-v7a APP_BUILD_SCRIPT=./Android.mk
#this is only a example to compile for android
#before run this script, ndk should be installed and ndk-build command should work.
ndk-build NDK_PROJECT_PATH=. NDK_MODULE_PATH=. NDK_APPLICATION_MK=./Application.mk APP_PLATFORM=android-14 APP_BUILD_SCRIPT=./Android.mk TARGET_OUT='out/android/$(TARGET_ARCH_ABI)' $@
#ndk-build NDK_PROJECT_PATH=.  APP_ABI=armeabi-v7a APP_PLATFORM=android-14 APP_BUILD_SCRIPT=./Android.mk TARGET_OUT='out/android/$(TARGET_ARCH_ABI)' $@
