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

#include "hci_tp.h"
#include "hci_uart.h"
#include "bt_types.h"
#include "trace_app.h"

#include "bt_board.h"
#include "hci_board.h"
#include "hci_process.h"


#define hci_board_32reg_set(addr, val) HAL_WRITE32(addr, 0, val)
#define hci_board_32reg_read(addr) HAL_READ32(addr, 0)

#define BT_EFUSE_TABLE_LEN             0x20


#define BT_CONFIG_SIGNATURE             0x8723ab55
#define BT_CONFIG_HEADER_LEN            6
//#define FIX_OLD_SDK

typedef struct {
    u32 IQK_xx;
    u32 IQK_yy;
    u32 QDAC;
    u32 IDAC;
}BT_Cali_TypeDef;
BT_Cali_TypeDef g_iqk_data;
static uint8_t vendor_flow;

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
unsigned int baudrates_length = sizeof(baudrates) / sizeof(BAUDRATE_MAP);

static uint32_t hci_tp_baudrate;
uint8_t  hci_tp_lgc_efuse[BT_EFUSE_TABLE_LEN];
uint8_t  hci_tp_phy_efuse[16];



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

static void set_reg_value(uint32_t reg_address, uint32_t Mask , uint32_t val)
{
    uint32_t shift = 0;
    uint32_t data = 0;
    data = hci_board_32reg_read(reg_address);
    shift = cal_bit_shift(Mask);
    data = ((data & (~Mask)) | (val << shift));
    hci_board_32reg_set(reg_address, data);
    data = hci_board_32reg_read(reg_address);
}
#ifdef FIX_OLD_SDK
uint8_t rltk_wlan_is_mp()
{
    return 1;
}
static uint8_t *rltk_bt_get_patch_code()
{
    extern unsigned char rtlbt_mp_fw[];
    return rtlbt_mp_fw;
}
#endif

bool hci_rtk_parse_config(uint8_t *config_buf, uint16_t config_len, uint8_t *efuse_buf)
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
#define BT_EFUSE_BLOCK1_OFFSET   0x06
#define BT_EFUSE_BLOCK2_OFFSET   0x0c
#define BT_EFUSE_BLOCK3_OFFSET   0x12

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
            if (rltk_wlan_is_mp()) 
            {
                //default use the 115200
                memcpy(p,&(baudrates[0].bt_baudrate),4);
            }
            LE_STREAM_TO_UINT32(hci_tp_baudrate, p);

#if 0
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
#endif
            break;

        case 0x0030:
            if (entry_len == 6)
            {
                if ((efuse_buf[0] != 0xff) && (efuse_buf[1] != 0xff))
                {
                    memcpy(p,&efuse_buf[0],6);
                    
                    hci_board_debug("\r\nBT ADDRESS:\r\n");
                    for(int i = 0 ;i <6;i ++)
                    {
                        hci_board_debug("%x:",efuse_buf[i]);
                    }
                    hci_board_debug("\r\n");
                }
                else
                {
                    hci_board_debug("hci_rtk_parse_config: BT ADDRESS  %02x %02x %02x %02x %02x %02x, use the defaut config\n",
                              p[0], p[1], p[2], p[3], p[4], p[5]);
                }
            }
            break;
        case 0x194:
            for(i = 0;i < entry_len;i ++)
            {
                if(efuse_buf[LEFUSE(0x196+i)] != 0xff)
                {
                    p[i]= efuse_buf[LEFUSE(0x196+i)];
                    hci_board_debug("\r\n logic efuseMap[%x] = %x\r\n",0x196+i, p[i]);
                }
            }
            break;

        case 0x19f:
            for(i = 0;i <entry_len;i ++)
            {
                if(efuse_buf[LEFUSE(0x19c+i)] != 0xff)
                {
                    p[i]= efuse_buf[LEFUSE(0x19c+i)];
                    hci_board_debug("\r\n logic efuseMap[%x] = %x\r\n",0x19c+i, p[i]);
                }
            }
            break;
        case 0x1A4:
            for(i = 0;i < entry_len;i ++)
            {
                if(efuse_buf[LEFUSE(0x1A2+i)] != 0xff)
                {
                    p[i]= efuse_buf[LEFUSE(0x1A2+i)];
                    hci_board_debug("\r\n logic efuseMap[%x] = %x\r\n",0x1A2+i, p[i]);
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


bool hci_rtk_find_patch(void)
{
    extern unsigned char rtlbt_fw[];
    extern unsigned int  rtlbt_fw_len;
    extern unsigned char rtlbt_config[];
    extern unsigned int  rtlbt_config_len;
    const uint8_t single_patch_sing[4]= {0xFD, 0x63, 0x05, 0x62};


    uint8_t            *fw_buf;
    uint8_t            *config_buf;
    uint16_t            fw_len;
    uint32_t            fw_offset;
    uint16_t            config_len;
    uint32_t            lmp_subversion;;
    uint16_t            mp_num_of_patch=0;
    uint16_t            fw_chip_id = 0;
    uint8_t             i;

    uint32_t val = 0;
    uint8_t *p_merge_addr = NULL;
    val = hci_board_32reg_read(MERGE_PATCH_SWITCH_ADDR);


    if(val != MERGE_PATCH_SWITCH_SINGLE)
    {
        p_merge_addr = (uint8_t *)rltk_bt_get_patch_code();
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

//if single patch or merged patch
    if(!memcmp(p_merge_addr, "Realtech", sizeof("Realtech")-1))
    {
        LE_ARRAY_TO_UINT32(lmp_subversion, (p_merge_addr+8));
        LE_ARRAY_TO_UINT16(mp_num_of_patch, p_merge_addr+0x0c);

        if(mp_num_of_patch == 1)
        {
            LE_ARRAY_TO_UINT16(fw_len, p_merge_addr+0x0e +2*mp_num_of_patch );
            LE_ARRAY_TO_UINT32(fw_offset, p_merge_addr+0x0e +4*mp_num_of_patch);
            if (rltk_wlan_is_mp())
            {
                hci_board_debug("\n fw_chip_id patch =%x,mp_num_of_patch=%x \n", fw_chip_id,mp_num_of_patch);
                hci_board_debug("\n  lmp_subversion=%x , fw_len =%x, fw_offset = %x \n", lmp_subversion, fw_len, fw_offset);
            }
        }
        else
        {
            for(i = 0 ;i<mp_num_of_patch; i++)
            {
                LE_ARRAY_TO_UINT16(fw_chip_id, p_merge_addr+0x0e + 2*i);

                if(fw_chip_id == SYSCFG_CUTVersion())
                {
                    LE_ARRAY_TO_UINT16(fw_len, p_merge_addr+0x0e +2*mp_num_of_patch + 2*i);
                    LE_ARRAY_TO_UINT32(fw_offset, p_merge_addr+0x0e +4*mp_num_of_patch + 4*i);
                    if(!CHECK_SW(EFUSE_SW_DRIVER_DEBUG_LOG))
                    {
                        //0
                        hci_board_debug("\n fw_chip_id patch =%x,mp_num_of_patch=%x \n", fw_chip_id,mp_num_of_patch);
                        hci_board_debug("\n  lmp_subversion=%x , fw_len =%x, fw_offset = %x \n", lmp_subversion, fw_len, fw_offset);
                    }
                    break;
                }
            }

            if(i >= mp_num_of_patch)
            {
                hci_board_debug("\n ERROR:no match patch\n");
                return false;
            }
        }

        fw_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, fw_len);
        if(fw_buf == NULL)
        {
            hci_board_debug("\n fw_buf ,malloc %d byte fail, \n", fw_len);
            return false;
        }
        else
        {
            memcpy(fw_buf,p_merge_addr+fw_offset, fw_len);
            LE_UINT32_TO_ARRAY(fw_buf+fw_len-4,lmp_subversion);
        }
    }
    else if(!memcmp(p_merge_addr, single_patch_sing, sizeof(single_patch_sing)))
    {
        hci_board_debug("\nwe use single patch\n");
        if(p_merge_addr != rltk_bt_get_patch_code())
        {
            hci_board_debug("\nnot support single patch on rom\n");
            return false;
        }

        fw_len = rtlbt_fw_len ;
        fw_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, fw_len);
        memcpy(fw_buf,rltk_bt_get_patch_code(), fw_len);
    }
    else
    {
        hci_board_debug("\nWrong patch head %x %x %x %x\n",p_merge_addr[0], p_merge_addr[1], p_merge_addr[2], p_merge_addr[3]);
        return false;
    }

    config_buf = rtlbt_config;
    config_len = rtlbt_config_len;
    //config_len = 0;

    if (config_len != 0)
    {
        if (hci_rtk_parse_config(config_buf, config_len, hci_tp_lgc_efuse) == false)
        {
            return false;
        }
    }


    if (rltk_wlan_is_mp()) 
    {
        hci_board_debug("\nWe use lmp_subversion=%x fw_buf=%x, fw_len = %x, config_buf = %x,config_len= %x, baudrate 0x%x\n", lmp_subversion,fw_buf, fw_len, config_buf, config_len, hci_tp_baudrate);
    }

    hci_set_patch(fw_buf, fw_len, config_buf, config_len, hci_tp_baudrate);

    return true;
}

void bt_read_efuse(void)
{
    u32 Idx = 0;
    u32 ret = 0;
    uint8_t *p_buf;

    p_buf = os_mem_zalloc(RAM_TYPE_DATA_ON, 1024);
    ret = EFUSE_LMAP_READ(p_buf);
    if (ret == _FAIL)
    {
        hci_board_debug("EFUSE_LMAP_READ fail \n");
    }
    memcpy(hci_tp_lgc_efuse, p_buf+0x190,BT_EFUSE_TABLE_LEN);

    if(!CHECK_SW(EFUSE_SW_DRIVER_DEBUG_LOG))
    {
        //0
        hci_board_debug("\r\n==logic_efuse:==\r\n ");
        for (Idx = 0; Idx < BT_EFUSE_TABLE_LEN; Idx++)
        {
            hci_board_debug("%x:", hci_tp_lgc_efuse[Idx]);
        }

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
       EFUSE_PMAP_READ8(0, 0x120 + Idx, hci_tp_phy_efuse + Idx, L25EOUTVOLTAGE);
    }

    if(!CHECK_SW(EFUSE_SW_DRIVER_DEBUG_LOG))
    {
        //0
        hci_board_debug("\r\n==bt phy_efuse 0x120~0x130:==\r\n ");
        for (Idx = 0; Idx < 16; Idx++)
        {
            hci_board_debug("%x:", hci_tp_phy_efuse[Idx]);
        }
    }
    os_mem_free(p_buf);

}


bool hci_board_init()
{
    bool ret=false;

    HCI_PRINT_INFO1("hci_tp_open, this cut is AmebaD %X CUT",SYSCFG_CUTVersion()?0x0B:0x0A);


    if (rltk_wlan_is_mp()) 
    {
        hci_board_debug("\r\n==========this is BT MP DRIVER===========,\r\n this cut is AmebaD %X CUT\r\n",SYSCFG_CUTVersion()+10);
    }

    bt_read_efuse();

    ret = hci_rtk_find_patch();
    if(ret == false)
    {
		hci_board_debug("\r\n%s: error operate\r\n",__FUNCTION__);
        return false;
    }
    return true;
}

void hci_normal_start(void)
{
    bt_coex_pta_only();
}


void bt_power_on(void)
{
    set_reg_value(0x40000000,BIT0 | BIT1, 3);
}

void bt_power_off(void)
{
    set_reg_value(0x40000000,BIT0 | BIT1, 0);
}

void bt_change_gnt_bt_only()
{
    set_reg_value(0x40080764, BIT9 | BIT10,3 );
}

void bt_change_gnt_wifi_only()
{
    set_reg_value(0x40080764, BIT9 | BIT10,1);
}

void hci_uart_out(void)
{
	hci_board_debug("HCI UART OUT OK: PA2 TX, PA4 RX\r\n");
	HAL_WRITE32(0x48000000, 0x5f0, 0x00000202);
    os_delay(50);
    //PA2 TX
	HAL_WRITE32(0x48000000, 0x408, 0x00005C11);
    os_delay(50);
    //PA4 RX
	HAL_WRITE32(0x48000000, 0x410, 0x00005C11);
    os_delay(50);

	HAL_WRITE32(0x48000000, 0x440, 0x00005C11);
    os_delay(50);

    bt_change_gnt_bt_only();
}

void hci_bt_reset_out(void)
{
	hci_board_debug("hci_bt_reset_out:    FULL,\r\n");
	HAL_WRITE32(0x48000000, 0x440, 0x00005C11);

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
	
	HAL_WRITE32(0x40000000, 0, 0xFFFF0003);
	HAL_WRITE32(0x40000000, 0, 0xFFFE0003);

    os_delay(5);


	HAL_WRITE32(0x40000204, 0, 0x01010307);
    os_delay(5);
	
    //BT clock enable
	HAL_WRITE32(0x40000214, 0, 0x01010307);
    os_delay(5);
}

void bt_reset(void)
{
    uint32_t val;
   // hci_board_debug("BT OPEN LOG...\n");


    if (rltk_wlan_is_mp())
    {
        set_reg_value(0x48000440,BIT0 |BIT1 | BIT2 | BIT3 | BIT4, 17);
    }
    else if(!CHECK_SW(EFUSE_SW_BT_FW_LOG))
    {
        //0
        set_reg_value(0x48000440,BIT0 |BIT1 | BIT2 | BIT3 | BIT4, 17);
    }
    //open bt log pa16

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

void bt_step_clear_iqk(void)
{
    FLASH_EreaseDwordsXIP(SPI_FLASH_BASE+FLASH_BT_PARA_ADDR, 4);
    hci_board_debug("bt_step_clear_iqk:    the flash eraseing \r\n");
    bt_dump_flash();
}

bool bt_iqk_flash_valid(BT_Cali_TypeDef  *bt_iqk_data)
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
        bt_iqk_data->IQK_xx = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR);
        bt_iqk_data->IQK_yy = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+4);
        bt_iqk_data->QDAC = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+8);
        bt_iqk_data->IDAC = HAL_READ32(SPI_FLASH_BASE, FLASH_BT_PARA_ADDR+12);
        hci_board_debug("bt_iqk_flash_valid: has data\r\n");
        return true;
	}
}

bool bt_iqk_efuse_valid(BT_Cali_TypeDef  *bt_iqk_data)
{

    if(((hci_tp_phy_efuse[3] == 0xff) && (hci_tp_phy_efuse[4] == 0xff))||
            ((hci_tp_phy_efuse[5] == 0xff)&& (hci_tp_phy_efuse[6] == 0xff))||
            (hci_tp_phy_efuse[0x0c] == 0xff)||
            (hci_tp_phy_efuse[0x0d] == 0xff)
            )
    {
        hci_board_debug("bt_iqk_efuse_valid: no data\r\n");
        return false;
    }
    else
    {
		bt_iqk_data->IQK_xx = hci_tp_phy_efuse[3] | hci_tp_phy_efuse[4] <<8;
		bt_iqk_data->IQK_yy = hci_tp_phy_efuse[5] | hci_tp_phy_efuse[6] <<8;
		bt_iqk_data->QDAC = hci_tp_phy_efuse[0x0c];
		bt_iqk_data->IDAC = hci_tp_phy_efuse[0x0d];
        hci_board_debug("bt_iqk_efuse_valid: has data\r\n");
        return true;
    }
}
bool bt_iqk_logic_efuse_valid(BT_Cali_TypeDef  *bt_iqk_data)
{
    if(((hci_tp_lgc_efuse[0x16] == 0xff) && (hci_tp_lgc_efuse[0x17] == 0xff))||
            ((hci_tp_lgc_efuse[0x18] == 0xff)&&(hci_tp_lgc_efuse[0x19] == 0xff))||
            (hci_tp_lgc_efuse[0x1a] == 0xff) ||
            (hci_tp_lgc_efuse[0x1b] == 0xff) )
    {
        hci_board_debug("bt_iqk_logic_efuse_valid: no data\r\n");
        return false;
    }
    else
    {
		bt_iqk_data->IQK_xx = hci_tp_lgc_efuse[0x16] | hci_tp_lgc_efuse[0x17] <<8;
		bt_iqk_data->IQK_yy = hci_tp_lgc_efuse[0x18] | hci_tp_lgc_efuse[0x19] <<8;
		bt_iqk_data->QDAC = hci_tp_lgc_efuse[0x1a];
		bt_iqk_data->IDAC = hci_tp_lgc_efuse[0x1b];
        hci_board_debug("bt_iqk_logic_efuse_valid: has data\r\n");
        return true;
    }
}

void bt_dump_iqk(BT_Cali_TypeDef *iqk_data)
{
	 	hci_board_debug("bt_iqk_dump:    DUMP,\r\n");
	 	hci_board_debug("the IQK_xx  data is 0x%x,\r\n", iqk_data->IQK_xx);
	 	hci_board_debug("the IQK_yy  data is 0x%x,\r\n", iqk_data->IQK_yy);
	 	hci_board_debug("the QDAC  data is 0x%x,\r\n", iqk_data->QDAC);
	 	hci_board_debug("the IDAC  data is 0x%x,\r\n", iqk_data->IDAC);
}

bool bt_check_iqk(void)
{
	u32 ret = _FAIL;
    BT_Cali_TypeDef     bt_iqk_data;



#if 0
#ifdef MP_MODE_HCI_OUT
#else
	if(bt_iqk_flash_valid(&bt_iqk_data))
	{
        //bt_dump_flash();
        //bt_dump_iqk(&bt_iqk_data);
		bt_lok_write(bt_iqk_data.IDAC, bt_iqk_data.QDAC);
		hci_tp_phy_efuse[0] = 0;
		hci_tp_phy_efuse[1] = 0xfe;
		hci_tp_phy_efuse[2] = 0xff;
		hci_tp_phy_efuse[3] = bt_iqk_data.IQK_xx & 0xff;
		hci_tp_phy_efuse[4] = (bt_iqk_data.IQK_xx >> 8) & 0xff;
		hci_tp_phy_efuse[5] = bt_iqk_data.IQK_yy & 0xff;
		hci_tp_phy_efuse[6] = (bt_iqk_data.IQK_yy >> 8) & 0xff;
        return true;
	}
#endif
#endif
    

    if(!CHECK_SW(EFUSE_SW_USE_LOGIC_EFUSE))
    {
        //0
		hci_board_debug("\r\n%s: USE FIX LOGIC EFUSE\r\n",__FUNCTION__);
        if (bt_iqk_logic_efuse_valid(&bt_iqk_data))
        {
            bt_dump_iqk(&bt_iqk_data);
            bt_lok_write(bt_iqk_data.IDAC, bt_iqk_data.QDAC);
            hci_tp_phy_efuse[0] = 0;
            hci_tp_phy_efuse[1] = 0xfe;
            hci_tp_phy_efuse[2] = 0xff;
            hci_tp_phy_efuse[3] = bt_iqk_data.IQK_xx & 0xff;
            hci_tp_phy_efuse[4] = (bt_iqk_data.IQK_xx >> 8) & 0xff;
            hci_tp_phy_efuse[5] = bt_iqk_data.IQK_yy & 0xff;
            hci_tp_phy_efuse[6] = (bt_iqk_data.IQK_yy >> 8) & 0xff;
            return true;
        }
        return false;
    }

	if (bt_iqk_efuse_valid(&bt_iqk_data))
	{
        bt_dump_iqk(&bt_iqk_data);
		bt_lok_write(bt_iqk_data.IDAC, bt_iqk_data.QDAC);
        return true;
	}

	if (bt_iqk_logic_efuse_valid(&bt_iqk_data))
	{
        bt_dump_iqk(&bt_iqk_data);
		bt_lok_write(bt_iqk_data.IDAC, bt_iqk_data.QDAC);
		hci_tp_phy_efuse[0] = 0;
		hci_tp_phy_efuse[1] = 0xfe;
		hci_tp_phy_efuse[2] = 0xff;
		hci_tp_phy_efuse[3] = bt_iqk_data.IQK_xx & 0xff;
		hci_tp_phy_efuse[4] = (bt_iqk_data.IQK_xx >> 8) & 0xff;
		hci_tp_phy_efuse[5] = bt_iqk_data.IQK_yy & 0xff;
		hci_tp_phy_efuse[6] = (bt_iqk_data.IQK_yy >> 8) & 0xff;
        return true;
	}
	else
	{
		hci_board_debug("bt_check_iqk:  NO IQK LOK DATA need start LOK,\r\n");
		//bt_change_gnt_wifi_only();
		//bt_adda_dck_8721d();
        reset_iqk_type();
		return false;
	}
    return true;
}


bool hci_start_iqk(void)
{
	u32 ret = 0;
   // bt_change_gnt_wifi_only();

    if(rltk_wlan_is_mp())  //JUST FOR DEBUG
    {
        hci_board_debug("BT GNT_BT %x LOG...\n", HAL_READ32(0x40080000, 0x0764));
        ret = bt_iqk_8721d(&g_iqk_data, 0);
        bt_dump_iqk(&g_iqk_data);

#ifdef FIX_OLD_SDK
        hci_board_debug("LOK USE default 0x20, \r\n");
        g_iqk_data.IDAC = 0x20;
        g_iqk_data.QDAC = 0x20;
#endif

        hci_board_debug("\r\n Please write logic efuse 0x1A6 =0x%02x", g_iqk_data.IQK_xx & 0xff);
        hci_board_debug("\r\n Please write logic efuse 0x1A7 =0x%02x", (g_iqk_data.IQK_xx >> 8) & 0xff);
        hci_board_debug("\r\n Please write logic efuse 0x1A8 =0x%02x", g_iqk_data.IQK_yy & 0xff);
        hci_board_debug("\r\n Please write logic efuse 0x1A9 =0x%02x", (g_iqk_data.IQK_yy >> 8) & 0xff);
        hci_board_debug("\r\n Please write logic efuse 0x1AA =0x%02x", g_iqk_data.QDAC);
        hci_board_debug("\r\n Please write logic efuse 0x1AB =0x%02x\r\n", g_iqk_data.IDAC);

    }
    else
    {
        ret = bt_iqk_8721d(&g_iqk_data, 0);
        bt_dump_iqk(&g_iqk_data);
    }


	if(_FAIL == ret)
	{
		hci_board_debug("bt_check_iqk:Warning: IQK Fail, please connect driver !!!!!!!!!\r\n");
		return false;
	}
    bt_lok_write(g_iqk_data.IDAC, g_iqk_data.QDAC);


	hci_tp_phy_efuse[0] = 0;
	hci_tp_phy_efuse[1] = 0xfe;
	hci_tp_phy_efuse[2] = 0xff;
	hci_tp_phy_efuse[3] = g_iqk_data.IQK_xx & 0xff;
	hci_tp_phy_efuse[4] = (g_iqk_data.IQK_xx >> 8) & 0xff;
	hci_tp_phy_efuse[5] = g_iqk_data.IQK_yy & 0xff;
	hci_tp_phy_efuse[6] = (g_iqk_data.IQK_yy >> 8) & 0xff;
    
#ifdef FIX_OLD_SDK
	hci_tp_lgc_efuse[0x16] = g_iqk_data.IQK_xx & 0xff;
	hci_tp_lgc_efuse[0x17] = (g_iqk_data.IQK_xx >> 8) & 0xff;
	hci_tp_lgc_efuse[0x18] = g_iqk_data.IQK_yy & 0xff;
	hci_tp_lgc_efuse[0x19] = (g_iqk_data.IQK_yy >> 8) & 0xff;
#endif
    return true;
}


bool hci_board_complete(void)
{

    if(rltk_wlan_is_mp())
    {
        /*TODO:GNT_BT TO WIFI */
        bt_change_gnt_wifi_only();
        hci_board_debug("EFUSE_SW_MP_MODE: UPPERSTACK NOT UP \r\nGNT_BT %x...\n", HAL_READ32(0x40080000, 0x0764));
        return false;
    }
    else
    {
    }
    hci_board_debug("Start upperStack\n");

    return true;
}

void bt_write_lgc_efuse_value()
{

    hci_board_debug("\r\n write logic efuse 0x1A6 =0x%02x", hci_tp_lgc_efuse[0x16]);
    hci_board_debug("\r\n write logic efuse 0x1A7 =0x%02x", hci_tp_lgc_efuse[0x17]);
    hci_board_debug("\r\n write logic efuse 0x1A8 =0x%02x", hci_tp_lgc_efuse[0x18]);
    hci_board_debug("\r\n write logic efuse 0x1A9 =0x%02x", hci_tp_lgc_efuse[0x19]);
    EFUSE_LMAP_WRITE(0x1A4,8,(uint8_t *)&hci_tp_lgc_efuse[0x14]);
}










