#ifndef DUERAPP_MEDIA_H
#define DUERAPP_MEDIA_H

#include "duerapp_config.h"
#include "stdbool.h"

#define URL_LEN  512
#define TOKEN_LEN  512
#define TOC_TABLE_NUM 100

typedef enum {
	NONE = 0,
	SPEAK,
	AUDIO,
	ALERT,
} duer_directive_type;

typedef enum {
	state_chunk_size_processing = 0,
	state_chunk_data_processing,
	state_chunk_r_processing,
	state_chunk_n_processing,
} duer_chunk_state;

typedef enum {
	DUER_PLAY_OK,
	DUER_PLAY_ERROR_UNKNOWN      		= -1,
	DUER_PLAY_ERROR_MALLOC_FAIL 		= -2,
	DUER_PLAY_ERROR_PARSE_INFO 			= -3,
	DUER_PLAY_ERROR_UNSUPPORT_CODEC 	= -4,
	DUER_PLAY_ERROR_READ_FAIL 			= -5,
	DUER_PLAY_ERROR_BAD_FRAME 			= -6,
	DUER_PLAY_STOP_BY_FLAG 				= -7,
	DUER_PLAY_STOP_BY_OTHER 			= -8,
} duer_media_play_error_t;
	
typedef struct {
	duer_directive_type ddt;
	int  offset;
	char token[TOKEN_LEN];
	char *download_url;
} duer_media_param_t;

typedef struct {
	char *host;
	char *url;
	char *redirect_url;
	char *buffer;
	char *cur_ptr;

	int server_fd;
	int dest_port;
	int status_code;
	int chunked_flag;
	int chunked_len;
	int content_len;
	int range_len;
	int offset;
	int bytesleft;
	int recv_data_len;
	int got_frame_info;
} http_t;

//save mp3 info
typedef struct {
	char url[URL_LEN];
	u8 is_parsed;
	u8 is_vbr;
	u8 mpeg_ver;
	u8 layer_ver;
	u32 sample_num;
	u32 bit_rate;
	u32 sample_rate;
    u32 nb_channels;
	u32 frame_size;
	u32 side_info_size;

	u32 xing_total_frame;
	u32 xing_total_size;
	u8 toc_table[TOC_TABLE_NUM];

	u32 content_len;
	u32 range_len;
	u32 duration;
	u32 cur_time;
	u32 cur_download_offset;
	u32 cur_played_offset;

	duer_directive_type ddt;
	int downloadBytes;
	char token[TOKEN_LEN];
}duer_media_info_t;

struct WAV_Format {
	uint32_t ChunkID;	/* "RIFF" */
	uint32_t ChunkSize;	/* 36 + Subchunk2Size */
	uint32_t Format;	/* "WAVE" */
 
	/* sub-chunk "fmt" */
	uint32_t Subchunk1ID;	/* "fmt " */
	uint32_t Subchunk1Size;	/* 16 for PCM */
	uint16_t AudioFormat;	/* PCM = 1*/
	uint16_t NumChannels;	/* Mono = 1, Stereo = 2, etc. */
	uint32_t SampleRate;	/* 8000, 44100, etc. */
	uint32_t ByteRate;	/* = SampleRate * NumChannels * BitsPerSample/8 */
	uint16_t BlockAlign;	/* = NumChannels * BitsPerSample/8 */	
	uint16_t BitsPerSample;	/* 8bits, 16bits, etc. */
 
	/* sub-chunk "data" */
	uint32_t Subchunk2ID;	/* "data" */
	uint32_t Subchunk2Size;	/* data size */
};

void duer_media_speak_play(const char *url);
void duer_media_speak_stop();

void duer_media_audio_start(const char *url, int offset);
void duer_media_audio_resume(const char *url, int offset);
void duer_media_audio_stop();
void duer_media_audio_pause();

void duer_media_player_rehabilitation();
int duer_media_audio_get_position();

void duer_media_volume_change(int volume);
void duer_media_set_volume(int volume);
int duer_media_get_volume();
void duer_media_set_mute(bool mute);
bool duer_media_get_mute();
#endif