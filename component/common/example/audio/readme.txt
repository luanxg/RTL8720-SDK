This audio example is used to play bird sound in birds_44100_2ch_16b.c from memory or SD CARD. In order to run the example the following steps must be followed.

	1. Set the parameter CONFIG_PLAY_SD_WAV to 0(memory) or 1(SD card) in example_audio.c
	2. If use ALC5651 extended board , confirm #define CONFIG_EXAMPLE_CODEC_ALC5651 1
	3. Build and flash the binary to test

[Supported List]
	Source code not in project :
	    Ameba-1, Ameba-z, Ameba-pro