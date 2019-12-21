#include "FreeRTOS.h"
#include "task.h"
#include <platform/platform_stdlib.h>
#include <platform_opts.h>
#include <wifi_conf.h>
#include <osdep_service.h>
#include <platform_opts_bt.h>
#include <gap_conn_le.h>
#if defined(CONFIG_PLATFORM_8710C) && defined(CONFIG_FTL_ENABLED)
#include "ftl_8710c.h"
#endif

#if CONFIG_BT

#if defined(CONFIG_PLATFORM_8710C) && defined(CONFIG_FTL_ENABLED)
ftl_t bt_ftl_obj;
const u32 phy_paddr[2] = {
	BT_FTL_PHY_ADDR0, 
	BT_FTL_PHY_ADDR1
};

void app_ftl_init(void)
{
	ftl_t* obj = &bt_ftl_obj;
	
	void *ptr = NULL;
	u32 logical_size = 2800;
	
	DBG_INFO_MSG_OFF(_DBG_SPI_FLASH_);
	DBG_ERR_MSG_ON(_DBG_SPI_FLASH_);
	
	ptr = pvPortMalloc(logical_size);
	if(ptr)
		obj->logical_map = ptr;
	else {
		DBG_SPIF_ERR("fail to allocate buffer!\n");
		return;
	}
	
	obj->logical_map_size = logical_size;
	obj->log_map_bkup_addr = BT_FTL_BKUP_ADDR;
	obj->phy_page_addr = (uint32_t *)phy_paddr;
	obj->phy_page_num = 2;
	
	ftl_init(obj);
}
#endif //CONFIG_PLATFORM_8710C && CONFIG_FTL_ENABLED

extern void bt_coex_init(void);
extern void wifi_btcoex_set_bt_on(void);

void bt_example_init_thread(void *param)
{
	T_GAP_DEV_STATE new_state;
	
	/*Wait WIFI init complete*/
	while( !wifi_is_up(RTW_STA_INTERFACE) ) {
		vTaskDelay(1000 / portTICK_RATE_MS);
	}
	wifi_disable_powersave();
	
	/*Init BT*/
#if CONFIG_BT_PERIPHERAL
	ble_app_main();
#endif
#if CONFIG_BT_CENTRAL
	ble_central_app_main();
#endif
#if CONFIG_BT_BEACON
	bt_beacon_app_main();
#endif
#if CONFIG_BT_MESH_PROVISIONER
	ble_app_main();
#endif
#if CONFIG_BT_MESH_DEVICE
	ble_app_main();
#endif
	bt_coex_init();

	/*Wait BT init complete*/
	do {
		vTaskDelay(100 / portTICK_RATE_MS);
		le_get_gap_param(GAP_PARAM_DEV_STATE , &new_state);
	}while(new_state.gap_init_state != GAP_INIT_STATE_STACK_READY);

	/*Start BT WIFI coexsitence*/
	wifi_btcoex_set_bt_on();
	
	vTaskDelete(NULL);
}

void bt_example_init()
{
	if(xTaskCreate(bt_example_init_thread, ((const char*)"bt example init"), 1024, NULL, tskIDLE_PRIORITY + 5 + PRIORITIE_OFFSET, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(bt example init) failed", __FUNCTION__);

}

#endif //CONFIG_BT
