#include "signalling.h"
#include "common.h"
#include "rtp.h"
#include "aes.h"
#include "opus.h"
#include <pulse/simple.h>
#include <pulse/error.h>


//used to receiving the speech transfer server's reply
//status 0 indicates ok,1 indicates failed
//start and end must equal to each other
//and if the value is 0x55aa then we know that it comes from the speech transfer server  
struct SERVER_REPLY
{
    int start;//0x55aa indicates sender is speech transfer server
    int status;//0 indicates ok,1 indicates failed
    int end;//also 0x55aa,it must equal to start
};


#define REMOTE_IP (argv[1])
#define REMOTE_PORT (argv[2])
#define CHANNEL "balabalabala"

static const SIGNALLING_BUF=64;
static const size_t BUFSIZE=640;
static const size_t CHANNELS = 1;
static const size_t LSB_DEPTH = 16;
static const size_t USE_VBR = 0;
static const size_t USE_VBR_CONSTANT = 1;
//global variable specify the personal singling type
static const int PERSONAL_SINGLING_TYPE=13;
static const int OPUS_RTP_PAYLOAD_TYPE = 97;


//the global pulse audio object "pa_sample_spec" ,used to initialzing pa_simple
static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 8000,
        .channels = 1
    };


// initialize the pulse audio playback handler with the low latency attrubte object
pa_simple * get_pa_playback_handler()
{
    pa_buffer_attr bufattr;
    bufattr.fragsize = (uint32_t)-1;
    bufattr.maxlength = pa_usec_to_bytes(20000,&ss);
    bufattr.minreq = pa_usec_to_bytes(0,&ss);
    bufattr.prebuf = (uint32_t)-1;
    bufattr.tlength = pa_usec_to_bytes(20000,&ss);
    pa_simple *s_play = NULL;
    int error;

     if (!(s_play = pa_simple_new(NULL, "CAMERA", PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, &bufattr, &error))) 
     {
         fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
         return NULL;
    }
    return s_play;
}


//init the rtp header's field with default value
//payload type 13 indicates personal signalling,97 indicates opus audio signalling
void init_default_rtp_header(RTP_HEADER & rtp_header,int payload_type )
{
    rtp_header.version=2;
    rtp_header.padding=0;
    rtp_header.extension=0;
    rtp_header.csrc_count=0;	
    rtp_header.marker=1;
    rtp_header.payload_type=payload_type;
    rtp_header.sequence_no=0;
    rtp_header.timestamp=0;
    rtp_header.ssrc=0;
}


//this function do something like the follwing
//1:generate the personal signalling with parameter of channel and rtp header
//2:generate the openssl encrypted string with the personal signalling
//3:encapsulate the rtp packet with the encrypted string
void generate_rtp_packet(uint8_t *rtp_packet,size_t packet_size,
                                              const char *channel,const RTP_HEADER & rtp_header)
{
    char personal_signalling[SIGNALLING_BUF];
    bool ok = generate_signalling(personal_signalling,SIGNALLING_BUF,channel,CAMERA,CREATE);
    if( !ok )
    {
        fprintf(stderr, __FILE__": generate_signalling() failed: %s\n", "the buffer size is too small or the channel's length is too long");
        exit(EXIT_FAILURE);
    }
    unsigned char encrypt_string[FIXED_AES_ENCRYPT_SIZE];
    generate_aes_encrypt_string(encrypt_string,personal_signalling);
    //print_aes_encrypt_string(encrypt_string,FIXED_AES_ENCRYPT_SIZE);
    rtp_packet_encapsulate(rtp_packet,packet_size,(uint8_t *)encrypt_string,sizeof(encrypt_string),rtp_header);
}


//as the function name described,try send the create request to
//speech transfer server in specified minutes
bool try_create_in_minutes(int minutes,int time_out,int sock_fd,const char * server_ip,
                                int server_port,const uint8_t *rtp_packet, size_t packet_size )
{
    struct sockaddr_in server_addr;
    socklen_t addr_len=sizeof(server_addr);
    memset(&server_addr, 0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    server_addr.sin_port = htons( server_port );
    struct SERVER_REPLY server_reply;
    server_reply.start=0;
    server_reply.status=-1;
    server_reply.end=0;
    //RTP_HEADER_LEAST_SIZE+FIXED_AES_ENCRYPT_SIZE
    struct timeval tv_timeout;
    tv_timeout.tv_sec = time_out;//waitting 10 minutes for once
    tv_timeout.tv_usec = 0;
    int times = (minutes *60) /time_out; 
    fd_set udp_readfds;
    int count=0;
    while( true )
    {
         if( count == times )
         {
             fprintf(stderr,"camera create channel failed in 1 minute\n");
             return false;
         }
         
         sendto(sock_fd, rtp_packet,packet_size, 0,
              (struct sockaddr *)&server_addr, sizeof(server_addr));

         FD_ZERO(&udp_readfds);
         FD_SET(sock_fd,&udp_readfds);
         int ret = select(sock_fd+1,&sock_fd,NULL,NULL,&tv_timeout);
         if( ret == -1 )
         {
              fprintf(stderr,"select() failed:%s",strerror(errno));
              return false;
         }
         
         if( FD_ISSET(sock_fd,&udp_readfds))
         {
                recvfrom(sock_fd,&server_reply,sizeof(server_reply),0,(struct sockaddr *)&server_addr,&addr_len);
                if( server_reply.start != 0x55aa || server_reply.status != 0 || server_reply.end != 0x55aa )
                {
                   ++count;
                   continue;
                }   
                break;
         }
         ++count;
    }
    return true;
}

OpusDecoder * get_opus_decoder()
{

    OPUS_DEC_INFO opus_dec_info;
    opus_dec_info.channels=CHANNELS;
    opus_dec_info.sample_rate = SAMPLE_RATE;
    opus_dec_info.use_vbr = USE_VBR;
    opus_dec_info.use_vbr_constant = USE_VBR_CONSTANT;
    opus_dec_info.lsb_depth = LSB_DEPTH;
   
    OpusDecoder *opus_decoder = create_opus_decoder(opus_dec_info);
    if( opus_decoder == NULL )
    {
       return NULL;
    }
    return opus_decoder;
}

int main( int argc, char ** argv )
{
    (void)argc;
    
    (void)argv;

    if( argc != 3 )
    {
        fprintf(stderr,"camera startup failed;usage:%s <server_ip> <server_port>",argv[0]);
        exit(EXIT_FAILURE);
    }
    RTP_HEADER rtp_header;
    init_default_rtp_header(rtp_header,PERSONAL_SINGLING_TYPE);
    uint8_t rtp_packet[RTP_HEADER_LEAST_SIZE+FIXED_AES_ENCRYPT_SIZE];
    generate_rtp_packet(rtp_packet,sizeof(rtp_packet),CHANNEL,rtp_header);
    int sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if( sock_fd < 0 )
    {
         fprintf(stderr,"socket() failed:%s",strerror(errno));
         exit(EXIT_FAILURE);   
    }
    bool ok = try_create_in_minutes(1,10,sock_fd,REMOTE_IP,REMOTE_PORT,rtp_packet,sizeof(rtp_packet));
    if( !ok )
    {
        fprintf(stderr,"try create channel in 1 minute failed,you might restart it or just terminate\n");
        exit(EXIT_FAILURE);
    }
    
    //now camera has create the channel succeeded,
    //so just play the pcm data receiving from speech transfer server
    size_t received_bytes;
    uint8_t rtp_packet_transfer[1024];
    OpusDecoder * opus_decoder = get_opus_decoder();

    if( opus_decoder == NULL )
    {
        fprintf(stderr,"get_opus_decoder() failed\n");
        exit(EXIT_FAILURE);
    }
    
    fprintf(stdout,"Camera create channel %s in chatroom succeeded\n"\
             "Just ready for playing pcm data receiving from speech transfer server\n\n",CHANNEL);
    
    struct sockaddr_in remote_addr;
    socklen_t addr_len = sizeof(remote_addr);
    int error;
    int count=0;
    int times = 0;
   while(true)
   {
        received_bytes = recvfrom(sock_fd,rtp_packet_transfer,1024,0,(struct sockaddr *)&remote_addr,&addr_len);
        //the rtp header hold 12 bytes and the encoded pcm data is 40 bytes
        //so 52
        if( received_bytes != 52)
        continue;    
        int ret = rtp_header_parse(rtp_packet_transfer,received_bytes,rtp_header);
        if( ret == -1 )
        {
             continue;
        }

        if( rtp_header.payload_type != 97 )
        {
             continue; 
        }
        
        ++times;
        if( times < 0 )
        times = 0;    
        opus_int16 buf[320]; 
        opus_int32 decode_bytes = decode_opus_stream(opus_decoder,rtp_packet_transfer+rtp_header.offset,
                                                                                 40,buf,320);
        fprintf(stdout,"[%d]decode_bytes:%d\n",times,(int)decode_bytes);
        if (pa_simple_write(s_play, (uint8_t *)buf, 640, &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
            ++count;
            if( count == 99 )
            exit(EXIT_FAILURE);    
            continue;
        }
   }
   return 0;
}
