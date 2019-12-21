
THis is an example for USB Video Capture specifically for motion-jpeg capturing. 

Please turn on CONFIG_EXAMPLE_TIMELAPSE for this example to run in application-UVC workspace.

It will use sd host to store captured image from camera by default. For sd environment set up, please refer to amebaiot official website. For time lapse assembling please run laps.bat after storing images. To use one time capture via http service, please turn on CONFIG_USE_HTTP_SERVER. Captured image can be accessed by http homepage request. 

Please MAKE SURE to reserve enough heap size for UVC by raising configTOTAL_HEAP_SIZE in freeRTOSconfig.h & turning off some functions (e.g. WPS, JDSMART, ATcmd for internal and system) since image frame storing could consume quite large memory space.

[Supported List]
	Supported :
	    Ameba-1
	Source code not in project:
	    Ameba-z, Ameba-pro