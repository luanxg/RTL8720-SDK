#include <device_lock.h>
#include "duerapp_ota_flash.h"

static flash_t flash_ota = {0};

uint32_t UpdImg2Addr = 0xFFFFFFFF;
uint32_t DefImg2Addr = 0xFFFFFFFF;

#if DUER_WRITE_OTA_ADDR
int duer_write_ota_addr_to_system_data(flash_t *flash, uint32_t ota_addr)
{
	uint32_t data, i = 0;
	//Get upgraded image 2 addr from offset
	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_read_word(flash, DUER_OFFSET_DATA, &data);
	printf("\n\r[%s] data 0x%x ota_addr 0x%x", __FUNCTION__, data, ota_addr);
	if(~0x0 == data){
		flash_write_word(flash, DUER_OFFSET_DATA, ota_addr);
	}
	else{
		//erase backup sector
		flash_erase_sector(flash, DUER_BACKUP_SECTOR);
		//backup system data to backup sector
		for(i = 0; i < 0x1000; i+= 4){
			flash_read_word(flash, DUER_OFFSET_DATA + i, &data);
			if(0 == i)
				data = ota_addr;
			flash_write_word(flash, DUER_BACKUP_SECTOR + i,data);
		}
		//erase system data
		flash_erase_sector(flash, DUER_OFFSET_DATA);
		//write data back to system data
		for(i = 0; i < 0x1000; i+= 4){
			flash_read_word(flash, DUER_BACKUP_SECTOR + i, &data);
			flash_write_word(flash, DUER_OFFSET_DATA + i,data);
		}
		//erase backup sector
		flash_erase_sector(flash, DUER_BACKUP_SECTOR);
	}
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
	return 0;
}
#endif


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
	printf("\n\r[%s] offset:0x%x", __FUNCTION__, offset);
	int i = 0;
	uint32_t fileBlkSize = ((filelen - 1) / 4096) + 1;
	for( i = 0; i < fileBlkSize; i++) {
		duer_flash_erase_sector(offset + i * 4096);
	}

	return 0;
}

int duer_read_new_img2_addr()
{
	uint32_t NewImg2Addr = 0x0;
	uint32_t Img2Len = 0;
	uint32_t IMAGE_x = 0, ImgxLen = 0, ImgxAddr = 0;
#if DUER_WRITE_OTA_ADDR
	uint32_t ota_addr = 0xE0000;
#endif
	// The upgraded image2 pointer must 4K aligned and should not overlap with Default Image2
	duer_flash_read_word(DUER_IMAGE_2, &Img2Len);
	IMAGE_x = DUER_IMAGE_2 + Img2Len + 0x10;
	duer_flash_read_word(IMAGE_x, &ImgxLen);
	duer_flash_read_word(IMAGE_x+4, &ImgxAddr);
	if(ImgxAddr == 0x30000000)
		printf("\n\r[%s] IMAGE_3 0x%x Img3Len 0x%x.", __FUNCTION__, IMAGE_x, ImgxLen);
	else
	{
		printf("\n\r[%s] No IMAGE_3.", __FUNCTION__);
		// no image3
		IMAGE_x = DUER_IMAGE_2;
		ImgxLen = Img2Len;
	}
#if DUER_WRITE_OTA_ADDR
	if((ota_addr > IMAGE_x) && ((ota_addr < (IMAGE_x+ImgxLen))) ||
			(ota_addr < IMAGE_x) ||
			((ota_addr & 0xfff) != 0)||
			(ota_addr == 0xFFFFFFFF)){
		printf("\n\r[%s] Illegal ota addr 0x%x", __FUNCTION__, ota_addr);
		UpdImg2Addr = 0xFFFFFFFF;
		return -1;
	}else
		duer_write_ota_addr_to_system_data(&flash_ota, ota_addr);
#endif

	//Get upgraded image 2 addr from offset
	duer_flash_read_word(DUER_OFFSET_DATA, &NewImg2Addr);
	if(((NewImg2Addr > IMAGE_x) && (NewImg2Addr < (IMAGE_x+ImgxLen))) ||
			(NewImg2Addr < IMAGE_x) ||
			((NewImg2Addr & 0xfff) != 0)||
			(NewImg2Addr == 0xFFFFFFFF))
	{
		printf("\n\r[%s] Invalid OTA Address 0x%x.", __FUNCTION__, NewImg2Addr);
		UpdImg2Addr = 0xFFFFFFFF;
		return -1;
	}

	uint32_t SigImage0,SigImage1;
	uint32_t Part1Addr=0xFFFFFFFF, Part2Addr=0xFFFFFFFF, ATSCAddr=0xFFFFFFFF;
	uint32_t OldImg2Addr;
	duer_flash_read_word(0x18, &Part1Addr);

	Part1Addr = (Part1Addr&0xFFFF)*1024;	// first partition
	Part2Addr = NewImg2Addr;

	printf("\n\r[%s] Part1Addr:0x%x Part2Addr:0x%x", __FUNCTION__, Part1Addr, Part2Addr);

	// read Part1/Part2 signature
	duer_flash_read_word(Part1Addr+8, &SigImage0);
	duer_flash_read_word(Part1Addr+12, &SigImage1);

	printf("\n\r[%s] Part1 Sig %x.", __FUNCTION__, SigImage0);

	if(SigImage0==0x30303030 && SigImage1==0x30303030)
		ATSCAddr = Part1Addr;		// ATSC signature
	else if(SigImage0==0x35393138 && SigImage1==0x31313738)
		OldImg2Addr = Part1Addr;	// newer version, change to older version
	else
		NewImg2Addr = Part1Addr;	// update to older version

	duer_flash_read_word(Part2Addr+8, &SigImage0);
	duer_flash_read_word(Part2Addr+12, &SigImage1);
	printf("\n\r[%s] Part2 Sig %x.", __FUNCTION__, SigImage0);
	if(SigImage0==0x30303030 && SigImage1==0x30303030)
		ATSCAddr = Part2Addr;		// ATSC signature
	else if(SigImage0==0x35393138 && SigImage1==0x31313738)
		OldImg2Addr = Part2Addr;
	else
		NewImg2Addr = Part2Addr;

	// update ATSC clear partitin first
	if(ATSCAddr != 0xFFFFFFFF)
	{
		OldImg2Addr = NewImg2Addr;
		NewImg2Addr = ATSCAddr;
	}
	DefImg2Addr = OldImg2Addr;
	UpdImg2Addr = NewImg2Addr;
	if(DefImg2Addr == UpdImg2Addr)
	{
		printf("\r\n[%s] Error : DefImg2Addr == UpdImg2Addr", __func__);
		return -1;
	}
	else
	{
		printf("\n\r[%s] New %p, Old %p.", __FUNCTION__, UpdImg2Addr, DefImg2Addr);
		return 0;
	}
}


int duer_start_upgrade(){
	vTaskSuspendAll();

	int ret = -1 ;
	uint32_t NewImg2Addr = UpdImg2Addr;
	uint32_t OldImg2Addr = DefImg2Addr;

	printf("\n\r[%s] New 0x%x, Old 0x%x", __FUNCTION__, NewImg2Addr, OldImg2Addr);
	// compare checksum with received checksum
	//if(!memcmp(&checksum,file_info,sizeof(checksum))
	{
		//Set signature in New Image 2 addr + 8 and + 12
		uint32_t sig_readback0,sig_readback1;
		duer_flash_write_word(NewImg2Addr + 8, 0x35393138);
		duer_flash_write_word(NewImg2Addr + 12, 0x31313738);
		duer_flash_read_word(NewImg2Addr + 8, &sig_readback0);
		duer_flash_read_word(NewImg2Addr + 12, &sig_readback1);
		printf("\n\r[%s] signature %x,%x", __FUNCTION__ , sig_readback0, sig_readback1);
#if DUER_SWAP_UPDATE
		if(OldImg2Addr != ~0x0){

			duer_flash_read_word(OldImg2Addr + 8, &sig_readback0);
			duer_flash_read_word(OldImg2Addr + 12, &sig_readback1);
			printf("\n\r[%s] before: old signature %x,%x", __FUNCTION__ , sig_readback0, sig_readback1);

			duer_flash_write_word(OldImg2Addr + 8, 0x35393130);
			duer_flash_write_word(OldImg2Addr + 12, 0x31313738);
			duer_flash_read_word(OldImg2Addr + 8, &sig_readback0);
			duer_flash_read_word(OldImg2Addr + 12, &sig_readback1);
			printf("\n\r[%s] after: old signature %x,%x", __FUNCTION__ , sig_readback0, sig_readback1);
		}
#endif
		printf("\n\r[%s] Update OTA success!", __FUNCTION__);
		ret = 0;
	}
	if(!ret){
		printf("\n\r[%s] Ready to reboot", __FUNCTION__);
		//ota_platform_reset(); //chenwen modify
	}

	//sys_reset();
	xTaskResumeAll();

	return 0;
}
