This flac example is used to play flac files from the SDCARD. In order to run the example the following steps must be followed.



	1. Set the parameter CONFIG_EXAMPLE_AUDIO_FLAC to 1 in platform_opts.h file

	2. In the example file "example_audio_flac.c" set the config parameters in the start of the file

		-->MP3_DECODE_SIZE :- Should be set to the value of decoded bytes depending upon the frequency of the mp3.
		-->NUM_CHANNELS :- Should be set to CH_MONO if number of channels in mp3 file is 1 and CH_STEREO if its 2.
		-->SAMPLING_FREQ:- Check the properties of the mp3 file to determine the sampling frequency and choose the appropriate macro.

		The default values of the parameters are pre-set to the values of the sample audio file provided in the folder, in case you are using your own audio file please change the above parameters to the parameters for your audio file else the audio will not be played properly.

	3. Build and flash the binary to test

[Supported List]
	Supported :
	    Ameba-D
	Source code not in project:
	    