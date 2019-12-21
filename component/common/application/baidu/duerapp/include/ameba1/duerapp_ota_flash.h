#ifndef DUERAPP_FLASH_OTA_H
#define DUERAPP_FLASH_OTA_H

#include <FreeRTOS.h>
#include <task.h>
#include <platform_stdlib.h>
#include <flash_api.h>
#include <lwip/sockets.h>

#define DUER_SWAP_UPDATE 			1
#define DUER_WRITE_OTA_ADDR 		1
#define DUER_OFFSET_DATA			0x9000 		// reserve 32K+4K for Image1 + Reserved data
#define DUER_IMAGE_2				0x0000B000

#if DUER_WRITE_OTA_ADDR
#define DUER_BACKUP_SECTOR	(FLASH_SYSTEM_DATA_ADDR - 0x1000)
#endif


#endif


