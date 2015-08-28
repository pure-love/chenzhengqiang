/*
 * FLV DEMUX
 * Shaozhong.Liang@gmail.com
 * 10/6/2010
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>

#include "avcodec.h"
#include "avformat.h"
#include "demuxer.h"

typedef struct AVDemuxContext
{
	int videoStream,audioStream;
	AVPacket   packet;
	
	AVFormatContext *pFormatCtx;
	AVCodecContext *pVideoCtxDec;
	AVCodecContext *pAudioCtxDec;
	AVCodec *pVideoCodecDec;
	AVCodec *pAudioCodecDec;
} AVDemuxContext;

void* open_flvdemux(const char *filename)
{
	int i, videoStream=-1, audioStream=-1;

	AVFormatContext *pFormatCtx = NULL;
	AVCodecContext *pVideoCtxDec = NULL;
	AVCodecContext *pAudioCtxDec = NULL;
	AVCodec *pVideoCodecDec = NULL;
	AVCodec *pAudioCodecDec = NULL;
	
	AVDemuxContext *pDemuxCtx = NULL;
	
	av_register_all();

	if(av_open_input_file(&pFormatCtx, filename, NULL, 0, NULL)!=0)
		return NULL; // Couldn't open file

	// Retrieve stream information
	if(av_find_stream_info(pFormatCtx)<0)
	{
		printf("Couldn't find stream information\n");
		//return ; // Couldn't find stream information
	}

	// Find the first video stream
	for(i=0; i<pFormatCtx->nb_streams; i++)
	{
		if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) 
		{
			videoStream = i;
			pVideoCtxDec=pFormatCtx->streams[i]->codec;
		}else if(pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO) 
		{
			audioStream = i;
			pAudioCtxDec=pFormatCtx->streams[i]->codec;
		}
	}
	
	// Find the decoder for the video stream
	pVideoCodecDec=avcodec_find_decoder(pVideoCtxDec->codec_id);
	pAudioCodecDec=avcodec_find_decoder(pAudioCtxDec->codec_id);
	if(pVideoCodecDec == NULL || pAudioCodecDec == NULL)
		return NULL; // Codec not found

	// Open video codec
	if(avcodec_open(pVideoCtxDec, pVideoCodecDec)<0)
		return NULL; // Could not open codec
	
	// Open audio codec
	if(avcodec_open(pAudioCtxDec, pAudioCodecDec)<0)
		return NULL; // Could not open codec

	pDemuxCtx = (AVDemuxContext *)av_mallocz(sizeof(AVDemuxContext));
	pDemuxCtx->videoStream = videoStream;
	pDemuxCtx->audioStream = audioStream;
	pDemuxCtx->pFormatCtx = pFormatCtx;
	pDemuxCtx->pVideoCtxDec = pVideoCtxDec;
	pDemuxCtx->pAudioCtxDec = pAudioCtxDec;
	pDemuxCtx->pVideoCodecDec = pVideoCodecDec;
	pDemuxCtx->pAudioCodecDec = pAudioCodecDec;
	
	return (void*)pDemuxCtx;
}
		
int close_flvdemux(const void* pDemux_ptr)
{	
	AVDemuxContext *pDemuxCtx;
	
	if(pDemux_ptr != NULL)
	{
		pDemuxCtx = (AVDemuxContext *)pDemux_ptr;
		
		av_free_packet(&pDemuxCtx->packet);
		
		avcodec_close(pDemuxCtx->pVideoCtxDec);
		avcodec_close(pDemuxCtx->pAudioCtxDec);
		av_close_input_file(pDemuxCtx->pFormatCtx);
		
		av_free(pDemuxCtx);
	}	
	return 0;
}

int demux_read_frame(const void* pDemux_ptr, uint8_t *buff, uint32_t *data_type, uint64_t *timestamp)
{	
	int outDecodeLen = 0;
	AVDemuxContext *pDemuxCtx;
	
	if(pDemux_ptr == NULL || buff == NULL || data_type == NULL || timestamp == NULL)
	{
		return 0;	
	}
	pDemuxCtx = (AVDemuxContext *)pDemux_ptr;
	if(av_read_frame(pDemuxCtx->pFormatCtx, &pDemuxCtx->packet)>=0) 
	{
		//printf("date type=%d size=%-4d dts=%lld\n",pDemuxCtx->packet.stream_index, pDemuxCtx->packet.size, pDemuxCtx->packet.dts);
		
		if(pDemuxCtx->packet.size == 0)
			return 0;
		
		if(pDemuxCtx->packet.size >= 64*1024)
			pDemuxCtx->packet.size = 64*1024;
		if(pDemuxCtx->packet.stream_index == pDemuxCtx->videoStream) 
		{
			*data_type = AV_VIDEO_DATA;
			*timestamp = pDemuxCtx->packet.dts;
			avcodec_decode_video(pDemuxCtx->pVideoCtxDec, (AVFrame *)buff, &outDecodeLen, pDemuxCtx->packet.data, pDemuxCtx->packet.size);
		}else
		{
			*data_type = AV_AUDIO_DATA;
			*timestamp = pDemuxCtx->packet.dts;
			avcodec_decode_audio(pDemuxCtx->pAudioCtxDec, (int16_t *)buff, &outDecodeLen, pDemuxCtx->packet.data, pDemuxCtx->packet.size);
		}
	}
	return outDecodeLen;
}
	
