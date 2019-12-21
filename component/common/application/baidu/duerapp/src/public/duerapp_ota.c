#include "duerapp_ota.h"
#include "lightduer_lib.h"
#include "duerapp_config.h"
#include "lightduer_types.h"
#include "lightduer_memory.h"
#include "lightduer_ota_updater.h"
#include "lightduer_ota_unpacker.h"
#include "lightduer_ota_notifier.h"
#include "lightduer_ota_installer.h"

extern uint32_t UpdImg2Addr;
//extern uint32_t DefImg2Addr;
u8 OtaSign[8];

static duer_package_info_t s_package_info = {
	.product = "Rtk Demo",
	.batch   = "12",
	.os_info = {
		.os_name = "FreeRTOS",
		.developer = "rtk",
		.os_version = "1.0.0.0",
		.staged_version = "1.0.0.0",
	},
};

static int notify_ota_begin(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Notify OTA begin");

    return ret;
}

static int get_module_info(
        duer_ota_installer_t *installer,
        void **custom_data,
        duer_ota_module_info_t const *info)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Module info");
    DUER_LOGI("Module name: %s", info->module_name);
    DUER_LOGI("Module size: %d", info->module_size);
    DUER_LOGI("Module signature: %s", info->module_signature);
    DUER_LOGI("Module version: %s", info->module_version);
    DUER_LOGI("Module HW version: %s", info->module_support_hardware_version);

	duer_read_new_img2_addr();

	DUER_LOGI("###UpdImg2Addr 0x%x	OTALen: %d", UpdImg2Addr, info->module_size);

	duer_erase_sector_for_ota(UpdImg2Addr, info->module_size);

    return ret;
}

static int get_module_data(
        duer_ota_installer_t *installer,
        void *custom_data,
        unsigned int offset,
        unsigned char const *data,
        size_t size)
{
    int ret = DUER_OK;

	uint32_t address;

	//write to OTA here
	address = UpdImg2Addr;

	/*static int cnt = 0;

	if(cnt%100==0){
		DUER_LOGD("\n###write offset  0x%x\n",address + offset);
	}
	cnt++;*/
	// DUER_LOGI("[%s]address 0x%x offset 0x%x size %d", __FUNCTION__, address, offset, size);
	if(offset == 0) {
		memcpy(OtaSign, data, 8);
		ret = duer_flash_stream_write(address + offset + 8, size - 8, data+8);
	} else {
		ret = duer_flash_stream_write(address + offset, size, data);
	}
	if (ret != 1) {
		DUER_LOGE("OTA Flash: Write OTA data failed! err: 0x%x", ret);

		ret = DUER_ERR_FAILED;
	}

    return DUER_OK;
}

static int verify_module_data(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Verify module data");

    return ret;
}

static int update_img_begin(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Update image begin");

    return ret;
}

static int update_img(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Updating image");

    return ret;
}

static int verify_image(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Verify image");

	duer_start_upgrade();

    return ret;
}

static int notify_ota_end(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Notify OTA end");

    return ret;
}

static int cancel_ota_update(duer_ota_installer_t *installer, void *custom_data)
{
    DUER_LOGI("OTA Flash: Cancel OTA update");

    return DUER_OK;
}

static int duer_ota_register_installer(void)
{
    int ret = DUER_OK;
    duer_ota_installer_t *installer = NULL;

    installer = duer_ota_installer_create_installer("ota.bin");
    if (installer == NULL) {
        DUER_LOGE("OTA Flash: Create installer failed");

        return DUER_ERR_FAILED;
    }

    installer->notify_ota_begin   = notify_ota_begin,
    installer->notify_ota_end     = notify_ota_end,
    installer->get_module_info    = get_module_info,
    installer->get_module_data    = get_module_data,
    installer->verify_module_data = verify_module_data,
    installer->update_image_begin = update_img_begin,
    installer->update_image       = update_img,
    installer->verify_image       = verify_image,
    installer->cancel_ota_update  = cancel_ota_update;

    ret = duer_ota_installer_register_installer(installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Flash: Register installer failed");

        ret = duer_ota_installer_destroy_installer(installer);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Flash: Destroy installer failed");
        }

        return DUER_ERR_FAILED;
    }

    return ret;
}

static int duer_ota_unregister_installer(void)
{
    int ret = DUER_OK;
    duer_ota_installer_t *installer = NULL;

    installer = duer_ota_installer_get_installer("ota.bin");
    if (installer == NULL) {
        DUER_LOGE("OTA Flash: Get installer failed");

        return DUER_ERR_FAILED;
    }

    ret = duer_ota_installer_unregister_installer("ota.bin");
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Flash: Unregister installer failed");

        return DUER_ERR_FAILED;
    }

    ret = duer_ota_installer_destroy_installer(installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Flash: Destroy installer failed");

        return DUER_ERR_FAILED;
    }

    return ret;
}

static int reboot(void *arg)
{
	int ret = DUER_OK;

    DUER_LOGE("OTA Flash: Prepare to restart system");

	//duer_sleep(10000);
	//DUER_LOGI("###do reset");
	//sys_reset();
	return ret;
}

static duer_ota_init_ops_t s_ota_init_ops = {
    .register_installer = duer_ota_register_installer,
    .unregister_installer = duer_ota_unregister_installer,
    .reboot = reboot,
};

static int get_package_info(duer_package_info_t *info)
{
	int ret = DUER_OK;
	char firmware_version[FIRMWARE_VERSION_LEN + 1];

	if (info == NULL) {
		DUER_LOGE("Argument Error");

		ret = DUER_ERR_INVALID_PARAMETER;

		goto out;
	}

	memset(firmware_version, 0, sizeof(firmware_version));

	//ret = duer_get_firmware_version(firmware_version);
	ret = duer_get_firmware_version(firmware_version, FIRMWARE_VERSION_LEN);
	if (ret != DUER_OK) {
		DUER_LOGE("Get firmware version failed");

		goto out;
	}

	strncpy((char *)&s_package_info.os_info.os_version,
			firmware_version,
			FIRMWARE_VERSION_LEN + 1);
	memcpy(info, &s_package_info, sizeof(*info));

out:
	return ret;
}


// See lightduer_ota_notifier.h Line: 74
static duer_package_info_ops_t s_package_info_ops = {
	.get_package_info = get_package_info,
};


int duer_initialize_ota(void)
{
	int ret = DUER_OK;

	ret = duer_init_ota(&s_ota_init_ops);
	if (ret != DUER_OK) {
		DUER_LOGE("Init OTA failed");
	}

	ret = duer_ota_register_package_info_ops(&s_package_info_ops);
	if (ret != DUER_OK) {
		DUER_LOGE("Register OTA package info ops failed");
	}

	return ret;
}

static int get_firmware_version(char *firmware_version)
{

	strncpy(firmware_version, FIRMWARE_VERSION, FIRMWARE_VERSION_LEN);

	return DUER_OK;
}
/*
static int get_chip_version(char *chip_version)
{
	strncpy(chip_version, CHIP_VERSION, CHIP_VERSION_LEN);

	return DUER_OK;
}

static int get_sdk_version(char *sdk_version)
{
	strncpy(sdk_version, SDK_VERSION, SDK_VERSION_LEN);

	return DUER_OK;
}

static int get_network_info(struct NetworkInfo *info)
{
	info->network_type = WIFI;

	strncpy(info->mac_address, "f0:03:8c:e7:a9:66", MAC_ADDRESS_LEN);
	strncpy(info->wifi_ssid, "ADSL_WLAN", WIFI_SSID_LEN);

	return DUER_OK;
}*/

// See lightduer_dev_info.h Line: 55
static struct DevInfoOps dev_info_ops = {
	.get_firmware_version = get_firmware_version,
	/*.get_chip_version     = get_chip_version,
	.get_sdk_version      = get_sdk_version,
	.get_network_info     = get_network_info,*/
};

void initialize_deviceInfo(){
	int ret = duer_register_device_info_ops(&dev_info_ops);
	if (ret != DUER_OK) {
		DUER_LOGE("Dev Info: Register dev ops failed");
		return;
	}
}
