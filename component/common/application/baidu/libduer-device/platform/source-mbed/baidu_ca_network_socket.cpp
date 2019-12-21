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
// Description: baidu_ca_network_socket

#include "baidu_ca_network_socket.h"
//#define SOCKETADAPTER_DEBUG

namespace duer {

NetworkInterface* SocketAdapter::_s_interface = NULL;

void SocketAdapter::set_network_interface(NetworkInterface* ifc) {
    _s_interface = ifc;
}

NetworkInterface* SocketAdapter::get_network_interface() {
    return _s_interface;
}

} // namespace duer

