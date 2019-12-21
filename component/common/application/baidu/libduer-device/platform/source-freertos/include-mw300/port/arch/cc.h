/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */
#ifndef __CC_H__
#define __CC_H__

#include <stdio.h>
#include <stdlib.h>

#include "cpu.h"
extern unsigned int os_get_timestamp();

#define U16_F "hu"
#define X16_F "hX"
#define U32_F "u"
#define X32_F "X"
#define S16_F "hd"
#define S32_F "d"


typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   long    u32_t;
typedef signed     long    s32_t;
typedef u32_t mem_ptr_t;
typedef int sys_prot_t;
extern int wmprintf(const char *format, ...);

#if defined(__GNUC__)
 #define PACK_STRUCT_BEGIN
 #define PACK_STRUCT_STRUCT __attribute__((packed))
 #define PACK_STRUCT_FIELD(x) x
#elif defined(__ICCARM__)
 #define PACK_STRUCT_BEGIN __packed
 #define PACK_STRUCT_STRUCT
 #define PACK_STRUCT_FIELD(x) x
#else
 #define PACK_STRUCT_BEGIN
 #define PACK_STRUCT_STRUCT
 #define PACK_STRUCT_FIELD(x) x
#endif

#define LWIP_PLATFORM_DIAG(_x_) do { unsigned int ts = os_get_timestamp(); \
												wmprintf("[%d.%06d] : ", ts/1000000, ts%1000000); \
												wmprintf _x_ ; \
												wmprintf("\r"); } while (0);
#define LWIP_PLATFORM_ASSERT(_x_) do { wmprintf( _x_ ); wmprintf("\n\r"); } while (0)
#define LWIP_PLATFORM_PRINT(_x_) do { wmprintf  _x_ ; } while (0)

#define LWIP_PLATFORM_BYTESWAP 1
/* TODO: Find optimized way of doing byte swap and implement it */
#define LWIP_PLATFORM_HTONS(x) (((x & 0xff) << 8) | ((x & 0xff00) >> 8))
#define LWIP_PLATFORM_HTONL(x) ((((x) & (0xff)) << 24) | (((x) & (0xff00)) << 8) | (((x) & (0xff0000UL)) >> 8) | (((x) & (0xff000000UL)) >> 24))
#define LWIP_RAND rand

#endif /* __CC_H__ */
