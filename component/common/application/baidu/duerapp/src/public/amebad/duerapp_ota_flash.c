#include "FreeRTOS.h"
#include <device_lock.h>
#include "duerapp_ota_flash.h"

static flash_t flash_ota = {0};

uint32_t UpdImg2Addr;	/* New FW address */
uint32_t DefImg2Addr;	/* Old FW address */
static u8 ota_target_index;
extern u8 OtaSign[8];

int  duer_flash_read_word(uint32_t address, uint32_t * data)
{
	int ret;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	ret = flash_read_word(&flash_ota, address, data);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	return ret;
}

int  duer_flash_write_word(uint32_t address, uint32_t data)
{
	int ret;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	ret = flash_write_word(&flash_ota, address, data);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	return ret;
}

int duer_flash_stream_write(uint32_t address, uint32_t len, uint8_t * data)
{
	int ret = 0;
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	ret = flash_stream_write(&flash_ota, address, len, data);
	if(ret < 0){
		printf("\n\r[%s] Write sector failed", __FUNCTION__);
		device_mutex_unlock(RT_DEV_LOCK_FLASH);
		return ret;
	}
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return ret;
}

void  duer_flash_erase_sector(uint32_t address)
{
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_erase_sector(&flash_ota, address);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	return;
}

uint32_t duer_erase_sector_for_ota(uint32_t offset, uint32_t filelen)
{
	printf("\n offset:0x%x\n",offset);
	int i = 0;
	uint32_t fileBlkSize = ((filelen - 1) / 4096) + 1;
	for( i = 0; i < fileBlkSize; i++) {
		duer_flash_erase_sector(offset + i * 4096);
	}

	return 0;
}

/* Get OTA address (old & new FW addr) */
int duer_read_new_img2_addr()
{
	u32 AddrStart, Offset, IsMinus, PhyAddr;
	u8 cur_idx = OTA_INDEX_1;

	RSIP_REG_TypeDef* RSIP = ((RSIP_REG_TypeDef *) RSIP_REG_BASE);
	u32 CtrlTemp = RSIP->FLASH_MMU[0].MMU_ENTRYx_CTRL;

	if (CtrlTemp & MMU_BIT_ENTRY_VALID) {
		AddrStart = RSIP->FLASH_MMU[0].MMU_ENTRYx_STRADDR;
		Offset = RSIP->FLASH_MMU[0].MMU_ENTRYx_OFFSET;
		IsMinus = CtrlTemp & MMU_BIT_ENTRY_OFFSET_MINUS;

		if(IsMinus)
			PhyAddr = AddrStart - Offset;
		else
			PhyAddr = AddrStart + Offset;

		if(PhyAddr == DUER_IMG2_OTA1_ADDR)
			cur_idx = OTA_INDEX_1;
		else if(PhyAddr == DUER_IMG2_OTA2_ADDR)
			cur_idx = OTA_INDEX_2;
	}

	if(cur_idx == OTA_INDEX_1) {
		UpdImg2Addr = IMG_OTA2_ADDR;
		DefImg2Addr = IMG_OTA1_ADDR;
		ota_target_index = OTA_INDEX_2;
	} else {
		UpdImg2Addr = IMG_OTA1_ADDR;
		DefImg2Addr = IMG_OTA2_ADDR;
		ota_target_index = OTA_INDEX_1;
	}

	return 0;
}

/* Write signature, modify Valid_IMG2 */
int duer_start_upgrade()
{
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_write(&flash_ota, UpdImg2Addr, 8, OtaSign);
	flash_write_word(&flash_ota, DefImg2Addr, 0x0);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);

	return 0;
}
