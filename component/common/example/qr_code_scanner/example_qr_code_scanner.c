#include "platform_opts.h"

#if CONFIG_EXAMPLE_QR_CODE_SCANNER
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "qr_code_scanner.h"
#include "jpeg_snapshot.h"
#include "wifi_conf.h"

#define QR_CODE_MAX_SCAN_COUNT	5

typedef struct _QR_CODE_SCANNER_CONFIG {
	unsigned int width;
	unsigned int height;
	int x_density;
	int y_density;
	unsigned int scan_count;
} QR_CODE_SCANNER_CONFIG;

QR_CODE_SCANNER_CONFIG qr_code_scanner_config_map[] = {
//	{640,  480,  2, 2, 0},		// QR_CODE_480P_DENSITY_2
	{640,  480,  1, 1, 0},		// QR_CODE_480P_DENSITY_1
//	{1280, 720,  2, 2, 0},		// QR_CODE_720P_DENSITY_2
	{1280, 720,  1, 1, 0},		// QR_CODE_720P_DENSITY_1
//	{1920, 1080, 2, 2, 0},		// QR_CODE_1080P_DENSITY_2
	{1920, 1080, 1, 1, 0}		// QR_CODE_1080P_DENSITY_1
};

typedef enum {
//	QR_CODE_480P_DENSITY_2,		// 480P, Density = 2
	QR_CODE_480P_DENSITY_1,		// 480P, Density = 1
//	QR_CODE_720P_DENSITY_2,		// 720P, Density = 2
	QR_CODE_720P_DENSITY_1,		// 720P, Density = 1
//	QR_CODE_1080P_DENSITY_2,	// 1080P, Density = 2
	QR_CODE_1080P_DENSITY_1,	// 1080P, Density = 1
	QR_CODE_CONFIG_MAX_INDEX
} qr_code_scanner_config_index;

SemaphoreHandle_t g_qr_code_scanner_sema = NULL;

void example_qr_code_scanner_done_event(unsigned char *buf, unsigned int buf_len)
{
	unsigned int index;
	unsigned int count;
	unsigned char ssid_string[33] = {0};		//max: 32
	unsigned int ssid_length = 0;
	unsigned char type_string[7] = {0};			//WEP WPA nopass, max: 6
	rtw_security_t security_type = RTW_SECURITY_OPEN;
	unsigned char password_string[65] = {0};	//max: 64
	unsigned int password_length = 0;
	unsigned char hidden_string[6] = {0};		//true false, max: 5
	unsigned int hidden_type = 0;
	unsigned int error_flag = 0;

	// WIFI:S:SSID;T:<WPA|WEP|nopass>;P:<password>;H:<true|false>;;
	// In SSID and password, special characters ':' ';' and '\' should be escaped with character '\' before.
	if(strncmp(buf, "WIFI:", 5) == 0 && *(buf + buf_len - 1) == ';') {
		printf("%s: WIFI QR code! \r\n", __FUNCTION__);

		index = 5;
		while(index < buf_len - 1) {
			if(strncmp(buf + index, "S:", 2) == 0) {
				index += 2;
				count = 0;

				while(1) {
					if(*(buf + index) == '\\')
					{
						*(ssid_string + count) = *(buf + index + 1);
						index += 2;
					}
					else
					{
						*(ssid_string + count) = *(buf + index);
						index ++;
					}
					count ++;
					if(*(buf + index) == ';')
						break;
				}

				ssid_length = count;

				index ++;
			} else if(strncmp(buf + index, "T:", 2) == 0) {
				index += 2;
				count = 0;

				while(*(buf + index + count) != ';')
					count ++;

				strncpy(type_string, buf + index, count);
				if(strncmp(type_string, "WPA", count) == 0)
					security_type = RTW_SECURITY_WPA2_AES_PSK;
				else if(strncmp(type_string, "WEP", count) == 0)
					security_type = RTW_SECURITY_WEP_PSK;
				else if(strncmp(type_string, "nopass", count) == 0)
					security_type = RTW_SECURITY_OPEN;
				else {
					error_flag = 1;
					printf("%s: type = %s \r\n", __FUNCTION__, type_string);
					break;
				}

				index += count + 1;
			} else if(strncmp(buf + index, "P:", 2) == 0) {
				index += 2;
				count = 0;

				while(1) {
					if(*(buf + index) == '\\')
					{
						*(password_string + count) = *(buf + index + 1);
						index += 2;
					}
					else
					{
						*(password_string + count) = *(buf + index);
						index ++;
					}
					count ++;
					if(*(buf + index) == ';')
						break;
				}

				password_length = count;

				index ++;
			} else if(strncmp(buf + index, "H:", 2) == 0) {
				index += 2;
				count = 0;

				while(*(buf + index + count) != ';')
					count ++;

				strncpy(hidden_string, buf + index, count);
				if(strncmp(hidden_string, "true", count) == 0)
					hidden_type = 1;
				else if(strncmp(hidden_string, "false", count) == 0)
					hidden_type = 0;
				else {
					error_flag = 1;
					printf("%s: hidden = %s \r\n", __FUNCTION__, hidden_string);
					break;
				}

				index += count + 1;
			} else {
				error_flag = 1;
				break;
			}
		}

		if(error_flag)
			printf("%s: Error WIFI QR code! \r\n", __FUNCTION__);
		else {
#if 0
			printf("%s: ssid = %s \r\n", __FUNCTION__, ssid_string);
			printf("%s: ssid_length = %d \r\n", __FUNCTION__, ssid_length);
			printf("%s: type = %s \r\n", __FUNCTION__, type_string);
			printf("%s: security_type = 0x%x \r\n", __FUNCTION__, security_type);
			printf("%s: password = %s \r\n", __FUNCTION__, password_string);
			printf("%s: password_length = %d \r\n", __FUNCTION__, password_length);
			printf("%s: hidden = %s \r\n", __FUNCTION__, hidden_string);
			printf("%s: hidden_type = %d \r\n", __FUNCTION__, hidden_type);
#else
			if(security_type == RTW_SECURITY_OPEN)
				wifi_connect(ssid_string, security_type, NULL, ssid_length, 0, 0, NULL);
			else
				wifi_connect(ssid_string, security_type, password_string, ssid_length, password_length, 0, NULL);
#endif
		}
	} else {
		printf("%s: Not WIFI QR code! \r\n", __FUNCTION__);
		printf("%s: buf = %s \r\n", __FUNCTION__, buf);
	}
}

static void example_qr_code_scanner_thread(void *param)
{
	unsigned char *raw_data;
	unsigned int width;
	unsigned int height;
	int x_density;
	int y_density;
	qr_code_scanner_config_index index;
	qr_code_scanner_result qr_code_result;
	unsigned char *result_string;
	unsigned int result_length;
	unsigned int total_scan_count;

	printf("%s \r\n", __FUNCTION__);

	g_qr_code_scanner_sema = xSemaphoreCreateBinary();
	if(g_qr_code_scanner_sema == NULL) {
		printf("%s: g_qr_code_scanner_sema create fail \r\n", __FUNCTION__);
		goto exit;
	}

	while(1) {
		if(xSemaphoreTake(g_qr_code_scanner_sema, portMAX_DELAY) != pdTRUE)
			break;

		result_string = (unsigned char *)malloc(QR_CODE_MAX_RESULT_LENGTH);
		if(result_string == NULL) {
			printf("%s: result_string malloc fail \r\n", __FUNCTION__);
			break;
		}

		for(int i = 0; i < QR_CODE_CONFIG_MAX_INDEX; i++)
			qr_code_scanner_config_map[i].scan_count = 0;
		total_scan_count = 0;

		index = 0;
		while(total_scan_count < QR_CODE_MAX_SCAN_COUNT) {
			width = qr_code_scanner_config_map[index].width;
			height = qr_code_scanner_config_map[index].height;
			x_density = qr_code_scanner_config_map[index].x_density;
			y_density = qr_code_scanner_config_map[index].y_density;
			qr_code_result = QR_CODE_FAIL_UNSPECIFIC_ERROR;
			memset(result_string, 0, QR_CODE_MAX_RESULT_LENGTH);

			if(yuv_snapshot_isp_config(width, height, 30, 0) < 0)
				printf("%s: yuv_snapshot_isp_config fail \r\n", __FUNCTION__);
			else {
				if(yuv_snapshot_isp(&raw_data) < 0)
					printf("%s: get image from camera fail \r\n", __FUNCTION__);
				else {
					qr_code_result = qr_code_parsing(raw_data, width, height, x_density, y_density, result_string, &result_length);
					qr_code_scanner_config_map[index].scan_count++;
				}
			}

			if(yuv_snapshot_isp_deinit() < 0)
				printf("%s: yuv_snapshot_isp_deinit fail \r\n", __FUNCTION__);
			
			if(qr_code_result == QR_CODE_SUCCESS)
				break;
			else if(qr_code_result == QR_CODE_FAIL_UNSPECIFIC_ERROR) {
				printf("%s: qr_code_scanner_config_map[%d] for QR_CODE_FAIL_UNSPECIFIC_ERROR \r\n", __FUNCTION__, index);
				if(index == QR_CODE_CONFIG_MAX_INDEX - 1)
					break;
				else if(qr_code_scanner_config_map[index].scan_count == 2)
					index++;
			} else if(qr_code_result == QR_CODE_FAIL_NO_FINDER_CENTER) {
				printf("%s: qr_code_scanner_config_map[%d] for QR_CODE_FAIL_NO_FINDER_CENTER \r\n", __FUNCTION__, index);
				if(index == QR_CODE_CONFIG_MAX_INDEX - 1)
					break;
				else
					index++;
			} else if(qr_code_result == QR_CODE_FAIL_DECODE_ERROR) {
				printf("%s: qr_code_scanner_config_map[%d] for QR_CODE_FAIL_DECODE_ERROR \r\n", __FUNCTION__, index);
				if(index == QR_CODE_CONFIG_MAX_INDEX - 1)
					break;
				else if(qr_code_scanner_config_map[index].scan_count == 2)
					index++;
			}

			total_scan_count++;
		}

		if(qr_code_result == QR_CODE_SUCCESS) {
			printf("%s: qr code scan success \r\n", __FUNCTION__);
			example_qr_code_scanner_done_event(result_string, result_length);
		} else
			printf("%s: qr code scan fail \r\n", __FUNCTION__);

		if(result_string) {
			free(result_string);
			result_string = NULL;
		}
	}

exit:
	if(g_qr_code_scanner_sema) {
		vSemaphoreDelete(g_qr_code_scanner_sema);
		g_qr_code_scanner_sema = NULL;
	}

	vTaskDelete(NULL);
}

void example_qr_code_scanner(void)
{
	if(xTaskCreate(example_qr_code_scanner_thread, ((const char *)"example_qr_code_scanner_thread"), 1024, NULL, tskIDLE_PRIORITY + 2, NULL) != pdPASS)
		printf("\n\r%s xTaskCreate(example_qr_code_scanner_thread) failed", __FUNCTION__);
}

#endif /* CONFIG_EXAMPLE_QR_CODE_SCANNER */

