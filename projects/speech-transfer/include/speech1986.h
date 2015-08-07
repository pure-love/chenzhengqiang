#ifndef _BACKER_SPEECH1986_H_
#define _BACKER_SPEECH1986_H_
#include"common.h"
#include"rtp.h"
#include<opus/opus.h>
#include<sys/socket.h>
#include<sys/types.h>
#include <pulse/simple.h>
#include <pulse/error.h>

static const pa_sample_spec PA_SAMPLE_SPEC = {
                 .format = PA_SAMPLE_S16LE,
                 .rate = 8000,
                 .channels = 1
};


OpusEncoder * get_opus_encoder( int sample_rate );
OpusDecoder * get_opus_decoder( int sample_rate );
bool obtain_sockaddr(const char *IP, int PORT,struct sockaddr_in & transfer_addr );
bool generate_rtp_header(RTP_HEADER & rtp_header, int payload_type );
bool send_personal_signalling(int sock_fd, struct sockaddr_in & transfer_addr,
                                                         const RTP_HEADER & rtp_header, 
                                                         const char * channel,int flags,int action);
bool send_speech_message( int sock_fd, struct sockaddr_in & transfer_addr,const RTP_HEADER & rtp_header, 
                                                    OpusEncoder * opus_encoder,const opus_int16 *pcm_stream,
                                                    size_t stream_size );
size_t get_pcm_stream(OpusDecoder *opus_decoder,opus_int16 *pcm_stream,
                                          size_t stream_size, uint8_t *rtp_packet,size_t packet_size );
pa_simple * register_pulse_audio_handler();
void delete_pulse_audio_handler( pa_simple *pa_handler );
bool play_pcm_stream(pa_simple *pulse_audio,const char *pcm_stream, size_t stream_size );
void camera_run( const char *IP, int PORT, const char *channel );
void camera_quit();
void imitate_client( const char *IP, int PORT, const char *channel,const char *pcm_file );
#endif
