/*
*@author:ChenZhengQiang
*@start date:2015/7/3
*@modified date:
*@version:1.0
*/

#include "opus.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>


/*
*@args:opus_enc_info[in]
*@returns:void
*@desc:initialize the opus encoder with arugment opus_enc_info
*/
OpusEncoder * create_opus_encoder( const OPUS_ENC_INFO & opus_enc_info )
{
    int error;
    OpusEncoder *opus_encoder = opus_encoder_create(opus_enc_info.sample_rate,opus_enc_info.channels,
                                                                         opus_enc_info.application,&error);
    if( error != OPUS_OK || opus_encoder == NULL )
    return NULL;

    //set the encoder's bit rate,sample rate and so on
    opus_encoder_ctl(opus_encoder, OPUS_SET_BITRATE(opus_enc_info.bitrate));
    opus_encoder_ctl(opus_encoder, OPUS_SET_BANDWIDTH(opus_enc_info.bandwidth));
    opus_encoder_ctl(opus_encoder, OPUS_SET_VBR(opus_enc_info.use_vbr));
    opus_encoder_ctl(opus_encoder, OPUS_SET_VBR_CONSTRAINT(opus_enc_info.use_vbr_constant));
    opus_encoder_ctl(opus_encoder, OPUS_SET_COMPLEXITY(COMPLEXITY));
    opus_encoder_ctl(opus_encoder, OPUS_SET_PACKET_LOSS_PERC(opus_enc_info.packet_loss_percentage));
    opus_encoder_ctl(opus_encoder, OPUS_SET_LSB_DEPTH(opus_enc_info.lsb_depth));  

    return opus_encoder;
}



/*
*@args:opus_enc_info[in]
*@returns:void
*@desc:delete the opus encoder
*/
void delete_opus_encoder( OpusEncoder * opus_encoder )
{
    opus_encoder_destroy(opus_encoder);
}


/*
*@args:opus_dec_info[in]
*@returns:void
*@desc:initialize the opus decoder with arugment opus_dec_info
*/
OpusDecoder * create_opus_decoder( const OPUS_DEC_INFO & opus_dec_info )
{
    int error;
    OpusDecoder *opus_decoder = opus_decoder_create(opus_dec_info.sample_rate,opus_dec_info.channels,&error);
    if( error != OPUS_OK  || opus_decoder == NULL )
    return NULL;
    opus_decoder_ctl(opus_decoder, OPUS_SET_BITRATE(opus_dec_info.bitrate));
    opus_decoder_ctl(opus_decoder, OPUS_SET_VBR(opus_dec_info.use_vbr));
    opus_decoder_ctl(opus_decoder, OPUS_SET_VBR_CONSTRAINT(opus_dec_info.use_vbr_constant));
    opus_decoder_ctl(opus_decoder, OPUS_SET_COMPLEXITY(COMPLEXITY));
    opus_decoder_ctl(opus_decoder, OPUS_SET_LSB_DEPTH(opus_dec_info.lsb_depth));  
    return opus_decoder;
}


/*
*@args:opus_dec_info[in]
*@returns:void
*@desc:delete the opus decoder
*/
void delete_opus_decoder( OpusDecoder * opus_decoder )
{
    opus_decoder_destroy(opus_decoder);   
}


/*
*@args:
*@returns:void
*@desc:encode the pcm stream to opus stream
  you must specify the frame size,detailed information
  see the opus codec api document
*/
opus_int32 encode_pcm_stream(OpusEncoder *opus_encoder, const opus_int16 *pcm_stream,
                                                int frame_size, unsigned char *data, opus_int32 max_data_bytes)
{
    if( opus_encoder == NULL || pcm_stream == NULL || data == NULL )
    {
        return -1;
    }
    
    opus_int32 encode_len = opus_encode(opus_encoder,pcm_stream,frame_size,
                                                           data,max_data_bytes);
    
    if( encode_len < 0 || encode_len > max_data_bytes )
    {
        return -1;
    }
    return encode_len;
}



/*
*@args:
*@returns:void
*@desc:encode the pcm file as opus file with the specified channels
*/
bool encode_pcm_file (OpusEncoder *opus_encoder, int channels, const char * pcm_file, const char * opus_file)
{
    
    FILE * fpcm_handler = fopen(pcm_file,"rb");
    FILE * fopus_handler = fopen(opus_file,"wb");
    if( fpcm_handler == NULL || fopus_handler == NULL )
    {
        fclose(fpcm_handler);
        fclose(fopus_handler);
        return false;
    }

    size_t read_bytes = 0;
    opus_int32 encode_bytes = 0;
    unsigned char data[MAX_PACKET];
    opus_int16 *pcm_stream = new opus_int16[MIN_FRAME_SAMP*channels *sizeof(opus_int16)];
    if( pcm_stream == NULL )
    return false;    
    memset(pcm_stream,0,sizeof(pcm_stream));

    //encode the file stream with the specified frame sample size
    while( ( read_bytes = fread(pcm_stream,sizeof(opus_int16),MIN_FRAME_SAMP*channels,fpcm_handler)) > 0 )
    {
        encode_bytes = encode_pcm_stream(opus_encoder,pcm_stream,MIN_FRAME_SAMP*channels,data,MAX_PACKET);
        if( encode_bytes < 0 )
        {
            fclose(fpcm_handler);
            fclose(fopus_handler);
            return false;
        }
        fwrite(&data[0],sizeof(unsigned char),encode_bytes,fopus_handler);
        memset(data,0,sizeof(data));
    }
    
    delete [] pcm_stream;
    pcm_stream = NULL;
    fclose(fpcm_handler);
    fclose(fopus_handler);
    return true;
}


/*
*@args:
*@returns:void
*@desc:decode the opus file to pcm file
*/
bool decode_opus_file(OpusDecoder *opus_decoder, const char * opus_file, const char *pcm_file )
{
    opus_int16 pcm_stream[65535];
    unsigned char data[ MAX_FRAME_SAMP];
    FILE * fpcm_handler = fopen(pcm_file,"wb");
    FILE * fopus_handler = fopen(opus_file,"rb");
    if( fpcm_handler == NULL || fopus_handler == NULL )
    {
         return false;
    }
    
    size_t read_bytes = 0;
    opus_int32 decode_bytes = -1;
    memset(data,0,sizeof(data));

    //just read the max frame sample size from opus file
    //then decode it with the compressed size per frame
    while(( read_bytes = fread(data,sizeof(unsigned char),MAX_FRAME_SAMP,fopus_handler)) > 0)
    {
        decode_bytes=decode_opus_stream(opus_decoder,data,read_bytes,pcm_stream,sizeof(pcm_stream));
        if( decode_bytes == -1 )
        {
            fclose(fpcm_handler);
            fclose(fopus_handler);
            return false;
        }
        fwrite(pcm_stream,sizeof(opus_int16),decode_bytes,fpcm_handler);
        memset(data,0,sizeof(data));
        memset(pcm_stream,0,sizeof(pcm_stream));
    }

    
    fclose(fpcm_handler);
    fclose(fopus_handler);
    return true;
}


opus_int32 decode_opus_stream( OpusDecoder *opus_decoder, unsigned char *data,size_t data_bytes,
                                      opus_int16 *pcm_stream, opus_int32 MAX_SIZE )
{
     
    opus_int32 decode_bytes = 0;
    int channels = opus_packet_get_nb_channels(data);
    //printf("channels is %d\n",channels);
    if( channels == OPUS_INVALID_PACKET )
    {
        return -1;
    }
    int frame_size = opus_packet_get_nb_samples(data,data_bytes,SAMPLE_RATE);
    if( frame_size == OPUS_INVALID_PACKET )
    return -1;
    
    int frames = opus_packet_get_nb_frames(data,data_bytes);
    //printf("frames are %d\n",frames);
    if( frames == OPUS_INVALID_PACKET )
    return -1;
    
    int samples_per_frame = opus_packet_get_samples_per_frame(data,data_bytes);
    //printf("samples per frame is %d\n",samples_per_frame);
    if( samples_per_frame == OPUS_INVALID_PACKET )
    return -1;
    
    opus_int32 compressed_size = frame_size*channels /COMPLEXITY;
    if( compressed_size <=0 )
    return -1;
    
    size_t total_bytes = 0;
    for( size_t index=0; index < data_bytes /compressed_size; ++index )
    {
        decode_bytes = opus_decode(opus_decoder,data+compressed_size*index,compressed_size,
                                                    pcm_stream+total_bytes,frame_size,0);
        if( decode_bytes < 0 || decode_bytes > frame_size )
        return -1;    
        total_bytes+=decode_bytes;
        if( total_bytes > (size_t)MAX_SIZE )
        return -1;    
    }
    return total_bytes;
}


