This ac3 example is used to play ac3 files from the array. In order to run the example the following steps must be followed.



	1. Set the parameter CONFIG_EXAMPLE_AUDIO_AC3 to 1 in platform_opts.h file

	2. In the example file "example_audio_ac3.c" set the config parameters in the start of the file

		-->
		-->
		-->

		The default values of the parameters are pre-set to the values of the sample audio file provided in the folder, in case you are using your own audio file please change the above parameters to the parameters for your audio file else the audio will not be played properly.

	3. Build and flash the binary to test

[Supported List]
	Supported :
	    Ameba-D
	Source code not in project:
	    