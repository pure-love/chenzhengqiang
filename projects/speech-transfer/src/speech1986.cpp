/*
*@authos:chenzhengqiang
*@start date:2015/7/9
*/
#include"speech1986.h"
#include"signalling.h"
#include"rtp.h"
#include"aes.h"
#include"opus.h"

static const size_t SIGNALLING_BUF_SIZE=64;
static const size_t PERSONAL_SIGNALLING_TYPE = 13;
static const size_t OPUS_PAYLOAD_TYPE = 97;
static const size_t RTP_PACKET_SIZE = 65535;
static const size_t PCM_STREAM_BUF_SIZE = 65535;
static bool CAMERA_QUIT_NOW = false;


/*
*@args:
*@returns:NULL indicates create opus encoder failed
*@desc:get the specified opus encoder
*/
OpusEncoder * get_opus_encoder( int sample_rate )
{
    OPUS_ENC_INFO opus_enc_info;
    opus_enc_info.application = OPUS_APPLICATION_AUDIO;
    opus_enc_info.bandwidth = OPUS_AUTO;
    opus_enc_info.bitrate = sample_rate;
    opus_enc_info.channels = 1;
    opus_enc_info.packet_loss_percentage = 0;
    opus_enc_info.sample_rate = sample_rate;
    opus_enc_info.use_vbr = 0;
    opus_enc_info.use_vbr_constant = 1;
    opus_enc_info.lsb_depth = 16;
    OpusEncoder *opus_encoder = create_opus_encoder( opus_enc_info );
    return opus_encoder;
}


/*
*@args:
*@returns:NULL indicates create opus decoder failed
*@desc:get the specified opus decoder
*/
OpusDecoder * get_opus_decoder( int sample_rate )
{
    OPUS_DEC_INFO opus_dec_info;
    opus_dec_info.channels=1;
    opus_dec_info.sample_rate = sample_rate;
    opus_dec_info.use_vbr = 0;
    opus_dec_info.use_vbr_constant = 1;
    opus_dec_info.lsb_depth = 16;
    OpusDecoder *opus_decoder = create_opus_decoder(opus_dec_info);
    return opus_decoder;
}



/*
*@args:
*@returns:
*@desc:as the function name described
*/
bool obtain_sockaddr(const char *IP, int PORT,struct sockaddr_in & transfer_addr )
{
    memset(&transfer_addr, 0,sizeof(transfer_addr));
    transfer_addr.sin_family = AF_INET;
    int ret = inet_pton(AF_INET,IP, &transfer_addr.sin_addr);
    if( ret != 1 )
    return false;    
    transfer_addr.sin_port = htons(PORT);
    return true;
}



/*
*@args:
*@returns:
*@desc:generate the rtp header with the specified payload type
 13 indicates personal signalling,97 indicates opus audio
*/
bool generate_rtp_header(RTP_HEADER & rtp_header, int payload_type)
{
    if( payload_type != PERSONAL_SIGNALLING_TYPE && payload_type != OPUS_PAYLOAD_TYPE )
    return false;
    
    rtp_header.version=2;
    rtp_header.padding=0;
    rtp_header.extension=0;
    rtp_header.csrc_count=0;	
    rtp_header.marker=1;
    rtp_header.payload_type= payload_type;
    rtp_header.sequence_no= 0;
    rtp_header.ssrc = 0;
    rtp_header.timestamp=0;
    rtp_header.ssrc=0;
    return true;
}


/*
@desc:the SIGNALLING_BUF_SIZE must greater than 64
the channel's length must less equal than 24
the flags only support 0 and 1,0 indicates camera,1 indicates pc
the action only supports 0 and 1,0 indicates CREATE AND JOIN, 1 indicates CANCEL AND LEAVE
*/
bool send_personal_signalling(int sock_fd, struct sockaddr_in & transfer_addr,
                                                         const RTP_HEADER & rtp_header, 
                                                         const char * channel,int flags,int action)
{
    char signalling[SIGNALLING_BUF_SIZE];
    bool generate_ok = generate_signalling(signalling,SIGNALLING_BUF_SIZE,channel,flags,action);
    if( !generate_ok )
    return false;
    unsigned char encrypt_string[FIXED_AES_ENCRYPT_SIZE];
    generate_aes_encrypt_string(encrypt_string,signalling);
    uint8_t rtp_packet[RTP_HEADER_LEAST_SIZE+FIXED_AES_ENCRYPT_SIZE];	
    rtp_packet_encapsulate(rtp_packet,RTP_HEADER_LEAST_SIZE+FIXED_AES_ENCRYPT_SIZE,
		                        (uint8_t *)encrypt_string,FIXED_AES_ENCRYPT_SIZE,rtp_header);
    int ret = sendto( sock_fd, rtp_packet, RTP_HEADER_LEAST_SIZE+FIXED_AES_ENCRYPT_SIZE, 0,
              (struct sockaddr *)&transfer_addr, sizeof(transfer_addr));
    if( ret == -1 )
    return false;
    return true;    
}


/*
*@args:
*@returns:
*@desc:convert the pcm data to opus data and then send it to transfer server
  the stream_size must equal to MIN_FRAME_SAMP which defined in opus.h
*/
bool send_speech_message( int sock_fd, struct sockaddr_in & transfer_addr,const RTP_HEADER & rtp_header, 
                                                    OpusEncoder * opus_encoder,const opus_int16 *pcm_stream,
                                                    size_t stream_size )
{
        unsigned char opus_stream[MAX_PACKET];
        if( stream_size != MIN_FRAME_SAMP )
        return false;    
        opus_int32 encode_bytes = encode_pcm_stream(opus_encoder,pcm_stream,MIN_FRAME_SAMP,opus_stream,MAX_PACKET);
        
        if( encode_bytes == -1 )
        {
            return false;
        }
        
        uint8_t * rtp_packet = new uint8_t[RTP_HEADER_LEAST_SIZE+encode_bytes];
        if( rtp_packet == NULL )
        return false;
        
        rtp_packet_encapsulate(rtp_packet,RTP_HEADER_LEAST_SIZE+encode_bytes,opus_stream,encode_bytes,rtp_header);

        int ret = sendto(sock_fd, rtp_packet, RTP_HEADER_LEAST_SIZE+encode_bytes, 0,
                                      (struct sockaddr *)&transfer_addr, sizeof(transfer_addr));
        if( ret == -1 )
        return false;
        
        delete rtp_packet;
        rtp_packet = NULL;
        return true;
    
}



/*
*@args:
*@returns:return the encode bytes,-1 indicates error
*/
size_t get_pcm_stream(OpusDecoder *opus_decoder,opus_int16 *pcm_stream,
                                          size_t stream_size, uint8_t *rtp_packet, size_t packet_size )
{
    RTP_HEADER rtp_header;
    int ret = rtp_header_parse(rtp_packet,packet_size,rtp_header);
    if( ret == -1 )
    {
        return ret;	    
    }

    if( rtp_header.payload_type != OPUS_PAYLOAD_TYPE )
    return 0;
    
    opus_int32 decode_bytes = decode_opus_stream(opus_decoder,rtp_packet+rtp_header.offset,
                                                         packet_size-rtp_header.offset,pcm_stream,stream_size);
    return decode_bytes;
}


//as the fucntion name described
//you can use the pa_simple handler to play pcm stream
pa_simple * register_pulse_audio_handler()
{
     int error;
     pa_simple *pa_handler;
     if (!(pa_handler= pa_simple_new(NULL,"play pcm", PA_STREAM_PLAYBACK, NULL, "playback", &PA_SAMPLE_SPEC, NULL, NULL, &error))) 
    {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        return NULL;
    } 
     return pa_handler;
}


//delete the pulse audio handler
void delete_pulse_audio_handler( pa_simple *pa_handler )
{
    if( pa_handler != NULL )
    pa_simple_free( pa_handler);
}


/*
*@desc:play the stream with pulse audio handler
*/
bool play_pcm_stream(pa_simple *pulse_audio,const char *pcm_stream, size_t stream_size )
{
     int error;
     if (pa_simple_write(pulse_audio,pcm_stream,stream_size, &error) < 0) 
     {
         fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
         return false;
     }
     return true;
}


/*
@desc:startup the camera's loop
*/
void camera_run( const char *IP, int PORT, const char *channel )
{
    int sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if( sock_fd < 0 )
    {
        printf("socket failed:%s",strerror(errno));
        exit(EXIT_FAILURE);
    }   
    struct sockaddr_in transfer_addr;
    bool OK = obtain_sockaddr(IP,PORT,transfer_addr);
    if( !OK )
    {
        printf("obtain_sockaddr failed\n");
        exit(EXIT_FAILURE);
    }
    //firstly send the personal signalling to speech transfer server
    
    RTP_HEADER rtp_header;
    generate_rtp_header(rtp_header,PERSONAL_SIGNALLING_TYPE);
    OK=send_personal_signalling(sock_fd,transfer_addr,rtp_header,channel,CAMERA,CREATE);
    if( ! OK )
    {
        printf("send personal signalling to speech transfer server failed\n");
        exit(EXIT_FAILURE);
    }

    pa_simple * pa_handler = register_pulse_audio_handler();
    if( pa_handler == NULL )
    {
        printf("register pulse audio failed\n");
        exit(EXIT_FAILURE);
    }

    OpusDecoder * opus_decoder = get_opus_decoder(8000);
    if( opus_decoder == NULL )
    {
        printf("get opus decoder failed\n");
        exit(EXIT_FAILURE);
    }

    
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    size_t received_bytes = 0;
    opus_int32 decode_bytes = 0;
    uint8_t rtp_packet[RTP_PACKET_SIZE];
    opus_int16 pcm_stream[PCM_STREAM_BUF_SIZE];
    
    while( ! CAMERA_QUIT_NOW )
    {
        memset(rtp_packet,0,sizeof(rtp_packet));
        memset(pcm_stream,0,sizeof(pcm_stream));
        received_bytes = recvfrom(sock_fd,rtp_packet,RTP_PACKET_SIZE,0,(struct sockaddr *)&remote_addr,&addr_len);
        if( received_bytes <=0 )
        continue;
        decode_bytes = get_pcm_stream(opus_decoder,pcm_stream,sizeof(pcm_stream),rtp_packet,received_bytes);
        if( decode_bytes == 0 )
        {
            continue;    
        }
	else if( decode_bytes == -1 )
	{
	    printf("opus decode failed\n");
	    break;
	}	
        play_pcm_stream(pa_handler,(char *)pcm_stream,decode_bytes*sizeof(opus_int16));
    }
    send_personal_signalling(sock_fd,transfer_addr,rtp_header,channel,CAMERA,CANCEL);
    delete_pulse_audio_handler(pa_handler);
    delete_opus_decoder(opus_decoder);
}


void camera_quit()
{
    CAMERA_QUIT_NOW = true;
}


void imitate_client( const char *IP, int PORT, const char *channel,const char *pcm_file )
{
    int sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if( sock_fd < 0 )
    {
        printf("socket failed:%s",strerror(errno));
        exit(EXIT_FAILURE);
    }   
    struct sockaddr_in transfer_addr;
    bool OK = obtain_sockaddr(IP,PORT,transfer_addr);
    if( !OK )
    {
        printf("obtain_sockaddr failed\n");
        exit(EXIT_FAILURE);
    }
    //firstly send the personal signalling to speech transfer server
    srand((int)time(0));
    RTP_HEADER rtp_header;
    generate_rtp_header( rtp_header,PERSONAL_SIGNALLING_TYPE );
    OK = send_personal_signalling(sock_fd,transfer_addr,rtp_header,channel,PC,JOIN);
    if( !OK )
    {
        printf("send personal signalling to speech transfer server failed\n");
        exit(EXIT_FAILURE);
    }

    OpusEncoder * opus_encoder = get_opus_encoder(8000);
    if( opus_encoder == NULL )
    {
        printf("get opus encoder failed\n");
        exit(EXIT_FAILURE);
    }

    FILE * fpcm_handler = fopen(pcm_file,"rb"); 
    if( fpcm_handler == NULL )
    {
        printf("failed to open file %s\n",pcm_file);
        exit(EXIT_FAILURE);
    }

    size_t read_bytes = 0;
    
    //encode the file stream with the specified frame sample size
    rtp_header.payload_type=OPUS_PAYLOAD_TYPE;
    opus_int16 pcm_stream[MIN_FRAME_SAMP];
    size_t sequence_no = rand() % 65536;
    uint32_t timestamp = rand() % 4294967295;
    rtp_header.ssrc = (uint32_t)time(NULL); 
    while( ( read_bytes = fread(pcm_stream,sizeof(opus_int16),MIN_FRAME_SAMP,fpcm_handler)) > 0 )
    {
        rtp_header.sequence_no = sequence_no;
        rtp_header.timestamp = timestamp;
        send_speech_message(sock_fd,transfer_addr,rtp_header,opus_encoder,pcm_stream,MIN_FRAME_SAMP);
        sequence_no = (sequence_no+1) % 65535;
        timestamp = (timestamp+1) % 4294967295;
    }

    rtp_header.payload_type = PERSONAL_SIGNALLING_TYPE;
    send_personal_signalling(sock_fd,transfer_addr,rtp_header,channel,PC,LEAVE);
    delete_opus_encoder(opus_encoder);
    fclose(fpcm_handler);
}
