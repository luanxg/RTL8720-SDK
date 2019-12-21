/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
// Author: Su Hao (suhao@baidu.com)
//
// Description: SocketAdapter

#ifndef BAIDU_IOT_TINYDU_IOT_OS_SRC_IOT_BAIDU_CA_ADAPTER_BAIDU_CA_NETWORK_SOCKET_H
#define BAIDU_IOT_TINYDU_IOT_OS_SRC_IOT_BAIDU_CA_ADAPTER_BAIDU_CA_NETWORK_SOCKET_H

#include "mbed.h"

namespace duer {

class SocketAdapter {
public:

    static void set_network_interface(NetworkInterface* ifc);

    static NetworkInterface* get_network_interface();

private:

    static NetworkInterface* _s_interface;
};

} // namespace duer

#endif // BAIDU_IOT_TINYDU_IOT_OS_SRC_IOT_BAIDU_CA_ADAPTER_BAIDU_CA_NETWORK_SOCKET_H
