#include "FreeRTOS.h"
#include "task.h"
#include "diag.h"
#include "osdep_service.h"
#include "device_lock.h"

extern uint32_t NS_ENTRY secure_api_gw (void* a0, int a1, char*(*a2)(void*));
extern void NS_ENTRY secure_main (void);

//CIPHER_AES_256_CBC
const unsigned char aes_enc_res[16] =
{
    0xF5, 0x8C, 0x4C, 0x04, 0xD6, 0xE5, 0xF1, 0xBA,
    0x77, 0x9E, 0xAB, 0xFB, 0x5F, 0x7B, 0xFB, 0xD6
};

char *secure_callback(void* param)
{
	if (memcmp(&aes_enc_res[0], param, sizeof(aes_enc_res)) == 0) {
		rt_printf("Callback from S world successfully\n\r");
		return "OK";
	} 
	return "ERR";
}

static void non_secure_app(void *param)
{
	device_mutex_lock(RT_DEV_LOCK_CRYPTO);
	secure_api_gw((void *)aes_enc_res, sizeof(aes_enc_res), secure_callback);  
	device_mutex_unlock(RT_DEV_LOCK_CRYPTO);
	vTaskDelete(NULL);
}

void example_trust_zone(void)
{
	/* Initialize application in secure world */
	secure_main();

	/* Start application in non-secure world */
	xSecureTaskCreate(non_secure_app, "non_secure_app", 256, 256, NULL, tskIDLE_PRIORITY + 1, NULL);
}
