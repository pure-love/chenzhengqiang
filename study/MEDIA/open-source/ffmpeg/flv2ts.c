/*
@author:chenzhengqiang
@filename:flv2ts.c
@start date:2016/1/11
@modified date:
@desc:convert flv file to ts files
*/

#include <stdio.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>


#define FORMAT "segment"
#define OUTPUT_FILE (argv[2])
#define INPUT_FILE (argv[1])


int main(int argc, char *argv[])
{
	AVPacket avPacket;
	AVFormatContext *iFormatContext = NULL;
	AVFormatContext *oFormatContext = NULL;
	AVBitStreamFilterContext * vbsf = NULL;
	
	int ret = 0;
	int i = 0;
	//int have_key = 0;
	static int first_avPacket = 1;
	const char *input_file = NULL;
	const char *output_file = NULL;

	if (argc < 3)
	{
        input_file = INPUT_FILE;
        output_file = OUTPUT_FILE;
        printf("1:%s\n",input_file);
	}
	else
	{
        input_file = argv[1];
        output_file = argv[2];
        printf("2\n");
	}
	
	av_register_all();
	if (avformat_open_input(&iFormatContext, input_file, NULL, NULL) < 0) {
		printf("av_log 1\n");
		av_log(NULL, AV_LOG_ERROR, "failed to open the file %s\n", input_file);
		return -ENOENT;
	}

	if (avformat_find_stream_info(iFormatContext, 0) < 0) {
		printf("av_log 2\n");
		av_log(NULL, AV_LOG_ERROR,
				"Failed to retrieve input stream information\n");
		return -EINVAL;
	}

	av_dump_format(iFormatContext, 0, input_file, 0);

	if (avformat_alloc_output_context2(&oFormatContext, NULL, FORMAT,
	output_file) < 0) {
		printf("av_log 3\n");
		av_log(NULL, AV_LOG_ERROR,
				"Cannot open the file %s \n",
				output_file);
		return -ENOENT;
	}

	for (i = 0; i < (int)iFormatContext->nb_streams; i++ ) {
		AVStream *in_stream = iFormatContext->streams[i];
		AVStream *out_stream = avformat_new_stream(oFormatContext,
				in_stream->codec->codec);
		if ( out_stream < 0 ) {
			printf("av_log 4\n");
			av_log(NULL, AV_LOG_ERROR,
					"Alloc new Stream error\n");
			return -EINVAL;
		}
        
		avcodec_copy_context(oFormatContext->streams[i]->codec,
				iFormatContext->streams[i]->codec);

		out_stream->codec->codec_tag = 0;
		if (oFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	vbsf = av_bitstream_filter_init("h264_mp4toannexb");

	av_dump_format(oFormatContext, 0, output_file, 1);


	av_opt_set(oFormatContext->priv_data, "segment_list", "playlist.m3u8", 0);
	av_opt_set(oFormatContext->priv_data, "segment_time", "10", 0);

	if ((ret = avformat_write_header(oFormatContext, NULL)) < 0) {
		printf("av_log 6\n");
		av_log(NULL, AV_LOG_ERROR,
				"Cannot write the header for the file '%s' ret = %d\n",
				"fuck.ts", ret);
		return -ENOENT;
	}

	while (1) {
		ret = av_read_frame(iFormatContext, &avPacket);
		if (ret < 0) {
			printf("av_log 7\n");
			av_log(NULL, AV_LOG_ERROR,
					"read frame error %d \n", ret);
			break;
		}

		if (avPacket.pts == AV_NOPTS_VALUE && first_avPacket) {
			avPacket.pts = avPacket.dts;
			first_avPacket = 0;
		}

		AVStream *in_stream = iFormatContext->streams[avPacket.stream_index];
		AVStream *out_stream = oFormatContext->streams[avPacket.stream_index];

		avPacket.pts = av_rescale_q_rnd(avPacket.pts, in_stream->time_base,
				out_stream->time_base,
				(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		avPacket.dts = av_rescale_q_rnd(avPacket.dts, in_stream->time_base,
				out_stream->time_base,
				(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		avPacket.duration = av_rescale_q(avPacket.duration, in_stream->time_base,
				out_stream->time_base);
		avPacket.pos = -1;

		if (avPacket.stream_index == 0) {

			AVPacket favPacket = avPacket;
			int a = av_bitstream_filter_filter(vbsf, out_stream->codec, NULL,
					&favPacket.data, &favPacket.size, avPacket.data, avPacket.size,
					avPacket.flags & AV_PKT_FLAG_KEY);
			if(a<0)
			{
				;
			}
			avPacket.data = favPacket.data;
			avPacket.size = favPacket.size;

		}

		ret = av_write_frame(oFormatContext, &avPacket);
		if (ret < 0) {
			printf("av_log 8\n");
			av_log(NULL, AV_LOG_ERROR,
					"Muxing Error\n");
			break;
		}

		av_free_packet(&avPacket);
	}

	av_write_trailer(oFormatContext);
	avformat_close_input(&iFormatContext);
	avio_close(oFormatContext->pb);
	avformat_free_context(oFormatContext);
	av_bitstream_filter_close(vbsf);
	vbsf = NULL;
	return 0;
}
