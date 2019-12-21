#include "log_service.h"
#include "atcmd_bt.h"
#include <platform/platform_stdlib.h>
#if defined(CONFIG_PLATFORM_8710C)
#include <platform_opts_bt.h>
#endif

#if defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL
extern int ble_central_at_cmd_connect(int argc, char **argv);
void fATBC (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
		if(strcmp(argv[1],"?") == 0)
			goto exit;
	}

	if ( argc != 3)
		goto exit;

	AT_PRINTK("[ATBC]: _AT_BT_GATT_CONNECT_[%s,%s]",argv[1],argv[2]);
	ble_central_at_cmd_connect(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBC] Connect to remote device: ATBC=P/R,BLE_BD_ADDR");
	AT_PRINTK("[ATBC] P=public, R=random");
	AT_PRINTK("[ATBS] eg:ATBC=P,001122334455");
}

extern int ble_central_at_cmd_disconnect(int argc, char **argv);
void fATBD (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
		if(strcmp(argv[1],"?") == 0)
			goto exit;
	}

	if ( argc != 2)
		goto exit;

	ble_central_at_cmd_disconnect(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBD] Disconnect to remote device: ATBD=connect_id");
	AT_PRINTK("[ATBD] eg:ATBD=0");
}

extern int ble_central_at_cmd_get_conn_info(int argc, char **argv);
void fATBI (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
		if(strcmp(argv[1],"?") == 0)
			goto exit;
	}

	ble_central_at_cmd_get_conn_info(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBI] Get all connected device information, ATBI");
	AT_PRINTK("[ATBI] eg:ATBI");
}

extern int ble_central_at_cmd_get(int argc, char **argv) ;
void fATBG(void* arg)
{
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
		if(strcmp(argv[1],"?") == 0)
			goto exit;
	}

	if(argc < 3)
		goto exit;

	ble_central_at_cmd_get(argc, argv);
	return;

exit:	
	AT_PRINTK("[ATBG] Get all services: ATBG=ALL,connect_id");
	AT_PRINTK("[ATBG] Discover services by uuid: ATBG=SRV,connect_id,uuid_type,uuid");
	AT_PRINTK("[ATBG] Discover characteristic: ATBG=CHARDIS,connect_id,start_handle,end_handle");
	AT_PRINTK("[ATBG] Discover characteristic by uuid: ATBG=CHARUUID,connect_id,start_handle,end_handle, type, uuid");
	AT_PRINTK("[ATBG] Discover characteristic descriptor: ATBG=CHARDDIS,connect_id,start_handle,end_handle");	
	AT_PRINTK("[ATBG] eg:ATBG=ALL,0");
	AT_PRINTK("[ATBG] eg(uuid16):ATBG=SRV,0,0,1803");
	AT_PRINTK("[ATBG] eg(uuid128):ATBG=SRV,0,1,00112233445566778899aabbccddeeff");
	AT_PRINTK("[ATBG] eg:ATBG=CHARDIS,0,0x8,0xFF");
	AT_PRINTK("[ATBG] eg(uuid16):ATBG=CHARUUID,0,0x1,0xFFFF,0,B001");
	AT_PRINTK("[ATBG] eg(uuid16):ATBG=CHARUUID,0,0x1,0xFFFF,0,00112233445566778899aabbccddeeff");
	AT_PRINTK("[ATBG] eg:ATBG=CHARDDIS,0,0xe,0x10"); 

}

extern int ble_central_at_cmd_scan(int argc, char **argv);
void fATBS(void * arg)
{
	int argc = 0;
	int ret;
	char *argv[MAX_ARGC] = {0};
	char buf[256] = {0};
	
	if(arg){
		strcpy(buf, arg);
		argc = parse_param(buf, argv);
		if(strcmp(argv[1],"?") == 0)
			goto exit;
	}

	ble_central_at_cmd_scan(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBS] Scan:ATBS=scan_enable,filter_policy,filter_duplicate");
	AT_PRINTK("[ATBS] [scan_enable]=0-(start scan),1(stop scan)");
	AT_PRINTK("[ATBS] [filter_policy]: 0-(any), 1-(whitelist), 2-(any RPA), 3-(whitelist RPA)");
	AT_PRINTK("[ATBS] [filter_duplicate]: 0-(disable), 1-(enable)");
	AT_PRINTK("[ATBS] eg:ATBS=1");
	AT_PRINTK("[ATBS] eg:ATBS=1,0");
	AT_PRINTK("[ATBS] eg:ATBS=1,0,1");
	AT_PRINTK("[ATBS] eg:ATBS=0");
}

extern int ble_central_at_cmd_auth(int argc, char **argv);
void fATBK(void * arg)
{
	int argc = 0;
	int ret;
	char *argv[MAX_ARGC] = {0};
	char buf[256] = {0};
	
	if(arg){
		strcpy(buf, arg);
		argc = parse_param(buf, argv);
		if(strcmp(argv[1],"?") == 0)
			goto exit;
	}

	ble_central_at_cmd_auth(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBK] Config and Set authentication information");
	AT_PRINTK("[ATBK] ATBK=SEND,conn_id");
	AT_PRINTK("[ATBS] ATBK=KEY,conn_id,passcode");
	AT_PRINTK("[ATBS] ATBK=MODE,auth_flags,io_cap,sec_enable,oob_enable");
	AT_PRINTK("[ATBK] eg:ATBK=SEND,0");
	AT_PRINTK("[ATBK] eg:ATBK=KEY,0,123456");
	AT_PRINTK("[ATBK] eg:ATBK=MODE,0x5,2,1,0");
}

extern int ble_central_at_cmd_send_userconf(int argc, char **argv);
void fATBY(void * arg)
{
	int argc = 0;
	int ret;
	char *argv[MAX_ARGC] = {0};
	char buf[256] = {0};
	
	if(arg){
		strcpy(buf, arg);
		argc = parse_param(buf, argv);
		if(strcmp(argv[1],"?") == 0)
			goto exit;
	}

	if(arg != 3)
		goto exit;

		
	ble_central_at_cmd_send_userconf(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBY] Send user confirmation when show GAP_MSG_LE_BOND_USER_CONFIRMATION");
	AT_PRINTK("[ATBY] ATBY=[conn_id],[conf]");
	AT_PRINTK("[ATBY] [conf]=0-(Reject),1(Accept)");
	AT_PRINTK("[ATBY] eg:ATBY=0,1");
}

extern int ble_central_at_cmd_read(int argc, char **argv);
void fATBR (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	
	if(arg){
		argc = parse_param(arg, argv);
		if(strcmp(argv[1], "?") == 0)
			goto exit;
	}

	if((argc != 3) && (argc != 6))
		goto exit;

	ble_central_at_cmd_read(argc, argv);
	return;
exit:
	AT_PRINTK("[ATBR] Read characteristic: ATBG=conn_id,handle");
	AT_PRINTK("[ATBR] Read characterristic value by uuid: ATBG=conn_id,start_handle,end_handle,uuid_type,uuid");
	AT_PRINTK("[ATBR] eg(uuid16):ATBG=0,0xa");
	AT_PRINTK("[ATBR] eg(uuid16):ATBG=0,0x1,0xFFFF,0,B001");
	AT_PRINTK("[ATBR] eg(uuid128):ATBG=0,0x1,0xFFFF,0,00112233445566778899aabbccddeeff");
}

extern int ble_central_at_cmd_write(int argc, char **argv);
void fATBW (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
		if(strcmp(argv[1], "?") == 0)
			goto exit;
	}

	AT_PRINTK("[ATBH]: _AT_BT_DELETE_PAIRED_INFO_[%s,%s], argc = %d", argv[1], argv[2], argc);

	if((argc != 5) && (argc != 6))
		goto exit;

	ble_central_at_cmd_write(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBW] Write data to service: ATBW=conn_id,type,handle,length,value");
	AT_PRINTK("[ATBW] [type]: write type: 1-(write request), 2-(write command)");
	AT_PRINTK("[ATBW] [handle]: attribute handle");
	AT_PRINTK("[ATBW] [length]: value length");
	AT_PRINTK("[ATBW] [lvalue]: overwrite the value");
	AT_PRINTK("[ATBW] eg:ATBW=0,1,0xc,0x1,02");
	AT_PRINTK("[ATBW] eg:ATBW=0,2,0x19,10");
}

extern int ble_central_at_cmd_update_conn_request(int argc, char **argv);
void fATBU (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	
	if(arg){
		argc = parse_param(arg, argv);
		if(strcmp(argv[1], "?") == 0)
			goto exit;
	}

	if(argc != 6)
		goto exit;

	ble_central_at_cmd_update_conn_request(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBU] Connection param update request: ATBU=conn_id,interval_min,interval_max,latency,supervision_timeout");
	AT_PRINTK("[ATBU] eg:ATBU=0,0x30,0x40,0x0,0x1F4");
}

extern int ble_central_at_cmd_bond_information(int argc, char **argv);
void fATBO (void *arg) {
	int argc = 0;
	char *argv[MAX_ARGC] = {0};
	
	if(arg){
		argc = parse_param(arg, argv);
		if(strcmp(argv[1], "?") == 0)
			goto exit;
	}

	if(argc != 2)
		goto exit;

	ble_central_at_cmd_bond_information(argc, argv);
	return;
	
exit:
	AT_PRINTK("[ATBO] Clear bond information :ATBO=CLEAR");
	AT_PRINTK("[ATBO] Get bond information :ATBO=INFO");
}
#endif

#if defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG
extern int bt_config_app_init(void);
extern void bt_config_app_deinit(bool deinit_bt_stack);
void fATBB(void *arg)
{
	int argc = 0;
	int param = 0;
	char *argv[MAX_ARGC] = {0};
	if (!arg)
		goto exit;
	
	argc = parse_param(arg, argv);
	
	if (argc < 2)
		goto exit;
	
	param = atoi(argv[1]);
	if (param == 1) {
		AT_PRINTK("[ATBB]:_AT_BT_CONFIG_[ON]\n\r");
		bt_config_app_init();
	}
	else if (param == 0) {
		bool deinit_bt_stack = 1;
		if (argc >= 3) {
			deinit_bt_stack =  (bool) atoi(argv[2]);
		}
		AT_PRINTK("[ATBB]:_AT_BT_CONFIG_[OFF, %s]\n\r", (deinit_bt_stack ? "deinit BT STACK":"Don't deinit BT STACK"));
		bt_config_app_deinit(deinit_bt_stack);
	}
	else
		goto exit;
	
	return;
exit:
	AT_PRINTK("[ATBB] Start BT Config: ATBB=1");
	AT_PRINTK("[ATBB] Stop  BT Config: ATBB=0,DEINIT_BT_STACK  <DEINIT_BT_STACK: 0/1>");
}

void fATBM(void *arg)
{
	unsigned int argc = 0;
	char *argv[MAX_ARGC] = {0};

	if(arg){
		argc = parse_param(arg, argv);
	}
	
	if((argc == 2) && (strcmp(argv[1], "?") == 0)){
		AT_PRINTK("provision cmd example: ATBM=pro,cmd,parametr \n");
		AT_PRINTK("device cmd example: ATBM=dev,cmd,parametr \n");
		goto exit;
	}

	if(argc < 3){
		AT_PRINTK("[ATBM]:not enough number of parameter,please use ATBM=? for help \n");
		goto exit;
	}	
	
	if(strcmp(argv[1],"pro") == 0){
		AT_PRINTK("[ATBM]:Provisioner Cmd \n");
	}
	else if(strcmp(argv[1],"dev") == 0){
		AT_PRINTK("[ATBM]:Device Cmd \n");
	}else{
		AT_PRINTK("[ATBM]:It must be dev or pro, please use ATBM=? to help \n");
		goto exit;
	}
	
	bt_mesh_set_cmd((argc-2),&argv[2]);

exit:
	return;
	
}
#endif

#if ((defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
	(defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG)	|| \
	(defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER) || \
	(defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE)) 
log_item_t at_bt_items[ ] = {
#if defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL
	{"ATBC", fATBC,}, // Create a GATT connection	
	{"ATBD", fATBD,}, // Disconnect GATT Connection
	{"ATBG", fATBG,}, // Get peripheral information
	{"ATBI", fATBI,}, // Get information of connected device
	{"ATBK", fATBK,}, // Reply GAP passkey
	{"ATBS", fATBS,}, // Scan BT
	{"ATBY", fATBY,}, // Reply GAP user confirm
	{"ATBR", fATBR,}, // GATT client read
	{"ATBW", fATBW,}, // GATT client write
	{"ATBU", fATBU,}, // Update connection request
	{"ATBO", fATBO,}, // Get / clear bond information
#endif
#if defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG
	{"ATBB", fATBB,},
#endif

#if ((defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER) || \
	(defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE)) 
	{"ATBM",fATBM,},
#endif
};
#endif

void at_bt_init(void)
{
#if ((defined(CONFIG_BT_CENTRAL) && CONFIG_BT_CENTRAL) || \
	(defined(CONFIG_BT_CONFIG) && CONFIG_BT_CONFIG) || \
	(defined(CONFIG_BT_MESH_PROVISIONER) && CONFIG_BT_MESH_PROVISIONER) || \
	(defined(CONFIG_BT_MESH_DEVICE) && CONFIG_BT_MESH_DEVICE)) 
	log_service_add_table(at_bt_items, sizeof(at_bt_items)/sizeof(at_bt_items[0]));
#endif
}

#if SUPPORT_LOG_SERVICE
log_module_init(at_bt_init);
#endif

