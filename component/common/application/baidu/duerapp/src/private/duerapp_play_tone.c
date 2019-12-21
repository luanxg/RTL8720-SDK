#include "duerapp.h"
#include "duerapp_audio.h"
#include "duerapp_peripheral_audio.h"
#include "mp3_codec.h"
#include "queue.h"
#include "flash_api.h"
#include "device_lock.h"
#include "lightduer_memory.h"

#define BUF_SIZE		1000
#define INPUT_FRAME_SIZE 1500
#define FLASH_ADDR_MASK 0xFF000000

extern mp3_decoder_t mp3;
extern signed short pcm_buf[];
extern unsigned long array_len[];
extern unsigned char *array_ptr[];

QueueHandle_t tone_queue;
extern void play_tone_pause_audio();
extern void play_tone_resume_audio();
void read_data_from_flash(uint32_t addr, uint32_t len, uint8_t * data)
{
	flash_t flash;

	device_mutex_lock(RT_DEV_LOCK_FLASH);
	flash_stream_read(&flash, addr, len, data);
	device_mutex_unlock(RT_DEV_LOCK_FLASH);
}

void play_tone_from_sram(Tones tone_name)
{
	long remain = array_len[tone_name], len_t;
	unsigned char *tone_ptr = array_ptr[tone_name];
	unsigned char *cur_ptr = tone_ptr;

	int offset = 0, res;
	player_t mp3_para = {0};
	mp3_info_t frame_info;

retry:
	len_t = (remain < BUF_SIZE) ? remain : BUF_SIZE;

	offset = MP3FindSyncWord(cur_ptr, len_t);
	if (offset >= 0 && MP3GetNextFrameInfo(&mp3_para, &cur_ptr[offset]) == 0) {
		cur_ptr += offset;
		remain -= offset;
		DUER_LOGI("Find first frame, %d\n", offset);
		HTTP_LOGI("frame info: br %d, nChans %d, sr %d,frame_size %d",
		          mp3_para.bit_rate, mp3_para.nb_channels,
		          mp3_para.sample_rate, mp3_para.frame_size);
		initialize_audio_as_player(&mp3_para);
	} else {
		tone_ptr += len_t;
		remain -= len_t;
		if (remain <= 0) {
			DUER_LOGI("can not find frame header\n");
			return;
		}
		goto retry;
	}

	while (remain > 0) {

		len_t = (remain < mp3_para.frame_size) ? remain : mp3_para.frame_size;
		res = mp3_decode(mp3, tone_ptr, len_t, pcm_buf, &frame_info);
		if (res <= 0)
			break;

		tone_ptr += res;
		remain -= res;
		audio_play_pcm(pcm_buf, frame_info.audio_bytes);
	}
	DUER_LOGI("play tone finished!");
	audio_player_stop(DUER_EXIT_NO_ERROR);


}

void play_mp3_from_flash(long mp3_file_len, uint32_t read_flash_addr)
{

	int i = 0;
	int sync;
	int frame_size = 0;
	unsigned char *cur_ptr;

	long remain = mp3_file_len, len_t;
	uint32_t offset = read_flash_addr;

	printf("offset 0x%x, mp3_file_len 0x%x\n", offset, mp3_file_len);
	mp3_info_t frame_info;
	player_t mp3_para = {0};

	unsigned char *buf = (unsigned char*)DUER_MALLOC(INPUT_FRAME_SIZE);
	if (buf == NULL) {
		DUER_LOGE("Alloc buffer failed!");
		goto Exit;
	}

retry:
	len_t = (remain < INPUT_FRAME_SIZE) ? remain : INPUT_FRAME_SIZE;
	read_data_from_flash(offset, len_t, buf);
	cur_ptr = buf;

	sync = MP3FindSyncWord(cur_ptr, remain);
	if (sync >= 0 && MP3GetNextFrameInfo(&mp3_para, &cur_ptr[sync]) == 0) {
		remain -= sync;
		offset += sync;
	} else {
		remain -= len_t;
		offset += len_t;
		if (remain <= 0) {
			DUER_LOGI("can not find frame header\n");
			goto Exit;
		}
		goto retry;
	}

	initialize_audio_as_player(&mp3_para);

	do {
		len_t = (remain < INPUT_FRAME_SIZE) ? remain : INPUT_FRAME_SIZE;
		memset(buf, 0x00, INPUT_FRAME_SIZE);
		read_data_from_flash(offset, len_t, buf);

		frame_size = mp3_decode(mp3, buf, len_t, pcm_buf, &frame_info);

		if (frame_size <= 0) {
			printf("decode error!\n");
			printf("buffer:\n");
			for (i = 0; i < len_t; i++) {
				if (i % 16 == 0)
					printf("\n");
				printf("%02x ", buf[i]);
			}
			break;
		}

		offset += frame_size;
		remain -= frame_size;
		audio_play_pcm(pcm_buf, frame_info.audio_bytes);
	} while (remain > 0);
	audio_player_stop(DUER_EXIT_NO_ERROR);

Exit:
	if (buf != NULL)
		SAFE_FREE(buf);
}


void play_tone_from_flash(Tones tone_name)
{
	int i = 0;
	int sync;
	int frame_size = 0;

	unsigned char *cur_ptr;
	long remain = array_len[tone_name], len_t;
	uint32_t offset = (uint32_t)array_ptr[tone_name] & (~FLASH_ADDR_MASK);
	mp3_info_t frame_info;
	player_t mp3_para = {0};

	unsigned char *buf = (unsigned char*)DUER_MALLOC(INPUT_FRAME_SIZE);
	if (buf == NULL) {
		DUER_LOGE("Alloc buffer failed!");
		goto Exit;
	}

retry:
	len_t = (remain < INPUT_FRAME_SIZE) ? remain : INPUT_FRAME_SIZE;
	read_data_from_flash(offset, len_t, buf);
	cur_ptr = buf;

	sync = MP3FindSyncWord(cur_ptr, remain);
	if (sync >= 0 && MP3GetNextFrameInfo(&mp3_para, &cur_ptr[sync]) == 0) {
		remain -= sync;
		offset += sync;
	} else {
		remain -= len_t;
		offset += len_t;
		if (remain <= 0) {
			DUER_LOGI("can not find frame header\n");
			goto Exit;
		}
		goto retry;
	}

	initialize_audio_as_player(&mp3_para);

	do {
		len_t = (remain < INPUT_FRAME_SIZE) ? remain : INPUT_FRAME_SIZE;
		memset(buf, 0x00, INPUT_FRAME_SIZE);
		read_data_from_flash(offset, len_t, buf);

		frame_size = mp3_decode(mp3, buf, len_t, pcm_buf, &frame_info);

		if (frame_size <= 0) {
			printf("decode error!\n");
			printf("buffer:\n");
			for (i = 0; i < len_t; i++) {
				if (i % 16 == 0)
					printf("\n");
				printf("%02x ", buf[i]);
			}
			break;
		}

		offset += frame_size;
		remain -= frame_size;
		audio_play_pcm(pcm_buf, frame_info.audio_bytes);
	} while (remain > 0);
	audio_player_stop(DUER_EXIT_NO_ERROR);

Exit:
	if (buf != NULL)
		SAFE_FREE(buf);

}

void play_tone(Tones tone_name)
{
	if (((uint32_t)array_ptr[tone_name] & FLASH_ADDR_MASK) == SPI_FLASH_BASE)
		play_tone_from_flash(tone_name);
	else
		play_tone_from_sram(tone_name);
}

void play_tone_thread(void* param)
{
	int message;
	tone_queue = xQueueCreate(5, sizeof(int));
	if (tone_queue == NULL) {
		DUER_LOGE("Create the tone queue failed!");
		goto Eixt;
	}

	do {
		if (pdTRUE == xQueueReceive(tone_queue, &message, portMAX_DELAY)) {
			play_tone_pause_audio();
			DUER_LOGI("play tone index: %d", message);
			play_tone(message);

			if (message == TONE_HELLO) {
				audio_record_start();
			}
			play_tone_resume_audio();
		} else {
			DUER_LOGE("Queue receive failed!");
		}
	} while (1);

Eixt:
	vTaskDelete(NULL);
}

void tone_enqueue(Tones tone) {
	int msg = (int)tone;

	if (pdPASS != duer_osMailPut(tone_queue, &msg)) {
		DUER_LOGE("Could not send to tone queue");
	}
}

