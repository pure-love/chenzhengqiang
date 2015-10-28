#include "common.h"
#include "rtp.h"
#include "aes.h"
#include "opus.h"
#include <pulse/simple.h>
#include <pulse/error.h>


#define REMOTE_IP "192.168.1.211"
#define REMOTE_PORT 54321

#define BUFSIZE 640
static const size_t CHANNELS = 1;
static const size_t LSB_DEPTH = 16;
static const size_t USE_VBR = 0;
static const size_t USE_VBR_CONSTANT = 1;

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const char* request="PC_JOIN_balabalabala_123456789";
    unsigned char encrypt_string[FIXED_AES_ENCRYPT_SIZE];

    static const pa_sample_spec ss = {
        .format = PA_SAMPLE_S16LE,
        .rate = 8000,
        .channels = 1
    };


    pa_simple *s_record = NULL;
    int error;
    /* Create the recording stream */
    if (!(s_record= pa_simple_new(NULL, "record", PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
        fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
        return -1;
    }
    RTP_HEADER rtp_header;
    rtp_header.version=2;
    rtp_header.padding=0;
    rtp_header.extension=0;
    rtp_header.csrc_count=0;	
    rtp_header.marker=1;
    rtp_header.payload_type=13;
    rtp_header.sequence_no=0;
    rtp_header.timestamp=0;
    rtp_header.ssrc=0;
    
    generate_aes_encrypt_string(encrypt_string,request);
    print_aes_encrypt_string(encrypt_string,FIXED_AES_ENCRYPT_SIZE);
    uint8_t *rtp_packet = new uint8_t[RTP_HEADER_LEAST_SIZE+FIXED_AES_ENCRYPT_SIZE];	
    rtp_packet_encapsulate(rtp_packet,RTP_HEADER_LEAST_SIZE+FIXED_AES_ENCRYPT_SIZE,
		                        (uint8_t *)encrypt_string,FIXED_AES_ENCRYPT_SIZE,rtp_header);
    
    int sock_fd = socket(AF_INET,SOCK_DGRAM,0);
    if( sock_fd < 0 )
    {
         printf("%s",strerror(errno));
         exit(EXIT_FAILURE);   
    }
    struct sockaddr_in pin;
    memset(&pin, 0,sizeof(pin));
    pin.sin_family = AF_INET;
    inet_pton(AF_INET, REMOTE_IP, &pin.sin_addr);
    pin.sin_port = htons(REMOTE_PORT);
    sendto(sock_fd, rtp_packet, RTP_HEADER_LEAST_SIZE+FIXED_AES_ENCRYPT_SIZE, 0,
              (struct sockaddr *)&pin, sizeof(pin));
    delete rtp_packet;
    rtp_packet = NULL;

    OPUS_ENC_INFO opus_enc_info;
    opus_enc_info.application = OPUS_APPLICATION_VOIP;
    opus_enc_info.bandwidth = OPUS_AUTO;
    opus_enc_info.bitrate = BIT_RATE;
    opus_enc_info.channels = CHANNELS;
    opus_enc_info.packet_loss_percentage = 0;
    opus_enc_info.sample_rate = SAMPLE_RATE;
    opus_enc_info.use_vbr = USE_VBR;
    opus_enc_info.use_vbr_constant = USE_VBR_CONSTANT;
    opus_enc_info.lsb_depth = LSB_DEPTH;
    
    OpusEncoder *opus_encoder = create_opus_encoder( opus_enc_info );
    if( opus_encoder == NULL )
    {
        printf("create_opus_encoder failed\n");
        return -1;
    }

    opus_int32 encode_bytes = 0;
    unsigned char data[MAX_PACKET];
    //memset(pcm_stream,0,sizeof(pcm_stream));

    //encode the file stream with the specified frame sample size
    
    while(true)
    {
        uint8_t pcm_stream[BUFSIZE];
        /* Record some data ... */
        if (pa_simple_read(s_record, pcm_stream, sizeof(pcm_stream), &error) < 0) {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
            return -1;
        }
        encode_bytes = encode_pcm_stream(opus_encoder,(opus_int16*)pcm_stream,MIN_FRAME_SAMP,data,MAX_PACKET);
        if( encode_bytes < 0 )
        {
            fprintf(stderr, __FILE__": encode_pcm_stream failed");
            return -1;
        }
        rtp_packet = new uint8_t[RTP_HEADER_LEAST_SIZE+encode_bytes];
        rtp_header.payload_type= 97;
        rtp_packet_encapsulate(rtp_packet,RTP_HEADER_LEAST_SIZE+encode_bytes,data,encode_bytes,rtp_header);
        sendto(sock_fd, rtp_packet, RTP_HEADER_LEAST_SIZE+encode_bytes, 0,
                  (struct sockaddr *)&pin, sizeof(pin));
        delete rtp_packet;
        rtp_packet = NULL;
        //memset(pcm_stream,0,sizeof(pcm_stream));
        //memset(data,0,sizeof(data));
    }   
    return 0;
}
