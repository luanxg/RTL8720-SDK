#include "example_audio_ac3.h"
#include <platform/platform_stdlib.h>
#include "platform_opts.h"
#include "section_config.h"

//------------------------------------- ---CONFIG Parameters-----------------------------------------------//
//real audio files are difined in a52dec.c
#define FILE_NAME "AudioSDTest.ac3"    

//------------------------------------- ---CONFIG Parameters-----------------------------------------------//

void audio_play_flash_ac3(u8* filename){
	int InputParaNum = 0,char_index = 0;
	char InputParas[50][20];
	char INStr[50*20]= "a52dec -o sport -c -r -a";
	char *str = INStr;
	char *In[50];   
	while(*str !='\0')
	{
		while(*str ==' ')
		{
			str++;
		};
		InputParas[InputParaNum][char_index]= *str;
		str++;char_index++;
		if((*str ==' ')||(*str =='\0'))
		{
			InputParas[InputParaNum][char_index] = '\0';
			In[InputParaNum] = &InputParas[InputParaNum][0];
			InputParaNum++;char_index=0;
			while(*str ==' ')
			{
				str++;
			};
		}
	}
	a52dec_main(InputParaNum, In);
}

void example_audio_ac3_thread(void)
{
	printf("Audio codec ac3 demo begin......\n");
	char wav[16] = FILE_NAME;
	audio_play_flash_ac3(wav);
exit:
	vTaskDelete(NULL);
}

void example_audio_ac3(void)
{
	if(xTaskCreate(example_audio_ac3_thread, ((const char*)"example_audio_ac3_thread"), 2000, NULL, tskIDLE_PRIORITY + 1, NULL) != pdPASS)
	printf("\n\r%s xTaskCreate(example_audio_mp3_thread) failed", __FUNCTION__);
}

