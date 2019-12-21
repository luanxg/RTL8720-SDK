#ifndef DUERAPP_FLASH_OTA_H
#define DUERAPP_FLASH_OTA_H

#include <FreeRTOS.h>
#include <task.h>
#include <platform_stdlib.h>
#include <flash_api.h>
#include <lwip/sockets.h>

#define IMG_OTA1_ADDR		0x00006000
#define IMG_OTA2_ADDR		0x00206000
#define DUER_IMG2_OTA1_ADDR	0x08006000				/* KM0 OTA1 start address*/
#define DUER_IMG2_OTA2_ADDR	0x08206000				/* KM0 OTA2 start address*/

#define OTA_INDEX_1			0
#define OTA_INDEX_2			1

#endif


