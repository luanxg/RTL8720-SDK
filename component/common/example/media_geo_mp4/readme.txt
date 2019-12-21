
THis is an example for mux streaming combined with GEO cam & mp4 record. 

If use mmf command callback (USE_MCMD_CB), MP4 callback will send mmf commands for dynamically turning on/off source stream
If not use mmf command callback (USE_MCMD_CB), source will start sending data to sink after init done.

If use mmf command socket (USE_MCMD_SOCKET), user can use socket to send commands start/stop recording.
User can add their own commands if need.

Please MAKE SURE to reserve enough heap size for UVC by raising configTOTAL_HEAP_SIZE in freeRTOSconfig.h & turning off some functions (e.g. WPS, JDSMART, ATcmd for internal and system) since image frame storing could consume quite large memory space.

You may switch to UVC workspace to run the example for all settings has been made for you to run example well.

[Supported List]
	Source code not in project :
	    Ameba-1, Ameba-z, Ameba-pro