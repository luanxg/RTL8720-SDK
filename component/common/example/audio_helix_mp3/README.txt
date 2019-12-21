This Helix MP3 example is used to play MP3 files from an binary array. In order to run the example the following steps must be followed.
Helix MP3 example

Description:
Helix MP3 decoder is an MP3 decoder which support MP3 file without ID3 tag.
It has low CPU usage with low memory.

This example show how to use this decoder.

Configuration:
1. [platform_opts.h]
	#define CONFIG_EXAMPLE_AUDIO_HELIX_MP3    1

2. example_audio_helix_mp3.c
	#define AUDIO_SOURCE_BINARY_ARRAY (1)
	#define ADUIO_SOURCE_HTTP_FILE    (0)

	This configuration select audio source. The audio source is mp3 raw data without ID3 tag.

	To test http file as audio source, you need provide a http file location.
	You can use some http file server tool ( Ex. HFS: http://www.rejetto.com/hfs/ )
	Fill address, port, and file name, then you can run this example.

[Supported List]
	Supported :
	    Ameba-1
	Source code not in project:
	    Ameba-z, Ameba-pro