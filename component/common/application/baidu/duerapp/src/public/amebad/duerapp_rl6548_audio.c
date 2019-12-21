#include "duerapp_audio.h"
#include "duerapp_rl6548_audio.h"
#include "rtl8721d_audio.h"
#include <platform/platform_stdlib.h>
#include "rl6548.h"
#include "duerapp.h"
#include "duerapp_media.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "lightduer_types.h"

SP_GDMA_STRUCT SPGdmaStruct;
SRAM_NOCACHE_DATA_SECTION unsigned char audio_comm_buf[AUDIO_MAX_DMA_PAGE_SIZE*SP_DMA_PAGE_NUM];
SP_OBJ sp_obj;
SP_InitTypeDef SP_InitStruct;

void audio_deinit(void)
{
	audio_play_deinit();
	audio_record_deinit();
}

void audio_set_sp_clk(u32 rate)
{
	u32 div = 0;
	u32 clk_src = 0;
	u32 Tmp = 0;

	// set sport sample rate
	switch(rate){
	case SR_48K:
		div = 8;
		break;
	case SR_96K:
		div = 4;
		break;
	case SR_32K:
		div = 12;
		break;
	case SR_16K:
		div = 24;
		break;
	case SR_8K:
		div = 48;
		break;
	case SR_44P1K:
		clk_src = 1;
		div = 4;
		break;
	case SR_88P2K:
		clk_src = 1;
		div = 2;
		break;
	default:
		DBG_8195A("sample rate not supported!!\n");
		break;
	}
	Tmp = HAL_READ32(SYSTEM_CTRL_BASE_HP, REG_HS_PERI_CLK_CTRL3);
	Tmp &= ~(BIT_MASK_HSYS_AC_SPORT_CKSL << BIT_SHIFT_HSYS_AC_SPORT_CKSL);
	if(clk_src)
		Tmp |= (1 << BIT_SHIFT_HSYS_AC_SPORT_CKSL);
	HAL_WRITE32(SYSTEM_CTRL_BASE_HP, REG_HS_PERI_CLK_CTRL3, Tmp);

	PLL_Div(div);
}
