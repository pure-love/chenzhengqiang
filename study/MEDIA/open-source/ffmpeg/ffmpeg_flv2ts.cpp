/*
 * ffmpegTest.cpp
 *
 *  Created on: 2014年12月30日
 *      Author: wk
 */
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}
#define FORMAT "segment"
#define OUTPUT_FILE "3%03d.ts"
#define INPUT_FILE "3.flv"

int main(int argc, char *argv[])
{
#if 1
	AVPacket pkt;
	AVFormatContext *input_fmtctx = NULL;
	AVFormatContext *output_fmtctx = NULL;
	//AVCodecContext *enc_ctx = NULL;
	//AVCodecContext *dec_ctx = NULL;
	//AVCodec *encoder = NULL;
	AVBitStreamFilterContext * vbsf = NULL;
	int ret = 0;
	int i = 0;
	//int have_key = 0;
	static int first_pkt = 1;
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
#endif
	av_register_all();

	if (avformat_open_input(&input_fmtctx, input_file, NULL, NULL) < 0) {
		printf("av_log 1\n");
		av_log(NULL, AV_LOG_ERROR, "failed to open the file %s\n", input_file);
		return -ENOENT;
	}

	if (avformat_find_stream_info(input_fmtctx, 0) < 0) {
		printf("av_log 2\n");
		av_log(NULL, AV_LOG_ERROR,
				"Failed to retrieve input stream information\n");
		return -EINVAL;
	}

	av_dump_format(input_fmtctx, 0, input_file, 0);

	if (avformat_alloc_output_context2(&output_fmtctx, NULL, FORMAT,
	output_file) < 0) {
		printf("av_log 3\n");
		av_log(NULL, AV_LOG_ERROR,
				"Cannot open the file %s \n",
				output_file);
		return -ENOENT;
	}

	for (i = 0; i < (int)input_fmtctx->nb_streams; i++) {
		AVStream *in_stream = input_fmtctx->streams[i];
		AVStream *out_stream = avformat_new_stream(output_fmtctx,
				in_stream->codec->codec);
		if (out_stream < 0) {
			printf("av_log 4\n");
			av_log(NULL, AV_LOG_ERROR,
					"Alloc new Stream error\n");
			return -EINVAL;
		}

		avcodec_copy_context(output_fmtctx->streams[i]->codec,
				input_fmtctx->streams[i]->codec);

		out_stream->codec->codec_tag = 0;
		if (output_fmtctx->oformat->flags & AVFMT_GLOBALHEADER) {
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}
	}

	vbsf = av_bitstream_filter_init("h264_mp4toannexb");

	av_dump_format(output_fmtctx, 0, output_file, 1);


	av_opt_set(output_fmtctx->priv_data, "segment_list", "playlist.m3u8", 0);
	av_opt_set(output_fmtctx->priv_data, "segment_time", "10", 0);

	if ((ret = avformat_write_header(output_fmtctx, NULL)) < 0) {
		printf("av_log 6\n");
		av_log(NULL, AV_LOG_ERROR,
				"Cannot write the header for the file '%s' ret = %d\n",
				"fuck.ts", ret);
		return -ENOENT;
	}

	while (1) {
		ret = av_read_frame(input_fmtctx, &pkt);
		if (ret < 0) {
			printf("av_log 7\n");
			av_log(NULL, AV_LOG_ERROR,
					"read frame error %d \n", ret);
			break;
		}

		if (pkt.pts == AV_NOPTS_VALUE && first_pkt) {
			pkt.pts = pkt.dts;
			first_pkt = 0;
		}

		AVStream *in_stream = input_fmtctx->streams[pkt.stream_index];
		AVStream *out_stream = output_fmtctx->streams[pkt.stream_index];

		pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base,
				out_stream->time_base,
				(AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base,
				out_stream->time_base,
				(AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base,
				out_stream->time_base);
		pkt.pos = -1;

		if (pkt.stream_index == 0) {

			AVPacket fpkt = pkt;
			int a = av_bitstream_filter_filter(vbsf, out_stream->codec, NULL,
					&fpkt.data, &fpkt.size, pkt.data, pkt.size,
					pkt.flags & AV_PKT_FLAG_KEY);
			if(a<0)
			{
				;
			}
			pkt.data = fpkt.data;
			pkt.size = fpkt.size;

		}

		ret = av_write_frame(output_fmtctx, &pkt);
		if (ret < 0) {
			printf("av_log 8\n");
			av_log(NULL, AV_LOG_ERROR,
					"Muxing Error\n");
			break;
		}

		av_free_packet(&pkt);
	}

	av_write_trailer(output_fmtctx);
	avformat_close_input(&input_fmtctx);
	avio_close(output_fmtctx->pb);
	avformat_free_context(output_fmtctx);
	av_bitstream_filter_close(vbsf);
	vbsf = NULL;
	return 0;
}
