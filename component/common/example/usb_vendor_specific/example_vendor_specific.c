/*
* vendor specific example
*/

#include "FreeRTOS.h"
#include "task.h"
#include "example_vendor_specific.h"
#include "vs_intf.h"

//---------------------------------------------------------------------------------------
void example_vendor_specific_main(void)
{

      	_usb_init();
        int status = wait_usb_ready();
	if(status != 0){
		if(status == 2)
			printf("\r\n NO USB device attached\n");
		else
			printf("\r\n USB init fail\n");
		goto error_exit;
	}

	
        if(vs_init() < 0){
            printf("initialize vc fail");
        }

        usb_hcd_post_init();
        printf("\r\nexample_vendor_specific_main delete task......\n");

        vTaskDelete(NULL);
error_exit:
        printf("\r\nERROR, delete\n");
        vTaskDelete(NULL);
}


void example_vendor_specific(void)
{
	/*user can start their own task here*/
	if(xTaskCreate(example_vendor_specific_main, ((const char*)"example_vendor_specific_main"), 512, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
		printf("\r\n example_vendor_specific_main: Create Task Error\n");
	}	
}



