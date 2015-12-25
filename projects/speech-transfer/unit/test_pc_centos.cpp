#include "common.h"
#include "rtp.h"
#include "aes.h"
#include "opus.h"

#define REMOTE_IP "192.168.1.11"
#define REMOTE_PORT 54321

#define LOCAL_IP "0.0.0.0"
#define LOCAL_PORT 12380
#define PCM_FILE "sample.pcm"
#define OPUS_FILE "sample.opus"
#define OPUS_DEC_FILE "sample98.pcm"



static const size_t CHANNELS = 1;
static const size_t LSB_DEPTH = 16;
static const size_t USE_VBR = 0;
static const size_t USE_VBR_CONSTANT = 1;

#define WAV_FILE "sample.wav"
#define PCM_FILE "sample.pcm"
#define OPUS_FILE "sample.opus"
#define PCM_DEC_FILE "sample98.pcm"
#define WAV_DEC_FILW "sample98.wav"


int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    const char* request="PC_JOIN_balabalabala_123456789";
    unsigned char encrypt_string[FIXED_AES_ENCRYPT_SIZE];
    RTP_HEADER rtp_header;
    rtp_header.version=2;
    rtp_header.padding=0;
    rtp_header.extension=0;
    rtp_header.csrc_count=0;	
    rtp_header.marker=1;
    rtp_header.payload_type=3;
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
    opus_enc_info.packet_loss_percentage = 10;
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

    OPUS_DEC_INFO opus_dec_info;
    opus_dec_info.channels=CHANNELS;
    opus_dec_info.sample_rate = SAMPLE_RATE;
    opus_dec_info.use_vbr = USE_VBR;
    opus_dec_info.use_vbr_constant = USE_VBR_CONSTANT;
    opus_dec_info.lsb_depth = LSB_DEPTH;
    
    OpusDecoder *opus_decoder = create_opus_decoder(opus_dec_info);

    if( opus_decoder == NULL )
    {
        printf("create_opus_decoder failed\n");
        return -1;
    }


    FILE * fpcm_handler = fopen(PCM_FILE,"rb");
    FILE * fopus_handler = fopen(OPUS_FILE,"wb");
    
    if( fpcm_handler == NULL || fopus_handler == NULL )
    {
        fclose(fpcm_handler);
        fclose(fopus_handler);
        return false;
    }

    size_t read_bytes = 0;
    opus_int32 encode_bytes = 0;
    unsigned char data[MAX_PACKET];
    opus_int16 *pcm_stream = new opus_int16[MIN_FRAME_SAMP *sizeof(opus_int16)];
    if( pcm_stream == NULL )
    return false;    
    //memset(pcm_stream,0,sizeof(pcm_stream));

    //encode the file stream with the specified frame sample size
    int times = 0;
    while( ( read_bytes = fread(pcm_stream,sizeof(opus_int16),MIN_FRAME_SAMP,fpcm_handler)) > 0 )
    {
        encode_bytes = encode_pcm_stream(opus_encoder,pcm_stream,MIN_FRAME_SAMP,data,MAX_PACKET);
        if( encode_bytes < 0 )
        {
            fclose(fpcm_handler);
            fclose(fopus_handler);
            return false;
        }
        ++times;
        printf("%d encode bytes %d\n",times,encode_bytes);
        rtp_packet = new uint8_t[RTP_HEADER_LEAST_SIZE+encode_bytes];
        rtp_header.payload_type= 0;
        rtp_packet_encapsulate(rtp_packet,RTP_HEADER_LEAST_SIZE+encode_bytes,data,encode_bytes,rtp_header);
        sendto(sock_fd, rtp_packet, RTP_HEADER_LEAST_SIZE+encode_bytes, 0,
                  (struct sockaddr *)&pin, sizeof(pin));
        delete rtp_packet;
        rtp_packet = NULL; 
        //memset(pcm_stream,0,sizeof(pcm_stream));
        fwrite(&data[0],sizeof(unsigned char),encode_bytes,fopus_handler);
        memset(data,0,sizeof(data));
    }
    
    delete [] pcm_stream;
    pcm_stream = NULL;
    fclose(fpcm_handler);
    fclose(fopus_handler); 

    
    
    if( !decode_opus_file(opus_decoder,OPUS_FILE,PCM_DEC_FILE))
    printf("decode %s failed\n",OPUS_FILE);
    return 0;
}

