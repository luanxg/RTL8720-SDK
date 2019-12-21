This tts example is used to play a sentence. In order to run the example the following steps must be followed.

	1. Set the parameter CONFIG_EXAMPLE_AUDIO_TTS to 1 in platform_opts.h file

	2. If you put espeak-data file in a SD card, you need to set macro "FLASH" in synthdata.cpp dictionary.cpp
	and voices.cpp to 0. If you don't use a SD card, set macro FLASH to 1.
	
	3. You may need to enlarge the heap size in FreeRTOSConfig under realtek_amebaD_cm4_gcc_verification folder if 
	SD card related operations fail(if wifi is powered off, set configTOTAL_HEAP_SIZE to (( size_t ) ( 150 * 1024 )))
	is ok).

	4. Build and flash the binary to test. You need to 1) uncomment "#CSRC += $(DIR)/audio_sport/audio_tts/example_audio_tts.c"
	in the makefile under /project/realtek_amebaD_cm4_gcc_verification/asdk/make/utilities_example 2) add a rule "make -C tts all"
	under "all: CORE_TARGETS" in the makefile under /project/realtek_amebaD_cm4_gcc_verification/asdk/make/audio.

[Supported List]
	Supported :
	    Ameba-D
	Source code not in project:
	    