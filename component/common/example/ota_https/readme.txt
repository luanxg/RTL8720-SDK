OTA HTTP UPDATING EXAMPLE

Description:
Download ota.bin from https download server

Configuration:
[example_ota_https.c]
	Modify PORT, HOST and RESOURCE based on your HTTP download server.
	eg: SERVER: http://m-apps.oss-cn-shenzhen.aliyuncs.com/051103061600.bin
	set:	#define PORT	443
		#define HOST	"m-apps.oss-cn-shenzhen.aliyuncs.com"
		#define RESOURCE "051103061600.bin"

[platform_opts.h]
	#define CONFIG_EXAMPLE_OTA_HTTPS    1
[RTL8721d_ota.h]
	#define HTTPS_OTA_UPDATE	

Execution:
Can make automatical Wi-Fi connection when booting by using wlan fast connect example.
A http download example thread will be started automatically when booting.

[Supported List]
	Supported :
	    Ameba-D