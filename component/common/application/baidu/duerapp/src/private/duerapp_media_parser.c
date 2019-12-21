#include "section_config.h"
#include "mp3_codec.h"
#include "duerapp_audio.h"
#include "duerapp_media.h"

SDRAM_DATA_SECTION static const unsigned short mp3_freq_tab[3] = { 44100, 48000, 32000 };
SDRAM_DATA_SECTION static const unsigned short mp3_bitrate_tab[2][15] = {
	{0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320 },
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
};


/* 11-bit syncword if MPEG 2.5 extensions are enabled */
#define	SYNCWORDH		0xff
#define	SYNCWORDL		0xe0

/* map to 0,1,2 to make table indexing easier */
typedef enum {
	MPEG1 =  0,
	MPEG2 =  1,
	MPEG25 = 2
} MPEGVersion;

/* map these to the corresponding 2-bit values in the frame header */
typedef enum {
	Stereo = 0x00,	/* two independent channels, but L and R frames might have different # of bits */
	Joint = 0x01,	/* coupled channels - layer III: mix of M-S and intensity, Layers I/II: intensity and direct coding only */
	Dual = 0x02,	/* two independent channels, L and R always have exactly 1/2 the total bitrate */
	Mono = 0x03		/* one channel */
} StereoMode;

static int MP3HeaderCheck(uint32_t header) {
	/* header */
	if ((header & 0xffe00000) != 0xffe00000)
		return -1;
	/* layer check */
	if ((header & (3 << 17)) != (1 << 17))
		return -1;
	/* bit rate */
	if ((header & (0xf << 12)) == 0xf << 12)
		return -1;
	/* frequency */
	if ((header & (3 << 10)) == 3 << 10)
		return -1;
	return 0;
}

int MP3FindSyncWord(unsigned char *buf, int nBytes)
{
	HTTP_LOGV("\r\nbuf 0x%x, 0x%x, 0x%x, 0x%x, addr %p\r\n", buf[0], buf[1], buf[2], buf[3], buf);
	uint32_t header;
	int offset = 0;
	int count = nBytes;
retry:

	if (count < 4) {
		HTTP_LOGD("http_mp3_find_frame_header, count %d size %d, buf 0x%x\r\n", count, nBytes, buf);
		return -1;
	}

	header = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	if (MP3HeaderCheck(header) < 0) {
		buf++;
		count--;
		offset++;
		goto retry;
	}
	
	HTTP_LOGD("\r\nhttp find frame header 0x%x, 0x%x, 0x%x, 0x%x, header 0x%x, offset %d\r\n", buf[0], buf[1], buf[2], buf[3], header, offset);
	return offset;
}

static int MP3VBRXingCheck(duer_media_info_t *mp3_info, u8* buf, int buf_size) {
	u8 *xing_header = NULL;
	u8* xing_data = NULL;

	u8 xing_flag = 0;
	u8 xing_has_total_frame = 0;
	u8 xing_has_total_size = 0;
	u8 xing_has_toc = 0;
	u8 xing_header_size = 0;

	if (buf_size < mp3_info->side_info_size + 4 + 8)
	{
		HTTP_LOGW("VBR header not enough, buf_size %d, side info size %d", buf_size, mp3_info->side_info_size);
		return -1;
	}

	xing_header = buf + mp3_info->side_info_size + 4;
	if (memcmp(xing_header, "Xing", 4) != 0 &&  memcmp(xing_header, "Info", 4) != 0)
	{
		HTTP_LOGW("Not find Xing Header, buf size %d", buf_size);
		return -1;
	}

	xing_flag = xing_header[7];
	xing_has_total_frame = xing_flag & (1 << 0);
	xing_has_total_size = xing_flag & (1 << 1);
	xing_has_toc = xing_flag & (1 << 2);
	xing_header_size = 4 * (xing_flag & (1 << 0)) + 4 * (xing_flag & (1 << 1)) + 100 * (xing_flag & (1 << 2));

	if (xing_header + 8 + xing_header_size > buf + buf_size) {
		HTTP_LOGE("Find Xing Header, but not full content");
		return -1;
	}
	xing_data = xing_header + 8;
	if (xing_has_total_frame) {
		mp3_info->xing_total_frame = ((u8)(xing_data[0]) << 24) | ((u8)(xing_data[1]) << 16) | ((u8)(xing_data[2]) << 8) | (u8)(xing_data[3]);
		xing_data += 4;
	}

	if (xing_has_total_size) {
		mp3_info->xing_total_size = ((u8)(xing_data[0]) << 24) | ((u8)(xing_data[1]) << 16) | ((u8)(xing_data[2]) << 8) | (u8)(xing_data[3]);
		xing_data += 4;
	}

	if (xing_has_toc) {
		memcpy(mp3_info->toc_table, xing_data, 100);
	}
	return 0;
}

static int MP3VBRVbriCheck(duer_media_info_t *mp3_info, u8* buf, int buf_size) {
	return -1;
}

static int MP3VBRCheck(duer_media_info_t *mp3_info, u8* buf, int buf_size) {
	int ret = 0;
	ret = MP3VBRXingCheck(mp3_info, buf, buf_size);
	if (ret != 0)
		ret = MP3VBRVbriCheck(mp3_info, buf, buf_size);

	return ret;
}

int MP3GetNextFrameInfo(player_t* mp3DecInfo, char *buf) {
	int lsf, mpeg25, bitrate, samprate;
	int verIdx, srIdx, brIdx, paddingBit, layer;
	MPEGVersion ver;
	StereoMode sMode;

	verIdx = 	 (buf[1] >> 3) & 0x03;
	layer = 4 - ((buf[1] >> 1) & 0x03);     /* easy mapping of index to layer number, 4 = error */
	brIdx =      (buf[2] >> 4) & 0x0f;
	srIdx = 	 (buf[2] >> 2) & 0x03;
	paddingBit = (buf[2] >> 1) & 0x01;
	sMode =      (StereoMode)((buf[3] >> 6) & 0x03);
	ver = (MPEGVersion)( verIdx == 0 ? MPEG25 : ((verIdx & 0x01) ? MPEG1 : MPEG2) );

	if (srIdx == 3 || layer == 4 || brIdx == 15)
		return -1;

	if (ver == MPEG25) {
		lsf = 1;
		mpeg25 = 1;
	} else {
		lsf = (ver == MPEG1) ? 0 : 1;
		mpeg25 = 0;
	}

	samprate = mp3_freq_tab[srIdx] >> (lsf + mpeg25);
	mp3DecInfo->nb_channels = (sMode == Mono ? 1 : 2);
	mp3DecInfo->sample_rate = samprate;

	if (brIdx) {
		bitrate = mp3_bitrate_tab[lsf][brIdx];
		mp3DecInfo->bit_rate = bitrate;
		mp3DecInfo->frame_size = (bitrate * 144000) / (samprate << lsf) + paddingBit;
	} else {
		/* if no frame size computed, signal it */
		HTTP_LOGE("MP3 get frame size failed %p", buf);
		return -1;
	}
	return 0;
}


int MP3GetFirstFrameInfo(duer_media_info_t *mp3_info, unsigned char *buf, int nBytes) {
	int ret;
	int lsf, mpeg25, bitrate, samprate;
	int layer, verIdx, srIdx, brIdx, paddingBit;
	MPEGVersion ver;
	StereoMode sMode;

	verIdx = 	 (buf[1] >> 3) & 0x03;
	layer = 4 - ((buf[1] >> 1) & 0x03);
	brIdx =      (buf[2] >> 4) & 0x0f;
	srIdx = 	 (buf[2] >> 2) & 0x03;
	paddingBit = (buf[2] >> 1) & 0x01;
	sMode =      (StereoMode)((buf[3] >> 6) & 0x03);
	ver = (MPEGVersion)( verIdx == 0 ? MPEG25 : ((verIdx & 0x01) ? MPEG1 : MPEG2) );

	if (ver == MPEG25) {
		lsf = 1;
		mpeg25 = 1;
	} else {
		lsf = (ver == MPEG1) ? 0 : 1;
		mpeg25 = 0;
	}

	samprate = mp3_freq_tab[srIdx] >> (lsf + mpeg25);
	mp3_info->nb_channels = (sMode == Mono ? 1 : 2);
	mp3_info->sample_rate = samprate;
	mp3_info->layer_ver = layer;
	mp3_info->mpeg_ver = ver;
	mp3_info->sample_num = (mp3_info->mpeg_ver == 1) ? 1152 : 576;

	if (brIdx) {
		bitrate = mp3_bitrate_tab[lsf][brIdx];
		mp3_info->bit_rate = bitrate;
		mp3_info->frame_size = (bitrate * 144000) / (samprate << lsf) + paddingBit;
	} else {
		/* if no frame size computed, signal it */
		HTTP_LOGE("MP3 get frame size failed %p", buf);
		return -1;
	}

	if (mp3_info->nb_channels == 2 && mp3_info->mpeg_ver == 1)
		mp3_info->side_info_size = 32;
	else if (mp3_info->nb_channels == 1 && mp3_info->mpeg_ver != 1)
		mp3_info->side_info_size = 9;
	else
		mp3_info->side_info_size = 17;
	
	if(mp3_info->range_len == 0)
		mp3_info->range_len = mp3_info->content_len;

	ret = MP3VBRCheck(mp3_info, buf, nBytes);
	if (ret == 0) {
		mp3_info->is_vbr = 1;
		mp3_info->duration = mp3_info->sample_rate ? mp3_info->xing_total_frame * mp3_info->sample_num / mp3_info->sample_rate : 0;
	} else {
		mp3_info->duration = mp3_info->bit_rate ? mp3_info->range_len * 8 / (mp3_info->bit_rate * 1000) : 0;
	}
	mp3_info->is_parsed = 1;

	HTTP_LOGD("MP3 get nb_channels %d, sample_rate %d, frame_size %d\r\n",
	          mp3_info->nb_channels, mp3_info->sample_rate, mp3_info->frame_size);

	return 0;
}


//######################### WAV PARSER #####################
int WAVGetMediaInfo(player_t *WAVPara, unsigned char *buffer, int nBytes){

	struct WAV_Format wav;
	int len = sizeof(struct WAV_Format);
	if(nBytes < len){
		DUER_LOGE("too short buffer");
		return -1;
	}

	memcpy(&wav, buffer , len);
	WAVPara->bit_rate = wav.ByteRate * 8/1000;
	WAVPara->frame_size = wav.BitsPerSample;
	WAVPara->nb_channels = wav.NumChannels;
	WAVPara->sample_rate = wav.SampleRate;
 
	DUER_LOGI("ChunkID \t%x", wav.ChunkID);
	DUER_LOGI("ChunkSize \t%d", wav.ChunkSize);
	DUER_LOGI("Format \t\t%x", wav.Format);
	DUER_LOGI("Subchunk1ID \t%x", wav.Subchunk1ID);
	DUER_LOGI("Subchunk1Size \t%d", wav.Subchunk1Size);
	DUER_LOGI("AudioFormat \t%d", wav.AudioFormat);
	DUER_LOGI("NumChannels \t%d", wav.NumChannels);
	DUER_LOGI("SampleRate \t%d", wav.SampleRate);
	DUER_LOGI("ByteRate \t%d", wav.ByteRate);
	DUER_LOGI("BlockAlign \t%d", wav.BlockAlign);
	DUER_LOGI("BitsPerSample \t%d", wav.BitsPerSample);
	DUER_LOGI("Subchunk2ID \t%x", wav.Subchunk2ID);
	DUER_LOGI("Subchunk2Size \t%d\n", wav.Subchunk2Size);

	return 0;
}
