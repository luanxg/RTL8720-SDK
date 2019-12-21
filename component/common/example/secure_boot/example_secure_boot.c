#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "hal_efuse.h"
#include "efuse_api.h"
#include "osdep_service.h"
#include "device_lock.h"
#include "efuse_logical_api.h"
/*
=========================================================================
 SS Key : the super secure private key used for encryption in secure boot
 S Key : the secure encrypted hash key used for hash in secure boot
=========================================================================
*/
#define PRIV_KEY_LEN 32		// The SS key and S key length is 32 bytes

//these two keys are the same default keys used in SDK
const uint8_t susec_key[PRIV_KEY_LEN] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x5F
};
const uint8_t sec_key[PRIV_KEY_LEN] = {
    0x64, 0xA7, 0x43, 0x3F, 0xCF, 0x02, 0x7D, 0x19, 
    0xDD, 0xA4, 0xD4, 0x46, 0xEE, 0xF8, 0xE7, 0x8A,
    0x22, 0xA8, 0xC3, 0x3C, 0xB2, 0xC3, 0x37, 0xC0,
    0x73, 0x66, 0xC0, 0x40, 0x61, 0x2E, 0xE0, 0xF2
};

static void efuse_secure_boot_task(void *param)
{
    int ret;
    u8 i, write_buf[PRIV_KEY_LEN], read_buf[PRIV_KEY_LEN];

    dbg_printf("\r\nefuse secure boot: Test Start\r\n");

    /*
    Step 1: program the super secure private key on eFUSE for image encryption
    */
    // read SS key
    device_mutex_lock(RT_DEV_LOCK_EFUSE);
	for(i=0; i<PRIV_KEY_LEN; i+=8){
        dbg_printf("[%d]\t%02X %02X %02X %02X  %02X %02X %02X %02X\r\n",
        		i, read_buf[i], read_buf[i+1], read_buf[i+2], read_buf[i+3], read_buf[i+4], read_buf[i+5], read_buf[i+6], read_buf[i+7]);
    }
    ret = hal_susec_key_get(read_buf);
    device_mutex_unlock(RT_DEV_LOCK_EFUSE);
    if(ret < 0){
        dbg_printf("efuse ss key: read address and length error\r\n");
        goto exit;
    }
    for(i=0; i<PRIV_KEY_LEN; i+=8){
        dbg_printf("[%d]\t%02X %02X %02X %02X  %02X %02X %02X %02X\r\n",
        		i, read_buf[i], read_buf[i+1], read_buf[i+2], read_buf[i+3], read_buf[i+4], read_buf[i+5], read_buf[i+6], read_buf[i+7]);
    }

    // write SS key
    memset(write_buf, 0xFF, PRIV_KEY_LEN);
    if(1){ // fill your data
        for(i=0; i<PRIV_KEY_LEN; i++)
            write_buf[i] = susec_key[i];
    }
    if(0){ // write
        device_mutex_lock(RT_DEV_LOCK_EFUSE);
        ret = efuse_susec_key_write(write_buf);
        device_mutex_unlock(RT_DEV_LOCK_EFUSE);
        if(ret < 0){
            dbg_printf("efuse SS key: write address and length error\r\n");
            goto exit;
        }
        dbg_printf("\r\nWrite Done.\r\n");
    }else{
        dbg_printf("\r\nPlease make sure the key is correct before programming in efuse.\r\n");
    }
    dbg_printf("\r\n");
	
     //read SS key
    device_mutex_lock(RT_DEV_LOCK_EFUSE);
    for(i=0; i<PRIV_KEY_LEN; i+=8){
        dbg_printf("[%d]\t%02X %02X %02X %02X  %02X %02X %02X %02X\r\n",
        		i, read_buf[i], read_buf[i+1], read_buf[i+2], read_buf[i+3], read_buf[i+4], read_buf[i+5], read_buf[i+6], read_buf[i+7]);
    }
    ret = hal_susec_key_get(read_buf);
    device_mutex_unlock(RT_DEV_LOCK_EFUSE);
    if(ret < 0){
        dbg_printf("efuse ss key: read address and length error\r\n");
        goto exit;
    }
    for(i=0; i<PRIV_KEY_LEN; i+=8){
        dbg_printf("[%d]\t%02X %02X %02X %02X  %02X %02X %02X %02X\r\n",
            i, read_buf[i], read_buf[i+1], read_buf[i+2], read_buf[i+3], read_buf[i+4], read_buf[i+5], read_buf[i+6], read_buf[i+7]);
    }
    if(memcmp(write_buf, read_buf, PRIV_KEY_LEN)){
        dbg_printf("efuse ss key: read error\r\n");
        goto exit;
    }

    /*
    Step 2: program the secure hash key which is encrypted by SS key on eFUSE for image hashing
    */
    // read S key, only key index 0 is used for secure boot
    device_mutex_lock(RT_DEV_LOCK_EFUSE);
    ret = hal_sec_key_get(read_buf, 0, PRIV_KEY_LEN);
    device_mutex_unlock(RT_DEV_LOCK_EFUSE);
    if(ret < 0){
        dbg_printf("efuse s key: read address and length error\r\n");
        goto exit;
    }
    for(i=0; i<PRIV_KEY_LEN; i+=8){
        dbg_printf("[%d]\t%02X %02X %02X %02X  %02X %02X %02X %02X\r\n",
            i, read_buf[i], read_buf[i+1], read_buf[i+2], read_buf[i+3], read_buf[i+4], read_buf[i+5], read_buf[i+6], read_buf[i+7]);
    }
	
    // write S key
    memset(write_buf, 0xFF, PRIV_KEY_LEN);
    if(1){ // fill your data
        for(i=0; i<PRIV_KEY_LEN; i++)
            write_buf[i] = sec_key[i];
    }
    if(0){ // write
        device_mutex_lock(RT_DEV_LOCK_EFUSE);
        ret = efuse_sec_key_write(write_buf, 0);
        device_mutex_unlock(RT_DEV_LOCK_EFUSE);
        if(ret < 0){
            dbg_printf("efuse S key: write address and length error\r\n");
            goto exit;
        }
        dbg_printf("\r\nWrite Done.\r\n");
    }else{
        dbg_printf("\r\nPlease make sure the key is correct before programming in efuse.\r\n");
    }
    dbg_printf("\r\n");

     //read S key
    device_mutex_lock(RT_DEV_LOCK_EFUSE);
    ret = hal_sec_key_get(read_buf, 0, PRIV_KEY_LEN);
    device_mutex_unlock(RT_DEV_LOCK_EFUSE);
    if(ret < 0){
        dbg_printf("efuse s key: read address and length error\r\n");
        goto exit;
    }
    for(i=0; i<PRIV_KEY_LEN; i+=8){
        dbg_printf("[%d]\t%02X %02X %02X %02X  %02X %02X %02X %02X\r\n",
            i, read_buf[i], read_buf[i+1], read_buf[i+2], read_buf[i+3], read_buf[i+4], read_buf[i+5], read_buf[i+6], read_buf[i+7]);
    }
    dbg_printf("efuse secure boot keys: Test Done\r\n");

    /*
    Step 3: lock and protect the SS key from being read by CPU
    */
    // lock SS key, make SS key unreadable forever.
    // this configure is irreversible, so please do this only if you are certain about SS key
    if(0){
        device_mutex_lock(RT_DEV_LOCK_EFUSE);
        ret = efuse_lock_susec_key();
        device_mutex_unlock(RT_DEV_LOCK_EFUSE);
        if(ret < 0){
            dbg_printf("efuse SS key lock error\r\n");
            goto exit;
        }
    }

    /*
    Step 4: enable the secure boot so that device will only boot with encrypted image
    */
    // enable secure boot, make device boot only with correctly encrypted image
    // this configure is irreversible, so please do this only if you are certain that the fw image is encrypted and hashed with the correct SS key and S key
    if(0){
        device_mutex_lock(RT_DEV_LOCK_EFUSE);
        ret = efuse_fw_verify_enable();
        device_mutex_unlock(RT_DEV_LOCK_EFUSE);
        if(ret < 0){
            dbg_printf("efuse secure boot enable error\r\n");
            goto exit;
        }
        device_mutex_lock(RT_DEV_LOCK_EFUSE);
        ret = efuse_fw_verify_check();
        device_mutex_unlock(RT_DEV_LOCK_EFUSE);
        if(ret)
            dbg_printf("secure boot is enabled!");
    }
    vTaskDelete(NULL);

exit:
    dbg_printf("efuse secure boot: Test Fail!\r\n");
    vTaskDelete(NULL);
}

void example_secure_boot(void)
{
    if(xTaskCreate(efuse_secure_boot_task, ((const char*)"efuse_secure_boot_task"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
        printf("\r\n\r%s xTaskCreate(efuse_secure_boot_task) failed", __FUNCTION__);

    /*Enable Schedule, Start Kernel*/
    if(rtw_get_scheduler_state() == OS_SCHEDULER_NOT_STARTED)
        vTaskStartScheduler();
    else
        vTaskDelete(NULL);
}