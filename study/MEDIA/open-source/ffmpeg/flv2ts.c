/*
@author:chenzhengqiang
@filename:flv2ts.c
@start date:2016/1/11
@modified date:
@desc:convert flv file to ts files
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>



#define FORMAT "segment"
#define INPUT_FILE (argv[1])
#define OUTPUT_FILE (argv[2])
#define M3U8_FILE (argv[3])

void print_usage(int argc, char **argv)
{
	if ( argc != 5)
	{
		printf("Usage:%s <flv file> <ts file prefix> <m3u8 prefix> <duration>"\
			"\n\rExample:./flv2ts anger.flv fuck fuck\n",argv[0]);
		exit(EXIT_FAILURE);
	}	
}

int main(int argc, char * *argv)
{
	AVPacket avPacket;
	AVFormatContext *iFormatContext = NULL;
	AVFormatContext *oFormatContext = NULL;
	AVBitStreamFilterContext * avbsfContext = NULL;
	
	int ret = 0;
	int index = 0;

	//int have_key = 0;
	static int firstAVPacket = 1;
	char outputFile[30];
	char m3u8File[30];
	
	print_usage(argc,argv);
	snprintf(outputFile, sizeof(outputFile), "%s%%03d.ts", OUTPUT_FILE);
	snprintf(m3u8File, sizeof(m3u8File), "%s.m3u8", M3U8_FILE);
	const char *duration = argv[4];
	av_register_all();
	if (avformat_open_input(&iFormatContext, INPUT_FILE, NULL, NULL) < 0) 
	{
		av_log(NULL, AV_LOG_ERROR, "failed to open the file %s\n", INPUT_FILE);
		exit(EXIT_FAILURE);
	}

	if (avformat_find_stream_info(iFormatContext, 0) < 0) 
	{
		av_log(NULL,  AV_LOG_ERROR, "Failed to retrieve input stream information\n");
		exit(EXIT_FAILURE);
	}

	av_dump_format(iFormatContext, 0, INPUT_FILE, 0);
	if (avformat_alloc_output_context2(&oFormatContext, NULL, FORMAT, outputFile) < 0) 
	{
		av_log(NULL, AV_LOG_ERROR,"Cannot open the file %s \n",outputFile);
		exit(EXIT_FAILURE);
	}

	for (index = 0; index < (int)iFormatContext->nb_streams; index++ ) 
	{
		AVStream *IStream = iFormatContext->streams[index];
		AVStream *OStream = avformat_new_stream(oFormatContext, IStream->codec->codec);
		if ( OStream == 0 ) 
		{
			av_log(NULL, AV_LOG_ERROR,"Alloc new Stream Failed\n");
			exit(EXIT_FAILURE);
		}
        
		avcodec_copy_context(oFormatContext->streams[index]->codec,
						     iFormatContext->streams[index]->codec);

		OStream->codec->codec_tag = 0;
		if (oFormatContext->oformat->flags & AVFMT_GLOBALHEADER) 
		{
			OStream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}


	avbsfContext = av_bitstream_filter_init("h264_mp4toannexb");
	av_dump_format(oFormatContext, 0, outputFile, 1);
	av_opt_set(oFormatContext->priv_data, "segment_list", m3u8File, 0);
	av_opt_set(oFormatContext->priv_data, "segment_time", duration, 0);

	if ((ret = avformat_write_header(oFormatContext, NULL)) < 0) 
	{
		av_log(NULL, AV_LOG_ERROR,"Cannot write the header for the file '%s' ret = %d\n","fuck.ts", ret);
		exit(EXIT_FAILURE);
	}

	while (1) 
	{
		ret = av_read_frame(iFormatContext, &avPacket);
		if (ret < 0) 
		{
			break;
		}

		if (avPacket.pts == AV_NOPTS_VALUE && firstAVPacket) 
		{
			avPacket.pts = avPacket.dts;
			firstAVPacket = 0;
		}

		AVStream *IStream = iFormatContext->streams[avPacket.stream_index];
		AVStream *OStream = oFormatContext->streams[avPacket.stream_index];

		avPacket.pts = av_rescale_q_rnd(avPacket.pts, IStream->time_base,
				OStream->time_base,
				(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		avPacket.dts = av_rescale_q_rnd(avPacket.dts, IStream->time_base,
				OStream->time_base,
				(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		avPacket.duration = av_rescale_q(avPacket.duration, IStream->time_base,
				OStream->time_base);
		avPacket.pos = -1;

		if (avPacket.stream_index == 0) 
		{

			AVPacket favPacket = avPacket;
			int a = av_bitstream_filter_filter(avbsfContext, OStream->codec, NULL,
					&favPacket.data, &favPacket.size, avPacket.data, avPacket.size,
					avPacket.flags & AV_PKT_FLAG_KEY);
			if(a >= 0)
			{
				avPacket.data = favPacket.data;
				avPacket.size = favPacket.size;
			}
			else
			{
				break;
			}

		}

		ret = av_write_frame(oFormatContext, &avPacket);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "AV_WRITE_FRAME ERROR\n");
			break;
		}

		av_packet_unref(&avPacket);
	}

	av_write_trailer(oFormatContext);
	avformat_close_input(&iFormatContext);
	avio_close(oFormatContext->pb);
	avformat_free_context(oFormatContext);
	av_bitstream_filter_close(avbsfContext);
	avbsfContext = NULL;
	return 0;
}
