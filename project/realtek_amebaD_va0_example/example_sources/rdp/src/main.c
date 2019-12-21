#include "basic_types.h"
#include "ameba_soc.h"
#include "PinNames.h"
#include "diag.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <example_entry.h>

RDP_DATA_SECTION
u32 rdp_data[32] = {
	0x10101010, 0x01010101, 0x02020202, 0x03030303,
	0x04040404, 0x05050505, 0x06060606, 0x07070707,
	0x08080808, 0x09090909, 0x0a0a0a0a, 0x0b0b0b0b, 
	0x0c0c0c0c, 0x0d0d0d0d, 0x0e0e0e0e, 0x0f0f0f0f,
	0x10101010, 0x11111111, 0x12121212, 0x13131313, 
	0x14141414, 0x15151515, 0x16161616, 0x17171717,
	0x18181818, 0x19191919, 0x1a1a1a1a, 0x1b1b1b1b, 
	0x1c1c1c1c, 0x1d1d1d1d, 0x1e1e1e1e, 0x1f1f1f1f
};

#ifdef __GNUC__
__attribute__ ((noinline))  
#endif
RDP_TEXT_SECTION
void rdp_protection_func(u32 idx, u32* presult)
{
	u32 result;
	u32 val;
	
	val = rdp_data[idx];
	result = val/5+3;
	result *=2;
	result +=8;

	*presult = result;
}

const u32 no_rdp_data[32] = {
	0x10101010, 0x01010101, 0x02020202, 0x03030303,
	0x04040404, 0x05050505, 0x06060606, 0x07070707,
	0x08080808, 0x09090909, 0x0a0a0a0a, 0x0b0b0b0b, 
	0x0c0c0c0c, 0x0d0d0d0d, 0x0e0e0e0e, 0x0f0f0f0f,
	0x10101010, 0x11111111, 0x12121212, 0x13131313, 
	0x14141414, 0x15151515, 0x16161616, 0x17171717,
	0x18181818, 0x19191919, 0x1a1a1a1a, 0x1b1b1b1b, 
	0x1c1c1c1c, 0x1d1d1d1d, 0x1e1e1e1e, 0x1f1f1f1f
};

u32 no_protection_func(u32 idx)
{
	u32 result;
	u32 val;
	
	val = no_rdp_data[idx];
	result = val/5+3;
	result *=2;
	result +=8;
	
	return result;
}

void rdp_function_call(void)
{
	int i = 0;
	u32 rdp_result;
	u32 no_rdp_result;
	
	for(i = 0; i < 32; i++){
		rdp_protection_func(i, &rdp_result);
		
		no_rdp_result = no_protection_func(i);
		DBG_8195A("rdp_result = 0x%x, no_rdp_result=0x%x\n", rdp_result, no_rdp_result);
		
		if(rdp_result != no_rdp_result) {
			DBG_8195A("rdp_protection_func call ok \n");
		}
	}
}

void rdp_illegal_read(void)
{
	DBG_8195A("rdp_illegal_read should generate interrupt \n");
	
	DBG_8195A("no_rdp_data[10]: 0x%x, rdp_data[10]: 0x%x\n", no_rdp_data[10], rdp_data[10]);
}

void rdp_irq_handle(VOID * Data)
{
	DBG_8195A("\nIllegal Read RDP Region \n");
}

void rdp_demo(void)
{
	if(!sys_get_rdp_valid()) {
		InterruptRegister((IRQ_FUN) rdp_irq_handle, RDP_IRQ, NULL, 0);
		InterruptEn(RDP_IRQ, 0);

		rdp_function_call();
		rdp_illegal_read();
	} else {
		DBG_8195A("RDP Invalid status=%d \n",sys_get_rdp_valid());
	}

	vTaskDelete(NULL);
}

void main(void)
{
	DBG_8195A("RDP demo main \n\r");
	
	// create demo Task
	if(xTaskCreate( (TaskFunction_t)rdp_demo, "RDP DEMO", (2048/4), NULL, (tskIDLE_PRIORITY + 1), NULL)!= pdPASS) {
		DBG_8195A("Cannot create demo task\n\r");
		goto end_demo;
	}

#ifdef PLATFORM_FREERTOS
	vTaskStartScheduler();
#endif

end_demo:	
	while(1);
}
