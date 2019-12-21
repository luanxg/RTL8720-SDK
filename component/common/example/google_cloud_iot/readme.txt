GOOGLE CLOUD IOT EXAMPLE

Description:
Connect to Google Cloud IoT Platform to publish messages. You need to regist a registry and a device using Google Cloud SDK. And using OPENSSL to create a RSA key pair.

Configuration:
1. Add and enable the Google Cloud IoT example
	[platform_opts.h]
		#define CONFIG_EXAMPLE_GOOGLE_CLOUD_IOT 1
2. Change the NTP server to time1.google.com
	[sntp.c]
		#define SNTP_SERVER_ADDRESS         "time1.google.com"
3. Undef the MQTT_TASK to enable the mqtt related configuration
	[MQTTClient.h]
		//#define MQTT_TASK
4. Eable the necessary function for MbedTLS or PolarSSL
	If using POLARSSL
		[config_rsa.h]
			#define POLARSSL_PK_PARSE_C
			#define POLARSSL_PKCS1_V15
	Else if using MbedTLS
		[platform_opts.h]
			#define CONFIG_USE_POLARSSL     0
			#define CONFIG_USE_MBEDTLS      1
		[config_rsa.h]
			#define MBEDTLS_CERTS_C
			#define MBEDTLS_PK_PARSE_C
			#define MBEDTLS_PKCS1_V15
5. Increase the heap size for this application
	[FreeRTOSConfig.h]
		#define configTOTAL_HEAP_SIZE			( ( size_t ) ( 120 * 1024 ) )	// use HEAP5
6. Set the information of a device
	[example_google_cloud_iot.c]
		project_id
		registry_id
		device_id
		count(The number of messages will be sent)
		private_key(The public key already sent to server to regist the device)

Execution:
You need to connect to WiFi manually by AT command. After connected the mqtt connection will preceed.

[Supported List]
	Source code not in project :
	    Ameba-1, Ameba-z, Ameba-pro