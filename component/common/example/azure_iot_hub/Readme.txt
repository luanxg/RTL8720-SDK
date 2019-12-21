In order to run the example follow the steps:


1. You need to setup an IOT hub on the azure IOT cloud portal.
	-->Steps to setup an IOT cloud portal on azure can be found in the following links:
	>https://github.com/Azure/azure-iot-sdk-c
	>https://github.com/Azure/azure-iot-device-ecosystem/blob/master/setup_iothub.md

2. Once the IOT hub is created, a device needs to be setup on the portal.

3. In order to setup the device on the portal the following steps need to be followed.
	-->https://github.com/Azure/azure-iot-device-ecosystem/blob/master/setup_iothub.md#manage-an-azure-iot-hub

4. Once you create the device identity, the device connection string needs to be pasted inside the example "example_azure_iot_hub.c"

5. Once you paste the connection string you need to add the IOT hub host name and port in the example.

6. The hostname can be found in the azure portal during the setup

7. Once all the setup is done enable the example from platform_opts.h and flash.
