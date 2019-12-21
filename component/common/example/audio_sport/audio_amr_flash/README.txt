This amr example is used to play amr files from the SDCARD. In order to run the example the following steps must be followed.



	1. Set the parameter CONFIG_EXAMPLE_AUDIO_AMR_FLASH to 1 in platform_opts.h file
	   
	   un-comment following 2 items in makefile under in makefile under \project\realtek_amebaD_cm4_gcc_verification\asdk\make\utilities_example folder
	   CSRC += $(DIR)/audio_sport/audio_amr_flash/example_audio_amr_flash.c
	   CSRC += $(DIR)/audio_sport/audio_amr_flash/audio_play.c
	   
	   add the following item under "all: CORE_TARGETS" as a rule in makefile under \project\realtek_amebaD_cm4_gcc_verification\asdk\make\audio folder
	   	make -C amr all

	2. In the example file "example_audio_amr_flash.c" set the config parameters in the start of the file

		-->NB_SP_DMA_PAGE_SIZE or WB_SP_DMA_PAGE_SIZE :- Should be set to 320 for amrnb or 640 for amrwb.
		-->NUM_CHANNELS :- Should be set to CH_MONO.
		-->SAMPLING_FREQ:- Check the properties of the amr file to determine the sampling frequency and choose the appropriate macro.

		The default values of the parameters are pre-set to the values of the sample audio file provided in the folder, in case you are using your own audio file please change the above parameters to the parameters for your audio file else the audio will not be played properly.

	3. Build and flash the binary to test

[Supported List]
	Source code not in project:
	    Ameba-1, Ameba-z, Ameba-pro