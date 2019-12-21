#ifndef DUERAPP_FLASH_OTA_H
#define DUERAPP_FLASH_OTA_H

#define FIRMWARE_VERSION "1.0.1.5"  //new version,update it when make new version
#define CHIP_VERSION     "RTL8721d"
#define SDK_VERSION      "1.0"


/*
 * Report device info to Duer Cloud
 *
 * @param info:
 *
 * @return int: Success: DUER_OK
 *              Failed:  Other
 */
extern int duer_report_device_info(void);

/*
 * Initialize DeviceInfo
 *
 * @param info:
 *
 * @return NULL
 */
extern void initialize_deviceInfo(void);

/*
 * Initialize DeviceInfo
 *
 * @param info:
 *
 * @return int: Success: DUER_OK
  * 			 Failed:  Other
  */
extern int duer_initialize_ota(void);


#endif

