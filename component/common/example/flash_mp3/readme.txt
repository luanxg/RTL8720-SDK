This mp3 example is used to play mp3 files from the FLASH memory. In order to run the example the following steps must be followed.



	1. Set the parameter CONFIG_EXAMPLE_FLASH_MP3 to 1 in platform_opts.h file

	2. In ffconf.h, set the MACROS as below
		--> _USE_MKFS to 1 which creates FAT volume on Flash
		--> _MAX_SS to 4096 which is the maximum range of supported sector size

	3. In the example file "example_flash_mp3.c" set the config parameters in the start of the file

		-->I2S_DMA_PAGE_SIZE :- Should be set to the value of decoded bytes depending upon the frequency of the mp3.
		-->NUM_CHANNELS :- Should be set to CH_MONO if number of channels in mp3 file is 1 and CH_STEREO if its 2.
		-->SAMPLING_FREQ:- Check the properties of the mp3 file to determine the sampling frequency and choose the appropriate macro.

		The default values of the parameters are pre-set to the values of the sample audio file provided in the folder, in case you are using your own audio file please change the above parameters to the parameters for your audio file else the audio will not be played properly.

	4. The mp3 audio file is uploaded to Ameba through HTTP server before written to the flash memory
		
		--> Set the below server parameters at the start of the file in "example_flash_mp3.c" 

			-> SERVER_HOST :- "ipv4 adddress" of the server
			-> SERVER_PORT :- server port number
			-> RESOURCE :- path of the audio file eg:  "/repository/IOT/AudioFlash.mp3", it can be empty if the audio file is inside the DownloadServer(HTTP) inside tools folder of SDK

	5. 512 KB of on-board Ameba flash memory with the starting adddress of #define FLASH_APP_BASE  0x180000 is used for this example, refer flash_fatfs.c
		-->  Change the FLASH_APP_BASE (starting address) according to your requirements
 
	6. Build and flash the binary to test

[Supported List]
	Source code not in project :
	    Ameba-1, Ameba-z, Ameba-pro