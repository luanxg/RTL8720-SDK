This m4a example is used to play m4a files from the SDCARD. In order to run the example the following steps must be followed.

	1. Set the parameter CONFIG_EXAMPLE_AUDIO_M4A to 1 in platform_opts.h file
	
	2. Set the parameter configTOTAL_HEAP_SIZE to 200*1024 in FreeRTOSConfig.h
	
	3. Enable SDRAM Heap in heap_5.c

	3. In the example file "example_audio_m4a.c" set the config parameters in the start of the file

		-->I2S_DMA_PAGE_SIZE :- Should be set to the value of decoded bytes depending upon the frequency of the m4a.
		-->NUM_CHANNELS :- Should be set to CH_MONO if number of channels in m4a file is 1 and CH_STEREO if its 2.
		-->SAMPLING_FREQ:- Check the properties of the m4a file to determine the sampling frequency and choose the appropriate macro.

		The default values of the parameters are pre-set to the values of the sample audio file provided in the folder, in case you are using your own audio file please change the above parameters to the parameters for your audio file else the audio will not be played properly.

	4. Build and flash the binary to test

[Supported List]
	Source code not in project :
	    Ameba-1, Ameba-z, Ameba-pro