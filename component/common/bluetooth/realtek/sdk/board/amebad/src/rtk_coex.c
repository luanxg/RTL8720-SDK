/**
 * Copyright (c) 2017, Realsil Semiconductor Corporation. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>

#include "trace_app.h"
#include "vendor_cmd.h"
#include "rtk_coex.h"

struct rtl_btinfo
{
    uint8_t cmd;
    uint8_t len;
    uint8_t data[6];
};

static void rtk_notify_info_to_wifi(uint8_t length, uint8_t *report_info)
{
    uint8_t para_length = 1 + length;
    uint8_t buf[para_length ];
    uint8_t *p = buf;
    struct rtl_btinfo *report = (struct rtl_btinfo *)(report_info);

    *p++ = report->cmd;
    //*p++ = 0x07;
    *p++ = report->len;
    if (length)
    {
        memcpy(p, report->data, length);
    }
    if (length)
    {
        HCI_PRINT_TRACE1("bt info: cmd %2.2X", report->cmd);
        HCI_PRINT_TRACE1("bt info: len %2.2X", report->len);
        HCI_PRINT_TRACE6("bt info: data %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X",
                         report->data[0], report->data[1], report->data[2],
                         report->data[3], report->data[4], report->data[5]);
    }

    /*
    hci_board_debug("\r rtk_notify_info_to_wifi: ");
    for(int i = 0 ;i <para_length; i++)
    {
        hci_board_debug(" %x", buf[i]);
    }
    */
	//mailbox_to_wifi(buf, para_length);
    //rtk_btcoex_send_msg_towifi(buf, para_length + HCI_CMD_PREAMBLE_SIZE);
   // mailbox_to_wifi(buf, para_length + HCI_CMD_PREAMBLE_SIZE);
    /* send BT INFO to Wi-Fi driver */
}

void bt_coex_handle_cmd_complete_evt(uint16_t opcode, uint16_t cause, uint8_t total_len, uint8_t *p)
{

    if (opcode == HCI_VENDOR_MAILBOX_CMD)
    {
        uint8_t status, subcmd;
        status = *p++;//jump the double subcmd
        total_len--;
        if(total_len <=1)
        {
            HCI_PRINT_TRACE0("bt_coex_handle_cmd_complete_evt: not reprot to wifi");
            return ;
        }

/*
        hci_board_debug("\r rtk_gap_handle_cmd_complete_evt: %x",total_len);
        for(int i = 0 ;i <total_len; i++)
        {
            hci_board_debug("%x", p[i]);
        }
        */
       // mailbox_to_wifi(p, total_len);
        //rtk_parse_vendor_mailbox_cmd_evt(p, total_len, status);
    }
}

void bt_coex_handle_specific_evt(uint8_t *p)
{
    uint16_t subcode;

    LE_STREAM_TO_UINT16(subcode, p);
    if (subcode == HCI_VENDOR_PTA_AUTO_REPORT_EVENT)
    {
        HCI_PRINT_TRACE0("notify wifi driver with autoreport data");
        rtk_notify_info_to_wifi(sizeof(struct rtl_btinfo),
                                (uint8_t *)p);
    }
}

void bt_coex_init(void)
{
	vendor_cmd_init(NULL);
}


