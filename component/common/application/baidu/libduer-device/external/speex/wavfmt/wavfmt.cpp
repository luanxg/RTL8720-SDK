

#include<stdlib.h>	
#include<string.h>	
#include<stdio.h>

#include "wavfmt.h"


int wavfmt_remove_header(FILE *pFile)
{
    RiffChunk riffchk = {0};
    FormatChunk fmtchk = {0};
    ChunkHeader chkhdr = {0};
        
    if (!pFile){
        return -1;
    }

    //read RIFF chunk
    fread(&riffchk, sizeof(RiffChunk), 1, pFile);

    //read fmt chunk
    fread(&fmtchk, sizeof(FormatChunk), 1, pFile);

    //skip the remaining data of fmt chunk
    if (fmtchk.chunkSize > sizeof(FormatChunk) - 8){
        fseek(pFile, fmtchk.chunkSize - ( sizeof(FormatChunk) - 8), SEEK_CUR);
    }

    //to find data chunk
    do {
        fread(&chkhdr, sizeof(ChunkHeader), 1, pFile);
        if (feof(pFile)){
            return -1;
        }
        if (strncmp(chkhdr.chunkID, DataID, 4) == 0){
            break; 
        }
        fseek(pFile, chkhdr.chunkSize, SEEK_CUR);
    }while(1);

    return 0;
}


int wavfmt_add_riff_chunk(FILE *pFile, long size)
{
    RiffChunk riffchk = {0};
    size_t wrBytes  =0;
    
    if (!pFile){
        return -1;
    }

    strncpy(riffchk.chunkID, RiffID, 4);
    strncpy(riffchk.RiffFmt, RIFF_FORMAT, 4);
    riffchk.chunkSize = size;

    fseek(pFile, 0, SEEK_SET);
    wrBytes = fwrite(&riffchk, sizeof(RiffChunk), 1, pFile);
    if (wrBytes != sizeof(RiffChunk)){
        return -1;
    }

    return 0;
}


int wavfmt_add_fmt_chunk(FILE *pFile, unsigned short wChannels, unsigned long  dwSamplesPerSec, unsigned long  dwAvgBytesPerSec, unsigned short wBitsPerSample)
{
    FormatChunk fmtchk = {0};
    size_t wrBytes  =0;
    
    if (!pFile){
        return -1;
    }

    strncpy(fmtchk.chunkID, FormatID, 4);
    fmtchk.chunkSize = sizeof(FormatChunk) - 8;
    fmtchk.wFormatTag = WAVE_FORMAT_PCM;
    fmtchk.wChannels = wChannels;
    fmtchk.dwSamplesPerSec = dwSamplesPerSec;
    fmtchk.dwAvgBytesPerSec = dwSamplesPerSec * wChannels * wBitsPerSample / 8;
    fmtchk.wBlockAlign = wChannels * wBitsPerSample / 8;
    fmtchk.wBitsPerSample = wBitsPerSample;
    

    fseek(pFile, OFFSET_FMT_CHK, SEEK_SET);
    wrBytes = fwrite(&fmtchk, sizeof(FormatChunk), 1, pFile);
    if (wrBytes != sizeof(FormatChunk)){
        return -1;
    }

    return 0;
}


int wavfmt_add_data_chunk_header(FILE *pFile, long size)
{
    DataChunk datachk = {0};
    size_t wrBytes  =0;
    
    if (!pFile){
        return -1;
    }

    strncpy(datachk.chunkID, DataID, 4);
    datachk.chunkSize = size;

    fseek(pFile, OFFSET_DATA_CHK, SEEK_SET);
    wrBytes = fwrite(&datachk, sizeof(DataChunk), 1, pFile);
    if (wrBytes != sizeof(DataChunk)){
        return -1;
    }

    return 0;

}





