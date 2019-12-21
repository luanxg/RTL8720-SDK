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

BASE_DIR=$(pwd)/..
HEAD_FOLDER="sdk_headers"

rm -rf ${HEAD_FOLDER}
mkdir ${HEAD_FOLDER}

find ${BASE_DIR}/modules -name \*.h -exec cp {} ${HEAD_FOLDER} \;
cp ${BASE_DIR}/external/baidu_json/baidu_json.h ${HEAD_FOLDER}
cp ${BASE_DIR}/framework/include/* ${HEAD_FOLDER}
cp ${BASE_DIR}/framework/core/*.h ${HEAD_FOLDER}
cp ${BASE_DIR}/platform/include/* ${HEAD_FOLDER}
