/*
*@learner:chenzhengqiang
*@origin:net
*@start date:2016/3/1
*/

#include <stdio.h>
extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavfilter/avfiltergraph.h>
	#include <libavfilter/buffersink.h>
	#include <libavfilter/buffersrc.h>
	#include <libavutil/opt.h>
	#include <libavutil/pixdesc.h>
}


static AVFormatContext *iFormatContext;
static AVFormatContext *oFormatContext;
typedef struct FilteringContext 
{
	AVFilterContext *afSinkContext;
	AVFilterContext *afSrcContext;
	AVFilterGraph *afGraph;
} FilteringContext;
static FilteringContext *avFilterContext;


static int openInputFile(const char *filename)
{
	int ret;
	iFormatContext = NULL;

	//retrieve the media file's format information
	if ((ret = avformat_open_input(&iFormatContext, filename, NULL, NULL)) < 0) 
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
		return ret;
	}

	
	if ((ret = avformat_find_stream_info(iFormatContext, NULL)) < 0) 
	{
		av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
		return ret;
	}

	for (int index = 0; index < iFormatContext->nb_streams; ++index) 
	{
		AVStream *stream;
		AVCodecContext *avCodecContext;
		stream = iFormatContext->streams[index];
		avCodecContext = stream->codec;
		/* Reencode video & audio and remux subtitles etc. */
		if ( avCodecContext->codec_type == AVMEDIA_TYPE_VIDEO
			|| avCodecContext->codec_type == AVMEDIA_TYPE_AUDIO ) 
		{
			/* Open decoder */
			ret = avcodec_open2(avCodecContext, avcodec_find_decoder(avCodecContext->codec_id), NULL);
			if ( ret < 0 ) 
			{
				av_log(NULL,  AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", index);
				return ret;
			}
		}
	}

	av_dump_format(iFormatContext, 0, filename, 0);
	return 0;
}


static int openOutputFile(const char *filename)
{
	AVStream *out_stream;
	AVStream *in_stream;
	AVCodecContext *dec_ctx, *enc_ctx;
	AVCodec *encoder;
	int ret;
	unsigned int i;

	oFormatContext = NULL;
	avformat_alloc_output_context2(&oFormatContext, NULL, NULL, filename);
	if ( !oFormatContext ) 
	{
		av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
		return AVERROR_UNKNOWN;
	}

	for (i = 0; i < iFormatContext->nb_streams; i++) 
	{
		out_stream = avformat_new_stream(oFormatContext, NULL);
		if (!out_stream) 
		{
			av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
			return AVERROR_UNKNOWN;
		}

		in_stream = iFormatContext->streams[i];
		dec_ctx = in_stream->codec;
		enc_ctx = out_stream->codec;

		if ( dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO ) 
		{
			encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
			if ( encoder != 0 ) 
			{
				enc_ctx->height = dec_ctx->height;
				enc_ctx->width = dec_ctx->width;
				enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
				enc_ctx->pix_fmt = encoder->pix_fmts[0];
				enc_ctx->time_base = dec_ctx->time_base;
				enc_ctx->me_range = 16;
				enc_ctx->max_qdiff = 4;
				enc_ctx->qmin = 10;
				enc_ctx->qmax = 51;
				enc_ctx->qcompress = 0.6;
				enc_ctx->refs = 3;
				enc_ctx->bit_rate = 500000;
				
				ret = avcodec_open2(enc_ctx, encoder, NULL);
				if (ret < 0) 
				{
					av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
					return ret;
				}
			}
			else
			{
				av_log(NULL, AV_LOG_FATAL, "Neccessary encoder not found\n");
				return AVERROR_INVALIDDATA;
			}
		
		}
		else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) 
		{
			av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
			return AVERROR_INVALIDDATA;
		}
		else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
			enc_ctx->sample_rate = dec_ctx->sample_rate;
			enc_ctx->channel_layout = dec_ctx->channel_layout;
			enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
			enc_ctx->sample_fmt = encoder->sample_fmts[0];
			AVRational ar = { 1, enc_ctx->sample_rate };
			enc_ctx->time_base = ar;
			ret = avcodec_open2(enc_ctx, encoder, NULL);
			if ( ret < 0 ) 
			{
				av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
				return ret;
			}
		}
		else 
		{
			ret = avcodec_copy_context(oFormatContext->streams[i]->codec,
				iFormatContext->streams[i]->codec);
			if (ret < 0) 
			{
				av_log(NULL, AV_LOG_ERROR, "Copying stream context failed\n");
				return ret;
			}
		}

		if ( oFormatContext->oformat->flags & AVFMT_GLOBALHEADER )
		{
			enc_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
		}

	}
	
	av_dump_format(oFormatContext, 0, filename, 1);
	if (!(oFormatContext->oformat->flags & AVFMT_NOFILE)) 
	{
		ret = avio_open(&oFormatContext->pb, filename, AVIO_FLAG_WRITE);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
			return ret;
		}
	}

	/* init muxer, write output file header */
	ret = avformat_write_header(oFormatContext, NULL);
	if (ret < 0) 
	{
		av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
		return ret;
	}

	return 0;
}


static int init_filter(FilteringContext* fctx, AVCodecContext *dec_ctx,
	AVCodecContext *enc_ctx, const char *filter_spec)
{
	char args[512];
	int ret = 0;
	AVFilter *buffersrc = NULL;
	AVFilter *buffersink = NULL;
	AVFilterContext *afSrcContext = NULL;
	AVFilterContext *afSinkContext = NULL;
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs = avfilter_inout_alloc();
	AVFilterGraph *afGraph = avfilter_graph_alloc();

	if (!outputs || !inputs || !afGraph) 
	{
		ret = AVERROR(ENOMEM);
		goto end;
	}

	if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) 
	{
		buffersrc = avfilter_get_by_name("buffer");
		buffersink = avfilter_get_by_name("buffersink");
		if (!buffersrc || !buffersink) 
		{
			av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		snprintf(args, sizeof(args),
			"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
			dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
			dec_ctx->time_base.num, dec_ctx->time_base.den,
			dec_ctx->sample_aspect_ratio.num,
			dec_ctx->sample_aspect_ratio.den);

		ret = avfilter_graph_create_filter(&afSrcContext, buffersrc, "in",
			args, NULL, afGraph);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot create buffer source\n");
			goto end;
		}

		ret = avfilter_graph_create_filter(&afSinkContext, buffersink, "out",
			NULL, NULL, afGraph);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot create buffer sink\n");
			goto end;
		}

		ret = av_opt_set_bin(afSinkContext, "pix_fmts",
			(uint8_t*)&enc_ctx->pix_fmt, sizeof(enc_ctx->pix_fmt),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot set output pixel format\n");
			goto end;
		}
	}
	else if (dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) 
	{
		buffersrc = avfilter_get_by_name("abuffer");
		buffersink = avfilter_get_by_name("abuffersink");
		if (!buffersrc || !buffersink) 
		{
			av_log(NULL, AV_LOG_ERROR, "filtering source or sink element not found\n");
			ret = AVERROR_UNKNOWN;
			goto end;
		}

		if (!dec_ctx->channel_layout)
			dec_ctx->channel_layout =
			av_get_default_channel_layout(dec_ctx->channels);
		snprintf(args, sizeof(args),
			//"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%0x"PRIx64,
			"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%0x",
			dec_ctx->time_base.num, dec_ctx->time_base.den, dec_ctx->sample_rate,
			av_get_sample_fmt_name(dec_ctx->sample_fmt),
			dec_ctx->channel_layout);
		ret = avfilter_graph_create_filter(&afSrcContext, buffersrc, "in",
			args, NULL, afGraph);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
			goto end;
		}

		ret = avfilter_graph_create_filter(&afSinkContext, buffersink, "out",
			NULL, NULL, afGraph);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
			goto end;
		}

		ret = av_opt_set_bin(afSinkContext, "sample_fmts",
			(uint8_t*)&enc_ctx->sample_fmt, sizeof(enc_ctx->sample_fmt),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
			goto end;
		}

		ret = av_opt_set_bin(afSinkContext, "channel_layouts",
			(uint8_t*)&enc_ctx->channel_layout,
			sizeof(enc_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
			goto end;
		}

		ret = av_opt_set_bin(afSinkContext, "sample_rates",
			(uint8_t*)&enc_ctx->sample_rate, sizeof(enc_ctx->sample_rate),
			AV_OPT_SEARCH_CHILDREN);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
			goto end;
		}
	}
	else 
	{
		ret = AVERROR_UNKNOWN;
		goto end;
	}

	/* Endpoints for the filter graph. */
	outputs->name = av_strdup("in");
	outputs->filter_ctx = afSrcContext;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = afSinkContext;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	if (!outputs->name || !inputs->name) 
	{
		ret = AVERROR(ENOMEM);
		goto end;
	}

	if ((ret = avfilter_graph_parse_ptr(afGraph, filter_spec,
		&inputs, &outputs, NULL)) < 0)
		goto end;

	if ((ret = avfilter_graph_config(afGraph, NULL)) < 0)
		goto end;

	/* Fill FilteringContext */
	fctx->afSrcContext = afSrcContext;
	fctx->afSinkContext = afSinkContext;
	fctx->afGraph = afGraph;

end:
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);

	return ret;
}

static int init_filters(void)
{
	const char *filter_spec;
	unsigned int i;
	int ret;
	avFilterContext = (FilteringContext*)av_malloc_array(iFormatContext->nb_streams, sizeof(*avFilterContext));
	if (!avFilterContext)
		return AVERROR(ENOMEM);

	for (i = 0; i < iFormatContext->nb_streams; i++) 
	{
		avFilterContext[i].afSrcContext = NULL;
		avFilterContext[i].afSinkContext = NULL;
		avFilterContext[i].afGraph = NULL;
		if (!(iFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
			|| iFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO))
			continue;


		if (iFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			filter_spec = "null"; /* passthrough (dummy) filter for video */
		else
			filter_spec = "anull"; /* passthrough (dummy) filter for audio */
		ret = init_filter(&avFilterContext[i], iFormatContext->streams[i]->codec,
			oFormatContext->streams[i]->codec, filter_spec);
		if (ret)
			return ret;
	}
	return 0;
}

static int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index, int *got_frame) {
	int ret;
	int got_frame_local;
	AVPacket enc_pkt;
	int(*enc_func)(AVCodecContext *, AVPacket *, const AVFrame *, int *) =
		(iFormatContext->streams[stream_index]->codec->codec_type ==
		AVMEDIA_TYPE_VIDEO) ? avcodec_encode_video2 : avcodec_encode_audio2;

	if (!got_frame)
		got_frame = &got_frame_local;

	av_log(NULL, AV_LOG_INFO, "Encoding frame\n");
	/* encode filtered frame */
	enc_pkt.data = NULL;
	enc_pkt.size = 0;
	av_init_packet(&enc_pkt);
	ret = enc_func(oFormatContext->streams[stream_index]->codec, &enc_pkt,
		filt_frame, got_frame);
	av_frame_free(&filt_frame);
	if (ret < 0)
		return ret;
	if (!(*got_frame))
		return 0;

	/* prepare packet for muxing */
	enc_pkt.stream_index = stream_index;
	av_packet_rescale_ts(&enc_pkt,
		oFormatContext->streams[stream_index]->codec->time_base,
		oFormatContext->streams[stream_index]->time_base);

	av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
	/* mux encoded frame */
	ret = av_interleaved_write_frame(oFormatContext, &enc_pkt);
	return ret;
}


static int filter_encode_write_frame(AVFrame *frame, unsigned int stream_index)
{
	int ret;
	AVFrame *filt_frame;

	av_log(NULL, AV_LOG_INFO, "Pushing decoded frame to filters\n");
	/* push the decoded frame into the filtergraph */
	ret = av_buffersrc_add_frame_flags(avFilterContext[stream_index].afSrcContext,
		frame, 0);
	if (ret < 0) 
	{
		av_log(NULL, AV_LOG_ERROR, "Error while feeding the filtergraph\n");
		return ret;
	}

	/* pull filtered frames from the filtergraph */
	while (1) 
	{
		filt_frame = av_frame_alloc();
		if (!filt_frame) 
		{
			ret = AVERROR(ENOMEM);
			break;
		}
		av_log(NULL, AV_LOG_INFO, "Pulling filtered frame from filters\n");
		ret = av_buffersink_get_frame(avFilterContext[stream_index].afSinkContext,
			filt_frame);
		if (ret < 0) 
		{
			/* if no more frames for output - returns AVERROR(EAGAIN)
			* if flushed and no more frames for output - returns AVERROR_EOF
			* rewrite retcode to 0 to show it as normal procedure completion
			*/
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				ret = 0;
			av_frame_free(&filt_frame);
			break;
		}

		filt_frame->pict_type = AV_PICTURE_TYPE_NONE;
		ret = encode_write_frame(filt_frame, stream_index, NULL);
		if (ret < 0)
			break;
	}

	return ret;
}

static int flush_encoder(unsigned int stream_index)
{
	int ret;
	int got_frame;

	if (!(oFormatContext->streams[stream_index]->codec->codec->capabilities &
		CODEC_CAP_DELAY))
		return 0;

	while (1) 
	{
		av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);
		ret = encode_write_frame(NULL, stream_index, &got_frame);
		if (ret < 0)
			break;
		if (!got_frame)
			return 0;
	}
	return ret;
}


static void printUsage( char ** argv )
{
	printf("Usage:%s <video file> <video file>\n", argv[0]);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if ( argc != 3 )
	{
		printUsage(argv);
	}
	
	int ret;
	AVPacket packet; 
	packet.data = NULL;
	packet.size = 0;
	
	AVFrame *frame = NULL;
	enum AVMediaType type;
	unsigned int stream_index;
	unsigned int i;
	int got_frame;
	int(*dec_func)(AVCodecContext *, AVFrame *, int *, const AVPacket *);

	//first you must register all the codec,demux handler swiths av_register_all
	av_register_all();

	//and you need register the filter for av filter's sake
	avfilter_register_all();

	//and the first step you must retrieve the input file's format conext information
	//by calling the ffmpeg's api avformat_open_input
	//if you want transcode the input media file 
	if (( ret = openInputFile(argv[1]) ) < 0 )
		goto end;
	if (( ret = openOutputFile(argv[2]) ) < 0 )
		goto end;
	if (( ret = init_filters() ) < 0)
		goto end;


	/* read all packets */
	while (1) 
	{
		if ((ret = av_read_frame(iFormatContext, &packet)) < 0)
			break;
		
		stream_index = packet.stream_index;
		type = iFormatContext->streams[packet.stream_index]->codec->codec_type;
		
		av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n", stream_index);

		if (avFilterContext[stream_index].afGraph) 
		{
			av_log(NULL, AV_LOG_DEBUG, "Going to reencode&filter the frame\n");
			frame = av_frame_alloc();
			if (!frame) 
			{
				ret = AVERROR(ENOMEM);
				break;
			}
			av_packet_rescale_ts(&packet,
				iFormatContext->streams[stream_index]->time_base,
				iFormatContext->streams[stream_index]->codec->time_base);
			dec_func = (type == AVMEDIA_TYPE_VIDEO) ? avcodec_decode_video2 :
				avcodec_decode_audio4;
			ret = dec_func(iFormatContext->streams[stream_index]->codec, frame,
				&got_frame, &packet);
			if (ret < 0) 
			{
				av_frame_free(&frame);
				av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
				break;
			}

			if (got_frame) 
			{
				frame->pts = av_frame_get_best_effort_timestamp(frame);
				ret = filter_encode_write_frame(frame, stream_index);
				av_frame_free(&frame);
				if (ret < 0)
					goto end;
			}
			else 
			{
				av_frame_free(&frame);
			}
		}
		else 
		{
			/* remux this frame without reencoding */
			av_packet_rescale_ts(&packet,
				iFormatContext->streams[stream_index]->time_base,
				oFormatContext->streams[stream_index]->time_base);

			ret = av_interleaved_write_frame(oFormatContext, &packet);
			if (ret < 0)
				goto end;
		}
		av_packet_unref(&packet);
	}

	/* flush filters and encoders */
	for (i = 0; i < iFormatContext->nb_streams; i++) 
	{
		/* flush filter */
		if (!avFilterContext[i].afGraph)
			continue;
		ret = filter_encode_write_frame(NULL, i);
		if (ret < 0) {
			av_log(NULL, AV_LOG_ERROR, "Flushing filter failed\n");
			goto end;
		}

		/* flush encoder */
		ret = flush_encoder(i);
		if (ret < 0) 
		{
			av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
			goto end;
		}
	}

	av_write_trailer(oFormatContext);
end:
	av_packet_unref(&packet);
	av_frame_free(&frame);
	for (i = 0; i < iFormatContext->nb_streams; i++) 
	{
		avcodec_close(iFormatContext->streams[i]->codec);
		if (oFormatContext && oFormatContext->nb_streams > i && oFormatContext->streams[i] && oFormatContext->streams[i]->codec)
			avcodec_close(oFormatContext->streams[i]->codec);
		if (avFilterContext && avFilterContext[i].afGraph)
			avfilter_graph_free(&avFilterContext[i].afGraph);
	}
	
	av_free(avFilterContext);
	avformat_close_input(&iFormatContext);
	if (oFormatContext && !(oFormatContext->oformat->flags & AVFMT_NOFILE))
		avio_closep(&oFormatContext->pb);
	avformat_free_context(oFormatContext);

	if (ret < 0)
		av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n"); //av_err2str(ret));

	return ret ? 1 : 0;
}
