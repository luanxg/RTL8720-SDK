/**
 * Copyright (c) 2017, Realsil Semiconductor Corporation. All rights reserved.
 *
 */

#include <stdio.h>
#include <string.h>
#include "os_sched.h"
#include "os_pool.h"
#include "os_sync.h"
#include "os_mem.h"
#include "trace_app.h"
#include "bt_types.h"

#include "platform_stdlib.h"
#include "strproc.h"
#include "basic_types.h"
#include "diag.h"
#include "section_config.h"
#include "ameba_soc.h"

#include "hci_tp.h"
//====coex====
#include "wifi_conf.h"
#include "trace_app.h"
#include "hci_uart.h"

typedef enum
{
    HCI_RTK_STATE_IDLE,
    HCI_RTK_STATE_READ_LOCAL_VER,
    HCI_RTK_STATE_READ_ROM_VER,
    HCI_RTK_STATE_SET_BAUDRATE,
    HCI_RTK_STATE_PATCH,
    HCI_RTK_STATE_HCI_RESET,
    HCI_RTK_STATE_CHECK_32K,
    HCI_RTK_STATE_WRITE_IQK,
    HCI_RTK_STATE_READY,
    HCI_RTK_STATE_CLOSE,
    HCI_RTK_STATE_IQK,
    HCI_RTK_STATE_DELETE,
} T_HCI_RTK_STATE;

typedef struct {
    u32 IQK_xx;
    u32 IQK_yy;
    u32 LOK_xx;
    u32 LOK_yy;
}BT_Cali_TypeDef;

typedef struct
{
    T_HCI_RTK_STATE     state;
}T_HCI_HARDWARE;


typedef struct
{
    P_HCI_TP_OPEN_CB    open_cb;

    uint8_t             rom_ver;
    uint16_t            lmp_subver;

    uint8_t            *fw_buf;
    uint8_t            *config_buf;
    uint16_t            fw_len;
    uint16_t            config_len;
    uint8_t             patch_frag_cnt;
    uint8_t             patch_frag_idx;
    uint8_t             patch_frag_len;
    uint8_t             patch_frag_tail;
    uint32_t            baudrate;
    uint8_t             hw_flow_cntrl;

    uint8_t            *tx_buf;

    uint32_t            vendor_flow;
    BT_Cali_TypeDef     bt_iqk_data;
    uint8_t             efuseMap[1024];
    uint8_t             phy_efuseMap[16];
} T_HCI_UART;

#define HCI_CMD_PKT             0x01
#define HCI_ACL_PKT             0x02
#define HCI_SCO_PKT             0x03
#define HCI_EVT_PKT             0x04

#define HCI_CMD_HDR_LEN         4   /* packet type (1), command (2), length (1) */

#define HCI_COMMAND_COMPLETE            0x0e
#define HCI_COMMAND_STATUS              0x0f

#define PATCH_FRAGMENT_MAX_SIZE         252
#define BT_CONFIG_SIGNATURE             0x8723ab55
#define BT_CONFIG_HEADER_LEN            6

#define HCI_READ_LOCAL_VERSION_INFO     0x1001
#define HCI_VSC_READ_ROM_VERSION        0xFC6D
#define HCI_VSC_UPDATE_BAUDRATE         0xFC17
#define HCI_VSC_DOWNLOAD_PATCH          0xFC20
#define HCI_VSC_CHECK_32K                     0xFC02
#define HCI_HCI_RESET                             0x0C03

#define HCI_VENDOR_RF_RADIO_REG_WRITE     0xfd4a
#define HCI_RX_TIMEOUT                  100


#define HCI_VSC_VENDOR_IQK        0xFD91

#define HCI_START_IQK  


//#define HCI_UART_OUT
//#define HCI_OLT_TEST

/* FIXME */
#define HCIUART_IRQ_PRIO    10

#ifdef CONFIG_MP_INCLUDED

#endif

typedef struct
{
    uint32_t    bt_baudrate;
    uint32_t    uart_baudrate;
} BAUDRATE_MAP;

const BAUDRATE_MAP baudrates[] =
{
    {0x0000701d, 115200},
    {0x0252C00A, 230400},
    {0x03F75004, 921600},
    {0x05F75004, 921600},
    {0x00005004, 1000000},
    {0x04928002, 1500000},
    {0x00005002, 2000000},
    {0x0000B001, 2500000},
    {0x04928001, 3000000},
    {0x052A6001, 3500000},
    {0x00005001, 4000000},
};

T_HCI_UART *p_hci_rtk;

static T_HCI_HARDWARE hci_hardware;

#define hci_board_debug DBG_8195A

//===========================IQK======================================
void bt_step_clear_iqk(void)
{
    FLASH_EreaseDwordsXIP(SPI_FLASH_BASE+FLASH_BT_PARA_ADDR, 4);
    hci_board_debug("bt_step_clear_iqk:    the flash eraseing \r\n");
    bt_dump_flash();
}
bool bt_iqk_flash_valid()
{
   if((HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR) == 0xFFFFFFFF) && 
	(HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR + 4) == 0xFFFFFFFF) &&
	(HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR + 8) == 0xFFFFFFFF) && 
	(HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR + 12) == 0xFFFFFFFF))
	{
		 hci_board_debug("bt_iqk_flash_valid: no data\r\n");
		 return false;
	}
    else
	{
		 hci_board_debug("bt_iqk_flash_valid: has data\r\n");
         return true;
	}
}

bool bt_iqk_efuse_valid()
{
    if((p_hci_rtk->phy_efuseMap[3] == 0xff) &&
            (p_hci_rtk->phy_efuseMap[4] == 0xff)&&
            (p_hci_rtk->phy_efuseMap[0x05] == 0xff)&&
            (p_hci_rtk->phy_efuseMap[0x06] == 0xff))
    {
        hci_board_debug("bt_iqk_efuse_valid: no data\r\n");
        return false;
    }
    else
    {
        hci_board_debug("bt_iqk_efuse_valid: has data\r\n");
        return true;
    }
}

static uint32_t cal_bit_shift(uint32_t Mask)
{
    uint32_t i;
    for(i=0; i<31;i++)
    {
        if(((Mask>>i) & 0x1)==1)
            break;
    }
    return (i);
}

void set_reg_value(uint32_t reg_address, uint32_t Mask , uint32_t val)
{
    uint32_t shift = 0;
    uint32_t data = 0;
    data = HAL_READ32(reg_address, 0);
    shift = cal_bit_shift(Mask);
    data = ((data & (~Mask)) | (val << shift));
    HAL_WRITE32(reg_address, 0, data);
    data = HAL_READ32(reg_address, 0);
}

void bt_change_gnt_bt_only()
{
    set_reg_value(0x40080764, BIT9 | BIT10,3 );
}

void bt_change_gnt_wifi_only()
{	 
    set_reg_value(0x40080764, BIT9 | BIT10,1);
}

bool bt_check_iqk(void)
{
	u32 ret = _FAIL;
    BT_Cali_TypeDef     bt_iqk_data;
	if(bt_iqk_flash_valid())
	{
		bt_iqk_data.IQK_xx = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR);
		bt_iqk_data.IQK_yy = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+4);
		bt_iqk_data.LOK_xx = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+8);
		bt_iqk_data.LOK_yy = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+12);
        //bt_dump_flash();
		bt_lok_write(bt_iqk_data.LOK_xx, bt_iqk_data.LOK_yy);
		p_hci_rtk->phy_efuseMap[0] = 0;
		p_hci_rtk->phy_efuseMap[1] = 0xfe;
		p_hci_rtk->phy_efuseMap[2] = 0xff;
        memcpy(&p_hci_rtk->bt_iqk_data, &bt_iqk_data, sizeof(BT_Cali_TypeDef));
	}
	else if (bt_iqk_efuse_valid())
	{
		bt_iqk_data.IQK_xx = p_hci_rtk->phy_efuseMap[3] | (p_hci_rtk->phy_efuseMap[4] <<8);
		bt_iqk_data.IQK_yy = p_hci_rtk->phy_efuseMap[5] | (p_hci_rtk->phy_efuseMap[6] <<8);
		bt_iqk_data.LOK_xx = p_hci_rtk->phy_efuseMap[0x0c];
		bt_iqk_data.LOK_yy = p_hci_rtk->phy_efuseMap[0x0d];

		hci_board_debug("bt_lok_write :the IQK_xx  data is 0x%x,\r\n", bt_iqk_data.IQK_xx);
	 	hci_board_debug("bt_lok_write: the IQK_yy data is 0x%x,\r\n", bt_iqk_data.IQK_yy);
		hci_board_debug("bt_lok_write :the LOK_xx  data is 0x%x,\r\n", bt_iqk_data.LOK_xx);
	 	hci_board_debug("bt_lok_write: the LOK_yy data is 0x%x,\r\n", bt_iqk_data.LOK_yy);
		bt_lok_write(bt_iqk_data.LOK_xx, bt_iqk_data.LOK_yy);
        memcpy(&p_hci_rtk->bt_iqk_data, &bt_iqk_data, sizeof(BT_Cali_TypeDef));
        //just debug
		//p_hci_rtk->phy_efuseMap[0] = 0;
		//p_hci_rtk->phy_efuseMap[1] = 0xfe;
		p_hci_rtk->phy_efuseMap[2] = 0xff;
	}
	else
	{
		hci_board_debug("bt_check_iqk:  NO IQK LOK DATA need start LOK,\r\n");
		hci_hardware.state   = HCI_RTK_STATE_IQK;
		p_hci_rtk->vendor_flow = 0;
		hci_tp_rf_radio_ver_flow();
           // hci_tp_rf_radio_ver(0x00, 0x4000);
		return false;
	}
    return true;
}

static void bt_power_on(void)
{
    set_reg_value(0x40000000,BIT0 | BIT1, 3);
}

static void bt_power_off(void)
{
    set_reg_value(0x40000000,BIT0 | BIT1, 0);
}

void bt_reset(void)
{
    uint32_t val;
   // hci_board_debug("BT OPEN LOG...\n");

    //open bt log pa16
    set_reg_value(0x48000440,BIT0 |BIT1 | BIT2 | BIT3 | BIT4, 17);

	#ifdef HCI_UART_OUT
    //
	HAL_WRITE32(0x48000000, 0x5f0, 0x00000202);
    //PA2 TX
	HAL_WRITE32(0x48000000, 0x408, 0x00005C11);
    //pa0 RTS
	HAL_WRITE32(0x48000000, 0x400, 0x00005C11);
    //PA4 RX
	HAL_WRITE32(0x48000000, 0x410, 0x00005C11);
    //PB31  CTS
	HAL_WRITE32(0x48000000, 0x4fc, 0x00005C11);
	HAL_WRITE32(0x48000000, 0x208, 0x1201119A);
	HAL_WRITE32(0x48000000, 0x208, 0x1201119A);
	HAL_WRITE32(0x48000000, 0x344, 0x00000200);
	HAL_WRITE32(0x48000000, 0x280, 0x00000007);
	HAL_WRITE32(0x48000000, 0x344, 0x00000300);
	HAL_WRITE32(0x48000000, 0x344, 0x00000301);
    os_delay(200);
	#endif
	
    hci_board_debug("BT Reset...\n");
    //bt power
    bt_power_on();

    os_delay(5);

    //isolation
    set_reg_value(0x40000000,BIT16, 0);
    os_delay(5);

    //BT function enable
    set_reg_value(0x40000204,BIT24, 0);
    os_delay(5);

    set_reg_value(0x40000204,BIT24, 1);
    os_delay(200);

    //BT clock enable
    set_reg_value(0x40000214, BIT24, 1);
    os_delay(5);


#if 0
#ifdef HCI_MAILBOX_SWITCH
    val = HAL_READ32(0x40080000, 0xA8);
    val = 0x82000000;
    HAL_WRITE32(0x40080000, 0xA8, val);
    os_delay(20);
#endif
#endif


}

/*
void efuse_test_dump16(u8 efuseMap[], u32 index)
{
    hci_board_debug("EFUSE[%02x]: %02x %02x %02x %02x %02x %02x %02x %02x "
              "%02x %02x %02x %02x %02x %02x %02x %02x\n", index,
              efuseMap[index], efuseMap[index + 1], efuseMap[index + 2], efuseMap[index + 3],
              efuseMap[index + 4], efuseMap[index + 5], efuseMap[index + 6], efuseMap[index + 7],
              efuseMap[index + 8], efuseMap[index + 9], efuseMap[index + 10], efuseMap[index + 11],
              efuseMap[index + 12], efuseMap[index + 13], efuseMap[index + 14], efuseMap[index + 15]);
}
*/

void bt_read_efuse(void)
{
    u32 Idx = 0;
    u32 ret = 0;

    ret = EFUSE_LMAP_READ(p_hci_rtk->efuseMap);
    if (ret == _FAIL)
    {
        hci_board_debug("EFUSE_LMAP_READ fail \n");
    }
    /*
	EFUSE_LMAP_WRITE(ADDR,COUNT,DATA);
    //ret = EFUSE_PMAP_WRITE8(0, 0x120,0x11, L25EOUTVOLTAGE);
    //ret = EFUSE_PMAP_WRITE8(0, 0x123,0x12, L25EOUTVOLTAGE);
    if (ret == _FAIL)
    {
        hci_board_debug("EFUSE_PMAP_WRITE fail \n");
    }
    */
    //read phy
    /*
    hci_board_debug("\r\n==WRITE=old phy_efuse:==\r\n ");
    for (Idx = 0; Idx < 16; Idx++)
    {
        hci_board_debug("%x ", phy_efuseMap[Idx]);
    }
    hci_board_debug("\r\n===new phy_efuse:==\r\n ");
    */
    for (Idx = 0; Idx < 16; Idx++)
    {
       EFUSE_PMAP_READ8(0, 0x120 + Idx, p_hci_rtk->phy_efuseMap + Idx, L25EOUTVOLTAGE);
	   //phy_efuseMap[Idx]=  efuseMap[0x120+Idx];
    }
    /*
    for (Idx = 0; Idx < 16; Idx++)
    {
        hci_board_debug("%x ", phy_efuseMap[Idx]);
    }
    */
}



void hci_rtk_convert_buadrate(uint32_t bt_baudrate, uint32_t *uart_baudrate)
{
    uint8_t i;

    *uart_baudrate = 115200;

    for (i = 0; i < sizeof(baudrates) / sizeof(BAUDRATE_MAP); i++)
    {
        if (baudrates[i].bt_baudrate == bt_baudrate)
        {
            *uart_baudrate = baudrates[i].uart_baudrate;
            return;
        }
    }

    HCI_PRINT_TRACE0("hci_rtk_convert_buadrate: use default baudrate[115200]");

    return;
}

bool hci_rtk_parse_config(uint8_t *config_buf, uint16_t config_len)
{
    uint32_t signature;
    uint16_t payload_len;
    uint16_t entry_offset;
    uint16_t entry_len;
    uint8_t *p_entry;
    uint8_t *p;
    uint8_t *p_len;
    uint8_t i;

    //enter the  config_len

    p = config_buf;
    p_len = config_buf + 4;


    LE_STREAM_TO_UINT32(signature, p);
    LE_STREAM_TO_UINT16(payload_len, p);

    if (signature != BT_CONFIG_SIGNATURE)
    {
        HCI_PRINT_ERROR1("hci_rtk_parse_config: invalid signature 0x%08x", signature);

        return false;
    }

    if (payload_len != config_len - BT_CONFIG_HEADER_LEN)
    {
        HCI_PRINT_ERROR2("hci_rtk_parse_config: invalid len, stated %u, calculated %u",
                         payload_len, config_len - BT_CONFIG_HEADER_LEN);
        LE_UINT16_TO_STREAM(p_len, config_len - BT_CONFIG_HEADER_LEN);  //just avoid the length is not coreect
        /* FIX the len */
       // return false;
    }

//    hci_board_debug("\r\nconfig_len = %x\r\n", config_len - BT_CONFIG_HEADER_LEN);

    p_entry = config_buf + BT_CONFIG_HEADER_LEN;

    while (p_entry < config_buf + config_len)
    {
        p = p_entry;
        LE_STREAM_TO_UINT16(entry_offset, p);
        LE_STREAM_TO_UINT8(entry_len, p);
        p_entry = p + entry_len;

        switch (entry_offset)
        {
        case 0x000c:
            LE_STREAM_TO_UINT32(p_hci_rtk->baudrate, p);
            if (entry_len >= 12)
            {
                p_hci_rtk->hw_flow_cntrl |= 0x80;   /* bit7 set hw flow control */
                p += 8;

                if (*p & 0x04)  /* offset 0x18, bit2 */
                {
                    p_hci_rtk->hw_flow_cntrl |= 1;  /* bit0 enable hw flow control */
                }
            }
            HCI_PRINT_TRACE2("hci_rtk_parse_config: baudrate 0x%08x, hw flow control 0x%02x",
                             p_hci_rtk->baudrate,p_hci_rtk->hw_flow_cntrl);
            //hci_board_debug("hci_rtk_parse_config: baudrate 0x%08x\n", p_hci_rtk->baudrate);
            break;
        case 0x0030:

            if (entry_len == 6)
            {
                if ((p_hci_rtk->efuseMap[0x190] != 0xff) && (p_hci_rtk->efuseMap[0x191] != 0xff))
                {
                    memcpy(p,&p_hci_rtk->efuseMap[0x190],6);
                    /*
                    p[0] = efuseMap[0x190];
                    p[1] = efuseMap[0x191];
                    p[2] = efuseMap[0x192];
                    p[3] = efuseMap[0x193];
                    p[4] = efuseMap[0x194];
                    p[5] = efuseMap[0x195];
                    */

                    hci_board_debug("\r\nBT ADDRESS:\r\n");
                    for(int i = 0 ;i <6;i ++)
                    {
                        hci_board_debug("%x:",p_hci_rtk->efuseMap[0x190+i]);
                    }
                    hci_board_debug("\r\n");
/*
                    hci_board_debug("hci_rtk_parse_config: BT ADDRESS use efuse %02x %02x %02x %02x %02x %02x\n",
                              hci_rtk.efuseMap[0x190 + 0], efuseMap[0x190 + 1], efuseMap[0x190 + 2], efuseMap[0x190 + 3],
                              efuseMap[0x190 + 4], efuseMap[0x190 + 5]);
                              */

                }
                else
                {
                    hci_board_debug("hci_rtk_parse_config: BT ADDRESS  %02x %02x %02x %02x %02x %02x, use the defaut config\n",
                              p[0], p[1], p[2], p[3], p[4], p[5]);
                }
            }
            break;
        case 0x0278:
            for(i = 0;i < 6;i ++)
            {
                if(p_hci_rtk->efuseMap[0x196+i] != 0xff)
                {
                    p[i]= p_hci_rtk->efuseMap[0x196+i];
                    hci_board_debug("\r\n p_hci_rtk->efuseMap[%x] = %x\r\n", 0x196+i, p[i]);
                }
            }
            break;
        case 0x0283:
            for(i = 0;i < 4;i ++)
            {
                if(p_hci_rtk->efuseMap[0x19c+i] != 0xff)
                {
                    p[i]= p_hci_rtk->efuseMap[0x19c+i];
                    hci_board_debug("\r\n p_hci_rtk->efuseMap[%x] = %x\r\n", 0x19c+i, p[i]);
                }
            }
            break;
        case 0x0288:
            for(i = 0;i < 4;i ++)
            {
                if(p_hci_rtk->efuseMap[0x1A1+i] != 0xff)
                {
                    p[i]= p_hci_rtk->efuseMap[0x1A1+i];
                    hci_board_debug("\r\n p_hci_rtk->efuseMap[%x] = %x\r\n", 0x1a1+i, p[i]);
                }
            }
            break;
        default:
            HCI_PRINT_TRACE2("hci_rtk_parse_config: entry offset 0x%04x, len 0x%02x",
                             entry_offset, entry_len);
            break;
        }
        
    }

    return true;
}

bool hci_rtk_load_patch(void)
{
    extern unsigned char rtlbt_fw[];
    extern unsigned int  rtlbt_fw_len;
    extern unsigned char rtlbt_config[];
    extern unsigned int  rtlbt_config_len;
    uint32_t svn_version, coex_version, lmp_subversion;;
    uint32_t temp;

#define MERGE_PATCH_ADDRESS_OTA1  0x080F0000
#define MERGE_PATCH_ADDRESS_OTA2  0x081F0000
#define MERGE_PATCH_SWITCH_ADDR   0x08003028
#define MERGE_PATCH_SWITCH_SINGLE 0xAAAAAAAA
    
    uint32_t val = 0;
    uint8_t *p_merge_addr = NULL;
    val = HAL_READ32(MERGE_PATCH_SWITCH_ADDR, 0);
    if(val != MERGE_PATCH_SWITCH_SINGLE)
    {
        p_merge_addr = rtlbt_fw;
    }
    else if (ota_get_cur_index() == OTA_INDEX_1)
    {
        hci_board_debug("\nWe use BT ROM OTA2 PATCH ADDRESS:0x%x\n", MERGE_PATCH_ADDRESS_OTA2);
        p_merge_addr = (uint8_t *)MERGE_PATCH_ADDRESS_OTA2;

        if(!memcmp(p_merge_addr, "Realtech", sizeof("Realtech")-1))
        {
            hci_board_debug("\nROM OTA2 has BT patch: use the FLASH Rom patch\n");
            p_merge_addr = MERGE_PATCH_ADDRESS_OTA2;
        }
        else
        {
            hci_board_debug("\nERROR!!!:OTA2 has no BT patch: please download on 0x%x\n", MERGE_PATCH_ADDRESS_OTA2);
            while(1);
        }
    }
    else
    {
        hci_board_debug("\nWe use BT ROM OTA1 PATCH ADDRESS:0x%x\n", MERGE_PATCH_ADDRESS_OTA1);
        p_merge_addr =(uint8_t *)MERGE_PATCH_ADDRESS_OTA1;

        if(!memcmp(p_merge_addr, "Realtech", sizeof("Realtech")-1))
        {
            hci_board_debug("\nROM has BT patch: use the FLASH Rom patch\n");
            p_merge_addr = MERGE_PATCH_ADDRESS_OTA1;
        }
        else
        {
            hci_board_debug("\nERROR!!!:OTA1 has no BT patch: please download on 0x%x\n", MERGE_PATCH_ADDRESS_OTA1);
            while(1);
        }
    }

    LE_ARRAY_TO_UINT32(lmp_subversion, (p_merge_addr+8));


    p_hci_rtk->fw_len = p_merge_addr[0x10]|(p_merge_addr[0x11]<<8) ;
    p_hci_rtk->fw_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, p_hci_rtk->fw_len);
    
    if(p_hci_rtk->fw_buf == NULL)
    {
        hci_board_debug("\n p_hci_rtk->fw_buf ,malloc fail\n");
        return;
    }
    else
    {
        memcpy(p_hci_rtk->fw_buf,p_merge_addr+0x30, p_hci_rtk->fw_len);
        LE_UINT32_TO_ARRAY(p_hci_rtk->fw_buf+p_hci_rtk->fw_len-4,lmp_subversion);
    }



    p_hci_rtk->config_buf = rtlbt_config;
    p_hci_rtk->config_len = rtlbt_config_len;
    p_hci_rtk->patch_frag_idx = 0;

    LE_ARRAY_TO_UINT32(svn_version, (p_hci_rtk->fw_buf+p_hci_rtk->fw_len-8));
    LE_ARRAY_TO_UINT32(coex_version, (p_hci_rtk->fw_buf+p_hci_rtk->fw_len-12));
    //hci_board_debug("2.BT patch:svn %d coex svn_version: %x LMP VERSION:%x\n",svn_version, coex_version,lmp_subversion);

    //p_hci_rtk->config_len = 0;

    if (p_hci_rtk->config_len != 0)
    {
        if (hci_rtk_parse_config(p_hci_rtk->config_buf, p_hci_rtk->config_len) == false)
        {
            return false;
        }

    }

    p_hci_rtk->patch_frag_cnt  = (p_hci_rtk->fw_len + p_hci_rtk->config_len) / PATCH_FRAGMENT_MAX_SIZE;
    p_hci_rtk->patch_frag_tail = (p_hci_rtk->fw_len + p_hci_rtk->config_len) % PATCH_FRAGMENT_MAX_SIZE;

    if (p_hci_rtk->patch_frag_tail)
    {
        p_hci_rtk->patch_frag_cnt += 1;
    }
    else
    {
        p_hci_rtk->patch_frag_tail = PATCH_FRAGMENT_MAX_SIZE;
    }

    HCI_PRINT_INFO3("hci_rtk_load_patch:svn %d patch frag count %u, tail len %u",svn_version,
                    p_hci_rtk->patch_frag_cnt, p_hci_rtk->patch_frag_tail);
    HCI_PRINT_INFO3("BT patch:svn %d coex svn_version: %x LMP VERSION:%x\n",svn_version, coex_version,lmp_subversion);
    //

    return true;
}

bool hci_rtk_tx_cb(void)
{
    if (p_hci_rtk->tx_buf != NULL)
    {
        os_mem_free(p_hci_rtk->tx_buf);
        p_hci_rtk->tx_buf = NULL;
    }

    return true;
}

void hci_tp_download_patch(void)
{
    uint8_t  *p_cmd;
    uint8_t  *p;
    uint8_t  *p_frag;
    uint16_t  sent_len;

    if (p_hci_rtk->patch_frag_idx < p_hci_rtk->patch_frag_cnt)
    {
        if (p_hci_rtk->patch_frag_idx == p_hci_rtk->patch_frag_cnt - 1)
        {
            HCI_PRINT_TRACE0("hci_tp_download_patch: send last frag");
            //hci_board_debug("hci_tp_download_patch: send last frag\n");

            p_hci_rtk->patch_frag_idx |= 0x80;
            p_hci_rtk->patch_frag_len  = p_hci_rtk->patch_frag_tail;
        }
        else
        {
            p_hci_rtk->patch_frag_len = PATCH_FRAGMENT_MAX_SIZE;
        }
    }

//    hci_board_debug("hci_tp_download_patch: frag index %d, len %d\n",
 //             p_hci_rtk->patch_frag_idx, p_hci_rtk->patch_frag_len);
    HCI_PRINT_TRACE2("hci_tp_download_patch: frag index %d, len %d",
                     p_hci_rtk->patch_frag_idx, p_hci_rtk->patch_frag_len);

    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, HCI_CMD_HDR_LEN + 1 + p_hci_rtk->patch_frag_len);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_VSC_DOWNLOAD_PATCH);
        LE_UINT8_TO_STREAM(p, 1 + p_hci_rtk->patch_frag_len);  /* length */
        LE_UINT8_TO_STREAM(p, p_hci_rtk->patch_frag_idx);  /* frag index */

        sent_len = (p_hci_rtk->patch_frag_idx & 0x7F) * PATCH_FRAGMENT_MAX_SIZE;

        if (sent_len >= p_hci_rtk->fw_len)    /* config patch domain */
        {
            p_frag = p_hci_rtk->config_buf + sent_len -p_hci_rtk->fw_len;
            memcpy(p, p_frag, p_hci_rtk->patch_frag_len);
        }
        else if (sent_len + p_hci_rtk->patch_frag_len <= p_hci_rtk->fw_len) /* fw patch domain */
        {
            p_frag = p_hci_rtk->fw_buf + sent_len;
            memcpy(p, p_frag, p_hci_rtk->patch_frag_len);
        }
        else    /* accross fw and config patch domains */
        {
            p_frag = p_hci_rtk->fw_buf + sent_len;
            memcpy(p, p_frag, p_hci_rtk->fw_len - sent_len);
            p += p_hci_rtk->fw_len - sent_len;
            memcpy(p, p_hci_rtk->config_buf, p_hci_rtk->patch_frag_len + sent_len - p_hci_rtk->fw_len);
        }

        p_hci_rtk->patch_frag_idx++;
        hci_tp_send(p_cmd, HCI_CMD_HDR_LEN + 1 + p_hci_rtk->patch_frag_len, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}

void hci_tp_set_controller_baudrate(void)
{
    uint8_t *p_cmd;
    uint8_t *p;

    if (p_hci_rtk->baudrate == 0)
    {
        p_hci_rtk->baudrate = 0x0000701d;
    }

    HCI_PRINT_TRACE1("hci_tp_set_controller_baudrate: baudrate 0x%08x", p_hci_rtk->baudrate);
  //  hci_board_debug("hci_tp_set_controller_baudrate: baudrate 0x%08x\n", p_hci_rtk->baudrate);

    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, HCI_CMD_HDR_LEN + 4);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_VSC_UPDATE_BAUDRATE);
        LE_UINT8_TO_STREAM(p, 4);   /* length */
        LE_UINT32_TO_STREAM(p, p_hci_rtk->baudrate);

        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}

void hci_tp_read_local_ver(void)
{
    uint8_t *p_cmd;
    uint8_t *p;

    HCI_PRINT_TRACE0("hci_tp_read_local_ver");
    //hci_board_debug("hci_tp_read_local_ver\n");

    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_READ_LOCAL_VERSION_INFO);
        LE_UINT8_TO_STREAM(p, 0); /* length */

        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}

#ifdef HCI_OLT_TEST
void hci_tp_tx_test_cmd(void)
{
    uint8_t *p_cmd;
    uint8_t *p;

    //HCI_PRINT_TRACE0("hci_tp_read_local_ver");
    //
#define HCI_LE_TRANSMIT_TEST 0x201e
    hci_board_debug("hci_tp_tx_test_cmd 2402, 0xFF, 0x00\n");

    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, 7);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_LE_TRANSMIT_TEST);
        LE_UINT8_TO_STREAM(p, 3);
        LE_UINT8_TO_STREAM(p, 0);//TX_CHANNEL
        LE_UINT8_TO_STREAM(p, 0xff);//length_of_test_data
        LE_UINT8_TO_STREAM(p, 0);//packet_payload

        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}

void hci_tp_rx_test_cmd(void)
{
    uint8_t *p_cmd;
    uint8_t *p;

    //HCI_PRINT_TRACE0("hci_tp_read_local_ver");
    //
#define HCI_LE_RECEIVE_TEST 0x201d
    hci_board_debug("hci_tp_rx_test_cmd 2402\n");
    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, 6);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_LE_RECEIVE_TEST);
        LE_UINT8_TO_STREAM(p, 1);
        LE_UINT8_TO_STREAM(p, 0);//TX_CHANNEL
        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}

void hci_tp_test_end_cmd(void)
{
    uint8_t *p_cmd;
    uint8_t *p;

    //HCI_PRINT_TRACE0("hci_tp_read_local_ver");
    //
#define HCI_LE_TEST_END 0x201f
    hci_board_debug("hci_tp_rx_test_cmd 2402\n");
    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, 6);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_LE_TEST_END);
        LE_UINT8_TO_STREAM(p, 0);
        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}
#endif
void hci_tp_rf_radio_ver(u8 offset, u16 value)
{
    uint8_t *p_cmd;
    uint8_t *p;

    HCI_PRINT_TRACE0("hci_tp_rf_radio_ver");
  //  hci_board_debug("hci_tp_rf_radio_ver\n");

    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, 8);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_VENDOR_RF_RADIO_REG_WRITE);
        LE_UINT8_TO_STREAM(p, 4); /* length */
	    LE_UINT8_TO_STREAM(p, offset); /* offset		*/
	    LE_UINT16_TO_STREAM(p, value); /* value */
	    LE_UINT8_TO_STREAM(p, 0); /*  */

        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}

void bt_dump_flash()
{
        uint32_t val = 0;
	 	hci_board_debug("bt_dump_flash:    DUMP,\r\n");
		val = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR); 
	 	hci_board_debug("the IQK_xx  data is 0x%x,\r\n", val);
		val = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR + 4); 
	 	hci_board_debug("the IQK_yy data is 0x%x,\r\n", val);
		val = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR + 8); 
	 	hci_board_debug("the LOK_xx  data is 0x%x,\r\n", val);
		val = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR + 12); 
	 	hci_board_debug("the LOK_yy data is 0x%x,\r\n", val);
}

void bt_dump_iqk(BT_Cali_TypeDef *iqk_data)
{
	 	hci_board_debug("bt_dump_flash:    DUMP,\r\n");
	 	hci_board_debug("the IQK_xx  data is 0x%x,\r\n", iqk_data->IQK_xx);
	 	hci_board_debug("the IQK_yy  data is 0x%x,\r\n", iqk_data->IQK_yy);
	 	hci_board_debug("the LOK_xx  data is 0x%x,\r\n", iqk_data->LOK_xx);
	 	hci_board_debug("the LOK_yy  data is 0x%x,\r\n", iqk_data->LOK_yy);
}

bool bt_simularity_compare(BT_Cali_TypeDef *a,BT_Cali_TypeDef *b)
{
    bool is_simular;
    int32_t diff, tmp1, tmp2;
    
    uint8_t i;
    uint32_t *tmp_A,*tmp_B;
    tmp_A = (uint32_t *)a;
    tmp_B = (uint32_t *)b;

#define MAX_IQKRANCE  5

    for(i = 0 ;i <4; i++)
    {
        tmp1 = tmp_A[i];
        tmp2 = tmp_B[i];
        diff = (tmp1 > tmp2)?(tmp1-tmp2):(tmp2-tmp1);

       // hci_board_debug("bt_simularity_compare:    %x,\r\n", diff);
        if(diff > MAX_IQKRANCE)
        {
            //hci_board_debug("bt_simularity_compare:   tmp1= %x, tmp2= %x\r\n", tmp1, tmp2);
            return false;
        }
    }
    //hci_board_debug("bt_simularity_compare:   return true\r\n");
    return true;
}
bool pre_start_IQK()
{
    BT_Cali_TypeDef     tmp_bt_iqk_data[3];
	uint32_t ret = 0;
    uint8_t i = 0;
    for(i = 0;i < 3; i++)
    {
        ret = bt_iqk_8721d(&tmp_bt_iqk_data[i], 0);
        if(_FAIL == ret)
        {
            hci_board_debug("bt_check_iqk:Warning: IQK Fail, please connect driver !!!!!!!!!\r\n");
            return false;
        }
       bt_dump_iqk(&tmp_bt_iqk_data[i]);
        os_delay(20);
    }

    if(     bt_simularity_compare(&tmp_bt_iqk_data[0],&tmp_bt_iqk_data[1]) ||
            bt_simularity_compare(&tmp_bt_iqk_data[1],&tmp_bt_iqk_data[2]) ||
            bt_simularity_compare(&tmp_bt_iqk_data[0],&tmp_bt_iqk_data[2]) )
    {
        //store the result
        return true;
    }
    else
    {
        hci_board_debug("the IQK result is not right:ERROR: IQK Fail, please connect driver !!!!!!!!!\r\n"); 
        return false;
    }

}

void hci_tp_rf_radio_ver_flow()
{
	u32 ret = 0;
    uint32_t vendor_flow = p_hci_rtk->vendor_flow;


    if(vendor_flow == 0)
	{
		hci_tp_rf_radio_ver(0x00, 0x4000);
	}
	else if(vendor_flow == 1)
    {
        hci_tp_rf_radio_ver(0x01, 0x0180);
    }
	else if(vendor_flow == 2)
    {
		hci_tp_rf_radio_ver(0x02, 0x3800);
	}
	else if(vendor_flow == 3)
	{
		hci_tp_rf_radio_ver(0x3f, 0x0400);
	}
	else if (vendor_flow == 4)
	{
            ret = bt_iqk_8721d(&p_hci_rtk->bt_iqk_data, 1);
            if(_FAIL == ret)
            {
                hci_board_debug("bt_check_iqk:Warning: IQK Fail, please connect driver !!!!!!!!!\r\n");
                return false;
            }
            bt_dump_iqk(&p_hci_rtk->bt_iqk_data);
            bt_lok_write(p_hci_rtk->bt_iqk_data.LOK_xx, p_hci_rtk->bt_iqk_data.LOK_yy);
        hci_board_debug("IQK OK , please reset\n");
	}
	else
		{
		  hci_board_debug("IQK error\n");
		}
    p_hci_rtk->vendor_flow++;
}



void hci_tp_read_rom_ver(void)
{
    uint8_t *p_cmd;
    uint8_t *p;

    HCI_PRINT_TRACE0("hci_tp_read_rom_ver");

    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_VSC_READ_ROM_VERSION);
        LE_UINT8_TO_STREAM(p, 0); /* length */

        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}

void hci_tp_write_efuse_iqk(void)
{
    uint8_t *p_cmd;
    uint8_t *p;

    HCI_PRINT_TRACE0("hci_tp_write_efuse_iqk");
   // hci_board_debug("hci_tp_write_efuse_iqk\n");

    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, 15);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_VSC_VENDOR_IQK);
        LE_UINT8_TO_STREAM(p, 0xc); /* length */
        LE_UINT8_TO_STREAM(p, p_hci_rtk->phy_efuseMap[0]); /* phy_version 0x120 */
        LE_UINT8_TO_STREAM(p, p_hci_rtk->phy_efuseMap[1]); /* phy_invalid 0x121 */
        LE_UINT8_TO_STREAM(p, p_hci_rtk->phy_efuseMap[2]); /* phy_invalid 0x122 */
        LE_UINT16_TO_STREAM(p, p_hci_rtk->bt_iqk_data.IQK_xx); /* phy_xx 0x123 */
        LE_UINT16_TO_STREAM(p, p_hci_rtk->bt_iqk_data.IQK_yy); /* phy_yy 0x125 */
        LE_UINT8_TO_STREAM(p, p_hci_rtk->phy_efuseMap[7]); /* tmeter_val 0x127 */
        LE_UINT8_TO_STREAM(p, p_hci_rtk->phy_efuseMap[8]); /* tmeter_ft_degree 0x128 */
        LE_UINT8_TO_STREAM(p, p_hci_rtk->phy_efuseMap[9]); /* iqm_txgaink 0x129 */
        LE_UINT8_TO_STREAM(p, p_hci_rtk->phy_efuseMap[10]); /* iqm_rxgaink 0x12a */
        LE_UINT8_TO_STREAM(p, p_hci_rtk->phy_efuseMap[11]); /* iqm_rxgaink 0x12b */
        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}

void hci_tp_check_32k()
{
    uint8_t clk_rdy = 0;
    uint32_t count;
    uint8_t *p_cmd;
    uint8_t *p;

    HCI_PRINT_TRACE0("hci_tp_check_32k");
    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, 5);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_VSC_CHECK_32K);
        LE_UINT8_TO_STREAM(p, 1);
        count = RTIM_GetCount(TIMM00);
        os_delay(1);
        if (RTIM_GetCount(TIMM00) - count != 0)
        {
            clk_rdy = 1;
        }
        LE_UINT8_TO_STREAM(p, clk_rdy);
        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}

void hci_tp_hci_reset(void)
{
    uint8_t *p_cmd;
    uint8_t *p;
    HCI_PRINT_TRACE0("hci_reset");
    //hci_board_debug("hci_reset\n");
    p_cmd = os_mem_zalloc(RAM_TYPE_DATA_ON, 4);
    if (p_cmd != NULL)
    {
        p = p_cmd;
        LE_UINT8_TO_STREAM(p, HCI_CMD_PKT);
        LE_UINT16_TO_STREAM(p, HCI_HCI_RESET);
        LE_UINT8_TO_STREAM(p, 0); /* length */
        hci_tp_send(p_cmd, p - p_cmd, hci_rtk_tx_cb);
    }
    else
    {
        p_hci_rtk->open_cb(false);
    }
}




void hci_tp_config(uint8_t *p_buf, uint16_t len)
{
    uint8_t  pkt_type;
    uint8_t  evt_code;
    uint16_t opcode;
    uint8_t  status;

    //hci_board_debug("hci tp config: state %08x\n", hci_hardware.state);
    //HCI_PRINT_INFO2("hci_tp_config: state %u, %b", hci_rtk.state, TRACE_BINARY(len, p_buf));

    LE_STREAM_TO_UINT8(pkt_type, p_buf);

    if (pkt_type != HCI_EVT_PKT)
    {
        /* Skip non-hci event pkt. */
        return;
    }

    LE_STREAM_TO_UINT8(evt_code, p_buf);
    STREAM_SKIP_LEN(p_buf, 1);  /* Skip event len */

    switch (hci_hardware.state)
    {
    case HCI_RTK_STATE_IQK:
       if (evt_code == HCI_COMMAND_COMPLETE)
        {
            STREAM_SKIP_LEN(p_buf, 1);  /* Skip num of hci cmd pkts */
            LE_STREAM_TO_UINT16(opcode, p_buf);

            if (opcode == HCI_VENDOR_RF_RADIO_REG_WRITE)
            {
                /* Skip status, hci version, hci revision, lmp version & manufacture name */
                LE_STREAM_TO_UINT8(status, p_buf);
                if (status == 0)
                {
                   // hci_rtk.state = HCI_RTK_STATE_READY;
                    //hci_board_debug("Hci init IQK %d\n", p_hci_rtk->vendor_flow);
					if(p_hci_rtk->vendor_flow == 5)
					{
						p_hci_rtk->vendor_flow = 0;

					}
					else
					{
						hci_tp_rf_radio_ver_flow();
                        if(p_hci_rtk->vendor_flow == 5)
                        {
                            p_hci_rtk->vendor_flow = 0;
                            os_delay(1000);
                            hci_board_debug("Hci init IQK ok\n");
                            //NORMAL START 
                            bt_reset();
                            bt_coex_pta_only();
                            hci_hardware.state   = HCI_RTK_STATE_READ_LOCAL_VER;
                            hci_tp_read_local_ver();
                        }
					}
                    
                }
                else
                {
                    hci_board_debug("Hci init fail\n");
                }

            }
        }
		break;
		
    case HCI_RTK_STATE_READ_LOCAL_VER:
        if (evt_code == HCI_COMMAND_COMPLETE)
        {
            STREAM_SKIP_LEN(p_buf, 1);  /* Skip num of hci cmd pkts */
            LE_STREAM_TO_UINT16(opcode, p_buf);

            if (opcode == HCI_READ_LOCAL_VERSION_INFO)
            {
                /* Skip status, hci version, hci revision, lmp version & manufacture name */
                STREAM_SKIP_LEN(p_buf, 7);
                LE_STREAM_TO_UINT16(p_hci_rtk->lmp_subver, p_buf);
                HCI_PRINT_INFO1("hci_tp_config: lmp_subver 0x%04x", p_hci_rtk->lmp_subver);
                //hci_board_debug("hci_tp_config: lmp_subver 0x%04x\n", p_hci_rtk->lmp_subver);

                if (p_hci_rtk->lmp_subver != 0x8721)
                {
                    HCI_PRINT_ERROR0("hci_tp_config: Patch already exists");
                    hci_board_debug("hci_tp_config: Patch already exists\n");
                    /* FIXME */
                    /* hci_hardware.state   = HCI_RTK_STATE_IDLE;
                     * p_hci_rtk->open_cb(false);
                     */
                    hci_hardware.state = HCI_RTK_STATE_READY;
                    p_hci_rtk->open_cb(true);

                    break;
                }

                hci_hardware.state = HCI_RTK_STATE_READ_ROM_VER;
                hci_tp_read_rom_ver();
            }
        }
        break;

    case HCI_RTK_STATE_READ_ROM_VER:
        if (evt_code == HCI_COMMAND_COMPLETE)
        {
            STREAM_SKIP_LEN(p_buf, 1);  /* Skip num of hci cmd pkts */
            LE_STREAM_TO_UINT16(opcode, p_buf);

            if (opcode == HCI_VSC_READ_ROM_VERSION)
            {
                LE_STREAM_TO_UINT8(status, p_buf);

                if (status == 0)
                {
                    LE_STREAM_TO_UINT8(p_hci_rtk->rom_ver, p_buf);

                    /* Load fw & config patches first */
                    if (hci_rtk_load_patch() == true)
                    {
                        hci_hardware.state = HCI_RTK_STATE_SET_BAUDRATE;
                        hci_tp_set_controller_baudrate();
                    }
                    else
                    {
                        p_hci_rtk->open_cb(false);
                    }
                    //hci_hardware.state = HCI_RTK_STATE_READY;
                    //p_hci_rtk->open_cb(true);

                }
                else
                {
                    p_hci_rtk->open_cb(false);
                }
            }
        }
        break;

    case HCI_RTK_STATE_SET_BAUDRATE:
        if (evt_code == HCI_COMMAND_COMPLETE)
        {
            STREAM_SKIP_LEN(p_buf, 1);  /* Skip num of hci cmd pkts */
            LE_STREAM_TO_UINT16(opcode, p_buf);

            if (opcode == HCI_VSC_UPDATE_BAUDRATE)
            {
                uint32_t uart_baudrate;

                hci_rtk_convert_buadrate(p_hci_rtk->baudrate, &uart_baudrate);
                hci_uart_set_baudrate(uart_baudrate);
                os_delay(50);

                hci_hardware.state = HCI_RTK_STATE_PATCH;

                HCI_PRINT_INFO0("Start download patch/config");
                //hci_board_debug("Start download patch/config\n");
                /* Kickoff patch downloading */
                hci_tp_download_patch();
            }
        }
        break;

    case HCI_RTK_STATE_PATCH:
        if (evt_code == HCI_COMMAND_COMPLETE)
        {
            STREAM_SKIP_LEN(p_buf, 1);  /* Skip num of hci cmd pkts */
            LE_STREAM_TO_UINT16(opcode, p_buf);

            if (opcode == HCI_VSC_DOWNLOAD_PATCH)
            {
                uint8_t index;

                LE_STREAM_TO_UINT8(status, p_buf);
                LE_STREAM_TO_UINT8(index, p_buf);

                if (status == 0)
                {
                    if (index & 0x80)   /* receive the last fragment completed evt */
                    {
                        /* hci_hardware.state = HCI_RTK_STATE_CHECK_32K;
                         * hci_tp_check_32k();
                         */
                        hci_board_debug("Download patch completely\n");
                        os_mem_free(p_hci_rtk->fw_buf);
						#ifdef HCI_START_IQK
						hci_hardware.state = HCI_RTK_STATE_WRITE_IQK;
						hci_tp_write_efuse_iqk();
						#else
                        hci_hardware.state = HCI_RTK_STATE_HCI_RESET;
                        hci_tp_hci_reset();
						#endif
                        //hci_hardware.state = HCI_RTK_STATE_READY;
                        //   p_hci_rtk->open_cb(true);
                    }
                    else
                    {
                        hci_tp_download_patch();
                    }
                }
                else
                {
                    p_hci_rtk->open_cb(false);
                }
            }
        }
        break;
    case HCI_RTK_STATE_WRITE_IQK:
        if (evt_code == HCI_COMMAND_COMPLETE)
        {
            STREAM_SKIP_LEN(p_buf, 1);  /* Skip num of hci cmd pkts */
            LE_STREAM_TO_UINT16(opcode, p_buf);

            if (opcode == HCI_VSC_VENDOR_IQK)
            {
                LE_STREAM_TO_UINT8(status, p_buf);

                if (status == 0)
                {
                  
                        /* hci_hardware.state = HCI_RTK_STATE_CHECK_32K;
                         * hci_tp_check_32k();
                         */
                        hci_board_debug("Write IQK successfully\n");
				
                        hci_hardware.state = HCI_RTK_STATE_HCI_RESET;
                        hci_tp_hci_reset();
                }
                else
                {
                    p_hci_rtk->open_cb(false);
                }
            }
        }	
		break;
    case HCI_RTK_STATE_HCI_RESET:
        if (evt_code == HCI_COMMAND_COMPLETE)
        {
            STREAM_SKIP_LEN(p_buf, 1);  /* Skip num of hci cmd pkts */
            LE_STREAM_TO_UINT16(opcode, p_buf);
            if (opcode == HCI_HCI_RESET)
            {
                LE_STREAM_TO_UINT8(status, p_buf);
                if (status == 0)
                {
                    hci_hardware.state = HCI_RTK_STATE_READY;
                    hci_board_debug("BT init complete \n");
#ifdef HCI_OLT_TEST
                    hci_board_debug("start olt test\n");
                    p_hci_rtk->open_cb(false);

#else
                    p_hci_rtk->open_cb(true);
#endif
                }
                else
                {
                    hci_board_debug("Hci init fail\n");
                }
            }
        }
        break;
    case HCI_RTK_STATE_CHECK_32K:
        if (evt_code == HCI_COMMAND_COMPLETE)
        {
            STREAM_SKIP_LEN(p_buf, 1);  /* Skip num of hci cmd pkts */
            LE_STREAM_TO_UINT16(opcode, p_buf);

            if (opcode == HCI_VSC_CHECK_32K)
            {
                hci_hardware.state = HCI_RTK_STATE_READY;
                p_hci_rtk->open_cb(true);
            }
        }
        break;

    case HCI_RTK_STATE_READY:
    default:
        break;
    }
}

//malloc
void hci_rtk_initing_fail(void)
{
    os_mem_free(p_hci_rtk);
    p_hci_rtk->open_cb(false);
}
//no malloc
void hci_rtk_init_fail(void)
{
    p_hci_rtk->open_cb(false);
}

bool hci_rtk_malloc(void)
{
    if(p_hci_rtk == NULL)
    {
        p_hci_rtk = os_mem_zalloc(RAM_TYPE_DATA_ON, sizeof(T_HCI_UART)); //reopen not need init uart

        if(!p_hci_rtk)
        {
            hci_board_debug("hci_tp_open:need %d, left %d\n", sizeof(T_HCI_UART),os_mem_peek(RAM_TYPE_DATA_ON));
            HCI_PRINT_INFO0("hci_tp_reopen");
            hci_rtk_init_fail();
            return false;
        }
        else
        {
           // hci_board_debug("hci_tp_reopen_ok:%x\n",p_hci_rtk);
        }
    }
    else
    {
        hci_board_debug("hci_tp_open twice:rx_buffer not free\n");
        HCI_PRINT_INFO0("hci_tp_reopen");
        hci_rtk_init_fail();
        return false;
    }


    return true;
}


bool hci_rtk_free(void)
{
    if(p_hci_rtk == NULL)
    {
        hci_board_debug("hci_tp_del: p_hci_rtk = NULL, no need free\r\n");
        return true;
    }
    else
    {
        os_mem_free(p_hci_rtk);
        p_hci_rtk = NULL;
    }
}

void hci_tp_open(P_HCI_TP_OPEN_CB open_cb, P_HCI_TP_RX_IND rx_ind)
{
    HCI_PRINT_INFO1("hci_tp_open, this cut is AmebaD %X CUT",SYSCFG_CUTVersion()?0x0B:0x0A);
    //hci_board_debug("\r\n hci_tp_open, this cut is AmebaD %x CUT\r\n",SYSCFG_CUTVersion()?0x0B:0x0A);

    //bt_step_clear_iqk();
    bt_reset();

    hci_rtk_malloc();
    if(!hci_uart_init(rx_ind))
    {
        hci_board_debug("%s:uart_init fail\n", __FUNCTION__);
    }

    hci_hardware.state   = HCI_RTK_STATE_IDLE;
    p_hci_rtk->open_cb = open_cb;

    bt_read_efuse();

#ifdef HCI_START_IQK
    if(bt_check_iqk()== true)
    {
#endif
        bt_coex_pta_only();
        hci_hardware.state   = HCI_RTK_STATE_READ_LOCAL_VER;
        hci_tp_read_local_ver();
#ifdef HCI_START_IQK
    }
#endif

}

void hci_tp_close(void)
{
    HCI_PRINT_INFO0("hci_tp_close");
    bt_power_off();
    hci_rtk_free();
    hci_uart_deinit();
    return;
}

void hci_tp_del(void)
{
    HCI_PRINT_INFO0("hci_tp_del");
    bt_power_off();
    hci_rtk_free();
    hci_uart_deinit();
    return;
}

bool hci_tp_send(uint8_t *p_buf, uint16_t len, P_HCI_TP_TX_CB tx_cb)
{
    p_hci_rtk->tx_buf  = p_buf;
    hci_uart_tx(p_buf, len, tx_cb);
}

uint16_t hci_tp_recv(uint8_t *p_buf, uint16_t size)
{
    return hci_uart_recv(p_buf, size);
}




#define FLATCALIBRATION
#ifdef FLATCALIBRATION
#define RF(x)   (0x40083800+4*x)
void hci_flatk(void)
{
    uint8_t txgain_flak_use;
    uint16_t txgain_flatk = p_hci_rtk->efuseMap[0x198] | p_hci_rtk->efuseMap[0x199]<<8;
    bool txgaink_flag ;
    uint8_t temp = 0;

    txgaink_flag = ((txgain_flatk>>0&0x0f) <= 2) &&
        ((txgain_flatk>>4&0x0f) <= 2) &&
        ((txgain_flatk>>8&0x0f) <= 2) &&
        ((txgain_flatk>>12&0x0f) <= 2);
    //hci_board_debug("\r\n ====debug2=====\r\n   txgain_flatk = %x \r\n", txgain_flatk);

     set_reg_value(RF(0xed), BIT6, 1);
     for(int i = 0; i<4;i++)
     {
         set_reg_value(RF(0x30), BIT12|BIT13, i);
         txgain_flak_use = (txgain_flatk>>(i*4)) & 0x0f ;
         if(txgaink_flag)
         {
             txgain_flak_use +=2;
         }
         if(txgain_flak_use == 0x07)
         {
             txgain_flak_use =0x06;
         }

         temp =(txgain_flak_use>>1 + txgain_flak_use&0x01)& 0x07;

         if(temp = 0x07)
         {
             set_reg_value(RF(0x30), BIT2|BIT1|BIT0, 0x05);
         }
         else if(temp == 0x05)
         {
             set_reg_value(RF(0x30), BIT2|BIT1|BIT0, 0x07);
         }
         else if(temp == 0x04)
         {
             set_reg_value(RF(0x30), BIT2|BIT1|BIT0, 0x07);
         }
         else
         {
             set_reg_value(RF(0x30), BIT2|BIT1|BIT0, temp);
         }
     }

     set_reg_value(RF(0xed), BIT6, 0);
}
#endif
