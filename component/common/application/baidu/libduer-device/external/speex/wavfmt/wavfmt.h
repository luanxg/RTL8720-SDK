#ifndef WAVFMT_H
#define WAVFMT_H

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    char               chunkID[4];
    long               chunkSize;
} ChunkHeader;


#define RiffID "RIFF"  /* chunk ID for RIFF Chunk */
#define RIFF_FORMAT  "WAVE" 

typedef struct {
    char               chunkID[4]; //'RIFF'
    long               chunkSize;
    char               RiffFmt[4];   //'WAVE' 
} RiffChunk;



#define WAVE_FORMAT_PCM             0X01
#define WAVE_CHANNEL_MONO         1
#define WAVE_CHANNEL_STEREO      2



#define OFFSET_FMT_CHK   sizeof(RiffChunk)
#define FormatID "fmt "    /* chunkID for Format Chunk. NOTE: There is a space at the end of this ID. */

typedef struct {
    char               chunkID[4];
    long               chunkSize;
    short              wFormatTag;
    unsigned short wChannels;
    unsigned long  dwSamplesPerSec;
    unsigned long  dwAvgBytesPerSec;
    unsigned short wBlockAlign;
    unsigned short wBitsPerSample;
    /* Note: there may be additional fields here, depending upon wFormatTag. */
    
} FormatChunk;


#define OFFSET_DATA_CHK   (OFFSET_FMT_CHK + sizeof(FormatChunk))
#define DataID "data"  /* chunk ID for data Chunk */

typedef struct {
    char               chunkID[4];
    long               chunkSize;

    unsigned char  waveformData[];
} DataChunk;


#define ListID "list"  /* chunk ID for list Chunk */

typedef struct {
    char               chunkID[4];      /* 'list' */
    long               chunkSize;   /* includes the Type ID below */
    char               typeID[];     /* 'adtl' */
} ListHeader;


#define WAVE_HEADER_SIZE (OFFSET_DATA_CHK + sizeof(DataChunk))


int wavfmt_remove_header(FILE *pFile);
int wavfmt_add_riff_chunk(FILE *pFile, long size);
int wavfmt_add_fmt_chunk(FILE *pFile, unsigned short wChannels, unsigned long  dwSamplesPerSec, unsigned long  dwAvgBytesPerSec, unsigned short wBitsPerSample);
int wavfmt_add_data_chunk_header(FILE *pFile, long size);


#ifdef __cplusplus
}
#endif


#endif
