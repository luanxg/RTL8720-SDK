OTA Introduction
==================================================================================================

|               Author                 |          Change                            | Version |    Date    |
| :-: | :-: | :-: | :-: |
| Zhong Shuai (zhongshuai@baidu.com)   |        Initialise                          |  1.0.0  | 2017.11.30 |
| Zhong Shuai (zhongshuai@baidu.com)   |1. Add OTA Installer abstraction layer      |  2.0.0  | 2018.8.31  |
|                                      |2. Add OTA decompression abstraction layer  |         |            |
|                                      |3. Refactor Unpacker module                 |         |            |
|                                      |4. Add OTA Verifier module                  |         |            |

--------------------------------------------------------------------------------------------------

1. OTA Software Architecture
2. OTA Modules
    1. Updater
    2. Downloader
    3. Unpacker
    4. Notifier
    5. Installer
    6. Verifier
    7. Decompression
3. Updating Package
    1. Updating Package Structure
    2. Build Updating Package
4. OTA Useage
    1. Preparation
    2. Initialize OTA
5. Demo Code

--------------------------------------------------------------------------------------------------

1. OTA Software Architecture
![OTA_Software_Architecture](./OTA_Software_Architecture.png)

--------------------------------------------------------------------------------------------------

2. OTA Modules
    1. Updater
       Updater is a manager who will call other OTA modules to install updating package.

    2. Downloader
       Downloader is a abstraction layer to support a wide variety of download protocol.
        1. OTA HTTP Downloader
           OTA HTTP Downloader is a downloader to support HTTP protocol.
        2. OTA COAP Downloader
           OTA COAP Downloader is a downloader to support COAP protocol.

    3. Unpacker
        1. Unpacker module is used to unpack a OTA package which packed by Duer Cloud.
        2. It will parse the header information and meta information of the OTA package.
        (MD5, Version, size ...)
        3. It will call the OTA installer which the user registered based on the
        meta information.

    4. Notifier
       Notifier module is used to notify the Duer Cloud that the upgrade progress of the device.

    5. Installer
       Installer module is a abstraction layer which supports a variety of chips
       (Flash, DSP, MCU , Codec ...)
       If the user want to update DSP, the user need to implement a DSP installer
       and register it to the Installer module.
       The Unpacker will send the DSP firmware & the information of install
       (firmware size, version, MD5 ...) to the DSP installer.

    6. Verifier
       Verifier will use public key to check OTA package.
       The unpacker will send the MD5 of each module data to the installer.
       so the installer have to check the MD5 of the data which get from unpacker module by itself.

    7. Decompression
       Decompression is a decompression abstraction layer which is used to support various
       decompression libraries.

--------------------------------------------------------------------------------------------------

3. Updating Package
    1. Updating Package Structure

```

                                              0                                     31  Bit
                                    ---       ----------------------------------------
                                    |    Tag  | M         B            E           D |
                                    |         ----------------------------------------
                                    |         |             Header Size              |
                                    |         ----------------------------------------
                                    |         |       Package Signature Size         |
                                    |         ----------------------------------------
                                    |         |     Package Signature (128 Byte)     |
                                    |         |             ......                   |
                   Package Header <=          ----------------------------------------
                                    |         |        Meta Signature Size           |
                                    |         ----------------------------------------
                                    |         |       Meta Signature (128 Byte)      |
                                    |         |             ......                   |
                                    |         ----------------------------------------
                                    |         |             Meta Size                |
                                    |         ----------------------------------------
                                    |         |       Original Package Size          |
            ---                      ---      ----------------------------------------
            |                       |         |             APP Name                 |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |     Meta Basic Info <=          |            Update Flag               |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Package Type              |
            |                       |         |               ......                 |
            |                        ---      ----------------------------------------
            |                                 |        Package Install Path          |
            |                                 |               ......                 |
            |                        ---      ----------------------------------------
            |                       |         |            Module_1 Name             |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Module_1 Type             |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Module_1 Size             |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Module_1 Update Flag      |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Module_1 Signature        |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Module_1 Version          |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |       Module_1 Hardware Version      |
            |                       |         |               ......                 |
            |    Module List Info <=          ----------------------------------------
            |                       |         |            Module_2 Name             |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
Meta Data <=                        |         |            Module_2 Type             |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Module_2 Size             |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Module_2 Update Flag      |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Module_2 Signature        |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |            Module_2 Version          |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |       Module_2 Hardware Version      |
            |                       |         |               ......                 |
            |                       |         ----------------------------------------
            |                       |         |                Module_3              |
            |                       |         |                ......                |
            |                       |          ---------------------------------------
            |                       |         |                Module_4              |
            |                       |         |                ......                |
            |                        ---      ----------------------------------------
            |                                 |            Update Information        |
            |                                 |                ......                |
            |                                 ----------------------------------------
            |                                 |                 Feature              |
            |                                 |                  ......              |
             ---         ---                  ----------------------------------------
                        |                     |             Module_1 Data            |
                        |                     |                 ......               |
                        |                     ----------------------------------------
                        |                     |             Module_2 Data            |
                        |                     |                 ......               |
          Module Data <=                      ----------------------------------------
                        |                     |             Module_3 Data            |
                        |                     |                 ......               |
                        |                     ----------------------------------------
                        |                     |             Module_4 Data            |
                        |                     |                 ......               |
                         ---                  ----------------------------------------
```

    2. Build OTA Package
    Upload the firmware to the Duer Cloud, fill out a form with the information about firmware.
    Duer Cloud will build OTA package automatically.

-----------------------------------------------------------------------------------------------

4. OTA Useage
    1. Preparation
        1. Dependence
            1. The OTA HTTP Downloader depend on HTTP Client module, so Enable it.
               modules_module_HTTP=y
               Now HTTP downloader adapt LWIP, so link it. You can adapt to other network protocol
               stacks.
            2. The OTA Unpacker depend on Zliblite so Enable it.
               external_module_Zliblite=y
            3. The Zliblite depend on MbedTLS, so Enable it.
               external_module_mbedtls=y (If you use your own MbedTLS lib, Disable it)
            4. OTA Notifier depend on lightduer connagent.
               module_framework=y
               modules_module_coap=y
               modules_module_connagent=y
            5. OTA Unpack depend on cJSON
               modules_module_cjson=y
            6. The OTA depend on device info module
               modules_module_Device_Info=y

        2. Enable OTA
           Copy the following text to the configuration file(sdkconfig.mk)
           modules_module_OTA=y

    2. Initialize OTA
        1. Implement duer_ota_init_ops_t (See lightduer_ota_updater.h Line:67)
           Call duer_init_ota() to init OTA module after CA(lightduer connagent) start.

        2. Implement duer_ota_installer_t (See lightduer_ota_installer.h Line:34)

        3. Register duer_ota_installer_t
           Call duer_ota_installer_register_installer() to register a OTA installer.

        4. Implement duer_package_info_ops_t
           Call duer_ota_register_package_info_ops() to register the information of updating
           package.

        5. Implement struct DevInfoOps
           Call duer_register_device_info_ops() to register the information of device.

        6. Report Device information
           You have to call duer_report_device_info() to report the information of device to the
           Duer Cloud after CA start.

        7. Report OTA package information
           After the reboot of system you have to call duer_ota_notify_package_info() to report the
           information of OTA package to the Duer Cloud after CA start.

--------------------------------------------------------------------------------------------------

5. Demo code
```
#include <stdint.h>
#include <stdio.h>
#include "esp_system.h"
#include "esp_ota_ops.h"
#include "lightduer_lib.h"
#include "duerapp_config.h"
#include "lightduer_types.h"
#include "lightduer_memory.h"
#include "lightduer_ota_updater.h"
#include "lightduer_ota_unpacker.h"
#include "lightduer_ota_notifier.h"
#include "lightduer_ota_installer.h"
#include "mbedtls/md5.h"

#define FIRMWARE_VERSION "1.0.0.0"
#define CHIP_VERSION     "esp32"
#define SDK_VERSION      "1.0"

typedef struct _duer_ota_handler_s {
    esp_ota_handle_t update_handle;
    esp_partition_t const *update_partition;
    mbedtls_md5_context md5_ctx;
    duer_ota_module_info_t module_info;
    unsigned int firmware_size;
    int firmware_offset;
} duer_ota_handler_t;

static duer_package_info_t s_package_info = {
    .product = "ESP32 Demo",
    .batch   = "12",
    .os_info = {
        .os_name        = "FreeRTOS",
        .developer      = "Allen",
        .os_version     = "1.0.0.0", // This version is that you write in the Duer Cloud
        .staged_version = "1.0.0.0", // This version is that you write in the Duer Cloud
    }
};

static int notify_ota_begin(duer_ota_installer_t *installer, void **custom_data)
{
    esp_err_t err;
    int ret = DUER_OK;
    esp_ota_handle_t update_handle = 0 ;
    duer_ota_handler_t *ota_handler = NULL;
    esp_partition_t const *update_partition = NULL;

    DUER_LOGI("OTA Flash: Notify OTA begin");

    if (custom_data == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    do {
        ota_handler = (duer_ota_handler_t *)malloc(sizeof(*ota_handler));
        if (ota_handler == NULL) {

            DUER_LOGE("OTA Flash: Malloc failed");

            installer->err_msg = "Malloc OTA Handler failed";

            ret = DUER_ERR_MEMORY_OVERLOW;

            break;
        }

        update_partition = esp_ota_get_next_update_partition(NULL);
        if (update_partition != NULL) {
            DUER_LOGI("OTA Flash: Writing to partition subtype %d at offset 0x%x",
                    update_partition->subtype, update_partition->address);
        } else {
            DUER_LOGE("OTA Flash: Get update partition failed");

            installer->err_msg = "Get update partition failed";

            ret = DUER_ERR_FAILED;

            break;
        }

        err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
        if (err != ESP_OK) {
            DUER_LOGE("OTA Flash: Start ESP OTA failed, error = %d", err);

            installer->err_msg = "Start ESP OTA failed";

            ret = DUER_ERR_FAILED;
        }

        ota_handler->update_handle = update_handle;
        ota_handler->update_partition = update_partition;
        ota_handler->firmware_offset = 0;

        mbedtls_md5_init(&ota_handler->md5_ctx);
        mbedtls_md5_starts(&ota_handler->md5_ctx);

        *custom_data = ota_handler;

        return ret;
    } while (0);

    if (ota_handler != NULL) {
        DUER_FREE(ota_handler);
    }

    return ret;
}

static int get_module_info(
        duer_ota_installer_t *installer,
        void *custom_data,
        duer_ota_module_info_t const *info)
{
    int ret = DUER_OK;
    duer_ota_handler_t *ota_handler = NULL;

    if (custom_data == NULL || info == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    DUER_LOGI("OTA Flash: Module info");
    DUER_LOGI("Module name: %s", info->module_name);
    DUER_LOGI("Module size: %d", info->module_size);
    DUER_LOGI("Module signature: %s", info->module_signature);
    DUER_LOGI("Module version: %s", info->module_version);
    DUER_LOGI("Module HW version: %s", info->module_support_hardware_version);

    ota_handler = (duer_ota_handler_t *)custom_data;

    DUER_MEMMOVE(&ota_handler->module_info, info, sizeof(*info));

    ota_handler->firmware_size = info->module_size;

    return ret;
}

static int get_module_data(
        duer_ota_installer_t *installer,
        void *custom_data,
        unsigned int offset,
        unsigned char *data,
        unsigned int size)
{
    esp_err_t err;
    int ret = DUER_OK;
    esp_ota_handle_t update_handle = 0 ;
    duer_ota_handler_t *ota_handler = NULL;

    if (installer == NULL || custom_data == NULL || data == NULL || size <= 0) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ota_handler = (duer_ota_handler_t *)custom_data;
    update_handle = ota_handler->update_handle;

    mbedtls_md5_update(&ota_handler->md5_ctx, data, size);

    do {
        if (ota_handler->firmware_offset > ota_handler->firmware_size) {
            DUER_LOGE("OTA Flash: Buffer overflow");
            DUER_LOGE("OTA Flash: buf offset: %d", ota_handler->firmware_offset);
            DUER_LOGE("OTA Flash: firmware size: %d", ota_handler->firmware_size);

            installer->err_msg = "Buffer overflow";

            ret = DUER_ERR_FAILED;

            break;
        }

        err = esp_ota_write(update_handle, (const void *)data, size);
        if (err != ESP_OK) {
            DUER_LOGE("OTA Flash: Write OTA data failed err: 0x%x", err);

            installer->err_msg = "Write Flash failed";

            ret = DUER_ERR_FAILED;

            break;
        }

        ota_handler->firmware_offset += size;
    } while(0);

    return ret;
}

static int verify_module_data(duer_ota_installer_t *installer, void *custom_data)
{
    int i = 0;
    int ret = DUER_OK;
    char md5_buf[33];
    char *p_buf = md5_buf;
    size_t buf_len = 33;
    unsigned char md5[16];
    duer_ota_handler_t *ota_handler = NULL;

    DUER_LOGI("OTA Flash: Verify module data");

    if (custom_data == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ota_handler = (duer_ota_handler_t *)custom_data;

    mbedtls_md5_finish(&ota_handler->md5_ctx, md5);
    mbedtls_md5_free(&ota_handler->md5_ctx);

    for (i = 0; i < 16; i++) {
        buf_len -= DUER_SNPRINTF(p_buf, buf_len, "%02x", md5[i]);
        p_buf += 2;
    }
    *p_buf = '\0';

    ret = strncmp(
            (char const *)&ota_handler->module_info.module_signature,
            (char const *)md5_buf,
            32);
    if (ret != 0) {
        DUER_LOGE("OTA Flash: Checksum error failed");
        DUER_LOGE("OTA Flash: MD5: %s", ota_handler->module_info.module_signature);
        DUER_LOGE("OTA Flash: Checksum MD5: %s", md5_buf);

        installer->err_msg = "Verify module data failed";
    } else {
        DUER_LOGI("OTA Flash: Checksum successful");
    }

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
    esp_err_t err;
    int ret = DUER_OK;
    esp_ota_handle_t update_handle = 0 ;
    duer_ota_handler_t *ota_handler = NULL;

    DUER_LOGI("OTA Flash: Verify image");

    if (custom_data == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ota_handler = (duer_ota_handler_t *)custom_data;

    update_handle = ota_handler->update_handle;

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        DUER_LOGE("OTA Flash: End OTA failed err: %d", err);

        installer->err_msg = "Verify OTA image failed";

        ret = DUER_ERR_FAILED;
    }

    return ret;
}

static int notify_ota_end(duer_ota_installer_t *installer, void *custom_data)
{
    esp_err_t err;
    int ret = DUER_OK;
    duer_ota_handler_t *ota_handler = NULL;
    const esp_partition_t *update_partition = NULL;

    DUER_LOGI("OTA Flash: Notify OTA end");

    if (custom_data == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ota_handler = (duer_ota_handler_t *)custom_data;

    mbedtls_md5_free(&ota_handler->md5_ctx);

    update_partition = ota_handler->update_partition;

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        DUER_LOGE("OTA Flash: Set boot partition failed err = 0x%x", err);

        installer->err_msg = "Set boot partition failed";

        ret = DUER_ERR_FAILED;
    }

    DUER_FREE(ota_handler);

    return ret;
}

static int cancel_ota_update(duer_ota_installer_t *installer, void *custom_data)
{
    duer_ota_handler_t *ota_handler = NULL;

    DUER_LOGI("OTA Flash: Cancel OTA update");

    if (custom_data == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ota_handler = (duer_ota_handler_t *)custom_data;

    DUER_FREE(ota_handler);

    return DUER_OK;
}

static int duer_ota_register_installer(void)
{
    int ret = DUER_OK;
    duer_ota_installer_t *installer = NULL;

    installer = duer_ota_installer_create_installer("in.bin");
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

    installer = duer_ota_installer_get_installer("in.bin");
    if (installer == NULL) {
        DUER_LOGE("OTA Flash: Get installer failed");

        return DUER_ERR_FAILED;
    }

    ret = duer_ota_installer_unregister_installer("in.bin");
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

    esp_restart();

    return ret;
}

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

    ret = duer_get_firmware_version(firmware_version, sizeof(firmware_version));
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

static duer_package_info_ops_t s_package_info_ops = {
    .get_package_info = get_package_info,
};

static duer_ota_init_ops_t s_ota_init_ops = {
    .register_installer = duer_ota_register_installer,
    .unregister_installer = duer_ota_unregister_installer,
    .reboot = reboot,
};

static int get_firmware_version(char *firmware_version)
{

    strncpy(firmware_version, FIRMWARE_VERSION, FIRMWARE_VERSION_LEN);

    return DUER_OK;
}

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

    strncpy(info->mac_address, "00:0c:29:f6:c8:24", MAC_ADDRESS_LEN);
    strncpy(info->wifi_ssid, "test", WIFI_SSID_LEN);

    return DUER_OK;
}

// See lightduer_dev_info.h Line: 55
static struct DevInfoOps dev_info_ops = {
    .get_firmware_version = get_firmware_version,
    .get_chip_version     = get_chip_version,
    .get_sdk_version      = get_sdk_version,
    .get_network_info     = get_network_info,
};
int duer_initialize_ota(void)
{
    int ret = DUER_OK;
    char firmware_version[VERSION_LEN];

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

static void duer_event_hook(duer_event_t *event)
{
    switch (event->_event) {
    case DUER_EVENT_STARTED:

        ret = duer_report_device_info();
        if (ret != DUER_OK) {
            DUER_LOGE("Report device info failed ret:%d", ret);
        }

        duer_initialize_ota();

        ret = duer_ota_notify_package_info();
        if (ret != DUER_OK) {
            DUER_LOGE("Report package info failed ret:%d", ret);
        }

        break;

    case DUER_EVENT_STOPPED:
        break;
    }
}

int main(void)
{
    int ret = DUER_OK;

    // ......

    duer_initialize();

    duer_set_event_callback(duer_event_hook);

    // ......

    ret = duer_register_device_info_ops(&dev_info_ops);
    if (ret != DUER_OK) {
        DUER_LOGE("Dev Info: Register dev ops failed");

        goto out;
    }

    // ......

out:
    return ret;
}
```
