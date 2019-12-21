#ifndef __EXAMPLE_IPV6_H__
#define __EXAMPLE_IPV6_H__

/******************************************************************************
 *
 * Copyright(c) 2007 - 2015 Realtek Corporation. All rights reserved.
 *
 *
 ******************************************************************************/
#include "FreeRTOS.h"
#include <autoconf.h>
#include "lwipopts.h"

#if defined(CONFIG_EXAMPLE_IPV6) && !defined(LWIP_IPV6)
#error "CONFIG_EXAMPLE_IPV6 defined, but LWIP_IPV6 is not defined"
#endif

#define MAX_RECV_SIZE    1500
#define MAX_SEND_SIZE    256
#define UDP_SERVER_PORT  5002
#define TCP_SERVER_PORT  5003
#define UDP_SERVER_IP    "fe80::5913:507e:2dbe:2ac1"
#define TCP_SERVER_IP    "fe80::5913:507e:2dbe:2ac1"

//MDNS
#define MCAST_GROUP_PORT 5353
#define MCAST_GROUP_IP   "ff02::fb"

#define IPV6_SEMA_TIMEOUT 15000

void example_ipv6(void);
void example_ipv6_callback(char* buf, int buf_len, int flags, void* userdata);

#endif //#ifndef __EXAMPLE_IPV6_H__
