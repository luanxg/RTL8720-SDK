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
// Description: The CoAP endpoint realization.

#include "lightduer_types.h"
#include "lightduer_lib.h"
#include "lightduer_network_defs.h"
#include "lightduer_coap_defs.h"
#include "lightduer_coap_ep.h"
#include "lightduer_memory.h"
#include "lightduer_ds_log_ca.h"

DUER_INT_IMPL duer_coap_ep_ptr duer_coap_ep_create(const char* name, const char* type,
                                                   duer_u32_t lifetime, const duer_addr_t* addr)
{
    duer_coap_ep_ptr ptr = DUER_MALLOC(sizeof(duer_coap_ep_t));

    if (ptr) {
        duer_size_t len;
        DUER_MEMSET(ptr, 0, sizeof(duer_coap_ep_t));

        if (name && (len = DUER_STRLEN(name)) > 0) {
            if ((ptr->name_ptr = DUER_MALLOC(len)) == NULL) {
                DUER_DS_LOG_CA_MEMORY_OVERFLOW();
                goto error;
            }

            DUER_MEMCPY(ptr->name_ptr, name, len);
            ptr->name_len = len;
        }

        if (type && (len = DUER_STRLEN(type)) > 0) {
            if ((ptr->type_ptr = DUER_MALLOC(len)) == NULL) {
                DUER_DS_LOG_CA_MEMORY_OVERFLOW();
                goto error;
            }

            DUER_MEMCPY(ptr->type_ptr, type, len);
            ptr->type_len = len;
        }

        if ((ptr->lifetime_ptr = DUER_MALLOC(LIFETIME_MAX_LEN)) == NULL) {
            DUER_DS_LOG_CA_MEMORY_OVERFLOW();
            goto error;
        }

        ptr->lifetime_len = DUER_SNPRINTF(ptr->lifetime_ptr, LIFETIME_MAX_LEN, "%d",
                                          lifetime);

        if (addr) {
            if ((ptr->address = DUER_MALLOC(sizeof(duer_addr_t))) == NULL) {
                DUER_DS_LOG_CA_MEMORY_OVERFLOW();
                goto error;
            }

            DUER_MEMCPY(ptr->address, addr, sizeof(duer_addr_t));

            if (addr->host) {
                ptr->address->host = DUER_MALLOC(addr->host_size);
                if (!ptr->address->host) {
                    DUER_DS_LOG_CA_MEMORY_OVERFLOW();
                    goto error;
                }

                DUER_MEMCPY(ptr->address->host, addr->host, addr->host_size);
            }
        }
    } else {
        DUER_DS_LOG_CA_MEMORY_OVERFLOW();
    }

exit:
    return ptr;
error:
    duer_coap_ep_destroy(ptr);
    ptr = NULL;
    goto exit;
}

DUER_INT_IMPL duer_status_t duer_coap_ep_destroy(duer_coap_ep_ptr ptr)
{
    if (ptr) {
        if (ptr->name_ptr) {
            DUER_FREE(ptr->name_ptr);
        }

        if (ptr->type_ptr) {
            DUER_FREE(ptr->type_ptr);
        }

        if (ptr->lifetime_ptr) {
            DUER_FREE(ptr->lifetime_ptr);
        }

        if (ptr->address) {
            if (ptr->address->host) {
                DUER_FREE(ptr->address->host);
            }

            DUER_FREE(ptr->address);
        }

        DUER_FREE(ptr);
    }

    return DUER_OK;
}
