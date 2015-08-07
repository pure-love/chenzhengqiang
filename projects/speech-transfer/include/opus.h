/*
*@author:ChenZhengQiang
*@start date:2015/7/3
*@modified date:
*@version:1.0
*/
#ifndef _BACKER_OPUS_H_
#define _BACKER_OPUS_H
#include<opus/opus.h>
#include<sys/types.h>

static const size_t MAX_PACKET=1500;
static const size_t SAMPLE_RATE = 8000;
static const size_t BIT_RATE = SAMPLE_RATE;
static const size_t MAX_FRAME_SAMP=960;
static const size_t MIN_FRAME_SAMP=320;
static const size_t COMPLEXITY = 8;

static const size_t COMPRESSED_SIZE = MIN_FRAME_SAMP / COMPLEXITY;


struct OPUS_ENC_INFO
{
    opus_int32 sample_rate;//it must be one of 8000, 12000, 16000, 24000, or 48000.
    int channels;//it must be 1 or 2
    int application;// OPUS_APPLICATION_VOIP/OPUS_APPLICATION_AUDIO/OPUS_APPLICATION_RESTRICTED_LOWDELAY
    opus_int32 bitrate;
    opus_int32 bandwidth;
    opus_int32 complexity;
    opus_int32 use_vbr;
    opus_int32 use_vbr_constant;
    opus_int32 lsb_depth;
    opus_int32 packet_loss_percentage;
    
};

struct OPUS_DEC_INFO
{
    opus_int32 sample_rate;
    opus_int32 channels;
    opus_int32 bitrate;
    opus_int32 complexity;
    opus_int32 use_vbr;
    opus_int32 use_vbr_constant;
    opus_int32 lsb_depth;
};


OpusEncoder * create_opus_encoder(const OPUS_ENC_INFO & opus_enc_info);
OpusDecoder * create_opus_decoder( const OPUS_DEC_INFO & opus_dec_info );
void delete_opus_encoder( OpusEncoder * opus_encoder );
void delete_opus_decoder( OpusDecoder * opus_decoder );
opus_int32 encode_pcm_stream(OpusEncoder *opus_encoder, const opus_int16 *pcm_stream,
                                                int frame_size, unsigned char *data, opus_int32 max_data_bytes);
bool encode_pcm_file (OpusEncoder *opus_encoder, int channels, const char * pcm_file, const char * opus_file );
bool decode_opus_file(OpusDecoder *opus_decoder, const char * opus_file, const char *pcm_file );
opus_int32 decode_opus_stream( OpusDecoder *opus_decoder, unsigned char *data,size_t data_bytes,
                                      opus_int16 *pcm_stream,opus_int32 MAX_SIZE );
#endif
