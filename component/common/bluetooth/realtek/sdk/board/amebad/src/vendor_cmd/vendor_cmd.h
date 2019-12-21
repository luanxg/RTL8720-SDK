#ifndef VNR_CMD_H
#define VNR_CMD_H

#include <gap.h>

//mailbox
#define HCI_VENDOR_ENABLE_PROFILE_REPORT_COMMAND        0xfc18
#define HCI_VENDOR_SET_PROFILE_REPORT_COMMAND           0xfc19
#define HCI_VENDOR_MAILBOX_CMD                          0xfc8f

//sub event from fw start
#define HCI_VENDOR_PTA_REPORT_EVENT         0x24
#define HCI_VENDOR_PTA_AUTO_REPORT_EVENT    0x25

//le vendor cmd
#define HCI_LE_VENDOR_EXTENSION_FEATURE         0xFC87
#define HCI_EXT_SUB_ONE_SHOT_ADV                1

bool mailbox_to_bt(uint8_t *data, uint8_t len);

bool mailbox_to_bt_set_profile_report(uint8_t *data, uint8_t len);

T_GAP_CAUSE le_vendor_one_shot_adv(void);

void vendor_cmd_init(P_FUN_GAP_APP_CB app_cb);

#endif /* VNR_CMD_H */
