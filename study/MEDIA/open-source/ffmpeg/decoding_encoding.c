/*
*@learner:chenzhengqiang
*@start date:2016/1/20
*@origin:ffmpeg official website http://ffmpeg.org/doxygen/trunk/examples.html
*/

#include <math.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096


/* check that a given sample format is supported by the encoder */
static int check_sample_fmt(AVCodec *avCodec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = avCodec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE) 
    {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}


/* just pick the highest supported samplerate */
static int select_sample_rate(AVCodec *avCodec)
{
    const int *p;
    int best_samplerate = 0;
    if (!avCodec->supported_samplerates)
        return 44100;
    p = avCodec->supported_samplerates;
    while (*p) {
        best_samplerate = FFMAX(*p, best_samplerate);
        p++;
    }
    return best_samplerate;
}
/* select layout with the highest channel count */
static int select_channel_layout(AVCodec *avCodec)
{
    const uint64_t *p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels   = 0;
    if (!avCodec->channel_layouts)
        return AV_CH_LAYOUT_STEREO;
    p = avCodec->channel_layouts;
    while (*p) {
        int nb_channels = av_get_channel_layout_nb_channels(*p);
        if (nb_channels > best_nb_channels) {
            best_ch_layout    = *p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return best_ch_layout;
}


/*
 * Audio encoding example
 */
static void audio_encode_example(const char *filename)
{
    AVCodec *avCodec;
    AVCodecContext *avCodecContext= NULL;
    AVFrame *frame;
    AVPacket pkt;
    int i, j, k, ret, got_output;
    int buffer_size;
    FILE *f;
    uint16_t *samples;
    float t, tincr;
    printf("Encode audio file %s\n", filename);
    /* find the MP2 encoder */
    avCodec = avcodec_find_encoder(AV_CODEC_ID_MP2);
    if (!avCodec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    avCodecContext = avcodec_alloc_context3(avCodec);
    if (!avCodecContext) {
        fprintf(stderr, "Could not allocate audio avCodec context\n");
        exit(1);
    }
    /* put sample parameters */
    avCodecContext->bit_rate = 64000;
    /* check that the encoder supports s16 pcm input */
    avCodecContext->sample_fmt = AV_SAMPLE_FMT_S16;
    if (!check_sample_fmt(avCodec, avCodecContext->sample_fmt)) {
        fprintf(stderr, "Encoder does not support sample format %s",
                av_get_sample_fmt_name(avCodecContext->sample_fmt));
        exit(1);
    }
    /* select other audio parameters supported by the encoder */
    avCodecContext->sample_rate    = select_sample_rate(avCodec);
    avCodecContext->channel_layout = select_channel_layout(avCodec);
    avCodecContext->channels       = av_get_channel_layout_nb_channels(avCodecContext->channel_layout);
    /* open it */
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        fprintf(stderr, "Could not open avCodec\n");
        exit(1);
    }
    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    /* frame containing input raw audio */
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }
    frame->nb_samples     = avCodecContext->frame_size;
    frame->format         = avCodecContext->sample_fmt;
    frame->channel_layout = avCodecContext->channel_layout;
    /* the avCodec gives us the frame size, in samples,
     * we calculate the size of the samples buffer in bytes */
    buffer_size = av_samples_get_buffer_size(NULL, avCodecContext->channels, avCodecContext->frame_size,
                                             avCodecContext->sample_fmt, 0);
    if (buffer_size < 0) {
        fprintf(stderr, "Could not get sample buffer size\n");
        exit(1);
    }
    samples = av_malloc(buffer_size);
    if (!samples) {
        fprintf(stderr, "Could not allocate %d bytes for samples buffer\n",
                buffer_size);
        exit(1);
    }
    /* setup the data pointers in the AVFrame */
    ret = avcodec_fill_audio_frame(frame, avCodecContext->channels, avCodecContext->sample_fmt,
                                   (const uint8_t*)samples, buffer_size, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not setup audio frame\n");
        exit(1);
    }
    /* encode a single tone sound */
    t = 0;
    tincr = 2 * M_PI * 440.0 / avCodecContext->sample_rate;
    for (i = 0; i < 200; i++) {
        av_init_packet(&pkt);
        pkt.data = NULL; // packet data will be allocated by the encoder
        pkt.size = 0;
        for (j = 0; j < avCodecContext->frame_size; j++) {
            samples[2*j] = (int)(sin(t) * 10000);
            for (k = 1; k < avCodecContext->channels; k++)
                samples[2*j + k] = samples[2*j];
            t += tincr;
        }
        /* encode the samples */
        ret = avcodec_encode_audio2(avCodecContext, &pkt, frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding audio frame\n");
            exit(1);
        }
        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, f);
            av_packet_unref(&pkt);
        }
    }
    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        ret = avcodec_encode_audio2(avCodecContext, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
        if (got_output) {
            fwrite(pkt.data, 1, pkt.size, f);
            av_packet_unref(&pkt);
        }
    }
    fclose(f);
    av_freep(&samples);
    av_frame_free(&frame);
    avcodec_close(avCodecContext);
    av_free(avCodecContext);
}


/*
 * Audio decoding.
 */
static void audio_decode_example(const char *outfilename, const char *filename)
{
    AVCodec *avCodec;
    AVCodecContext *avCodecContext= NULL;
    int len;
    FILE *f, *outfile;
    uint8_t inbuf[AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;
    AVFrame *decoded_frame = NULL;
    av_init_packet(&avpkt);
    printf("Decode audio file %s to %s\n", filename, outfilename);
    /* find the mpeg audio decoder */
    avCodec = avcodec_find_decoder(AV_CODEC_ID_MP2);
    if (!avCodec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    avCodecContext = avcodec_alloc_context3(avCodec);
    if (!avCodecContext) {
        fprintf(stderr, "Could not allocate audio avCodec context\n");
        exit(1);
    }
    /* open it */
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        fprintf(stderr, "Could not open avCodec\n");
        exit(1);
    }
    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    outfile = fopen(outfilename, "wb");
    if (!outfile) {
        av_free(avCodecContext);
        exit(1);
    }
    /* decode until eof */
    avpkt.data = inbuf;
    avpkt.size = fread(inbuf, 1, AUDIO_INBUF_SIZE, f);
    while (avpkt.size > 0) {
        int i, ch;
        int got_frame = 0;
        if (!decoded_frame) {
            if (!(decoded_frame = av_frame_alloc())) {
                fprintf(stderr, "Could not allocate audio frame\n");
                exit(1);
            }
        }
        len = avcodec_decode_audio4(avCodecContext, decoded_frame, &got_frame, &avpkt);
        if (len < 0) {
            fprintf(stderr, "Error while decoding\n");
            exit(1);
        }
        if (got_frame) {
            /* if a frame has been decoded, output it */
            int data_size = av_get_bytes_per_sample(avCodecContext->sample_fmt);
            if (data_size < 0) {
                /* This should not occur, checking just for paranoia */
                fprintf(stderr, "Failed to calculate data size\n");
                exit(1);
            }
            for (i=0; i<decoded_frame->nb_samples; i++)
                for (ch=0; ch<avCodecContext->channels; ch++)
                    fwrite(decoded_frame->data[ch] + data_size*i, 1, data_size, outfile);
        }
        avpkt.size -= len;
        avpkt.data += len;
        avpkt.dts =
        avpkt.pts = AV_NOPTS_VALUE;
        if (avpkt.size < AUDIO_REFILL_THRESH) {
            /* Refill the input buffer, to avoid trying to decode
             * incomplete frames. Instead of this, one could also use
             * a parser, or use a proper container format through
             * libavformat. */
            memmove(inbuf, avpkt.data, avpkt.size);
            avpkt.data = inbuf;
            len = fread(avpkt.data + avpkt.size, 1,
                        AUDIO_INBUF_SIZE - avpkt.size, f);
            if (len > 0)
                avpkt.size += len;
        }
    }
    fclose(outfile);
    fclose(f);
    avcodec_close(avCodecContext);
    av_free(avCodecContext);
    av_frame_free(&decoded_frame);
}



/*
 * Video encoding example
 */
static void video_encode_example(const char *filename, int codec_id)
{
    AVCodec *avCodec;
    AVCodecContext *avCodecContext= NULL;
    int i, ret, x, y, got_output;
    FILE *f;
    AVFrame *frame;
    AVPacket pkt;
    uint8_t endcode[] = { 0, 0, 1, 0xb7};
    printf("Encode video file %s\n", filename);
    /* find the mpeg1 video encoder */
    avCodec = avcodec_find_encoder(codec_id);
    if (!avCodec) 
    {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
	
    avCodecContext = avcodec_alloc_context3(avCodec);
    if (!avCodecContext)
    {
        fprintf(stderr, "Could not allocate video avCodec context\n");
        exit(1);
    }
	
    /* put sample parameters */
    avCodecContext->bit_rate = 400000;//40K 
    /* resolution must be a multiple of two */
    avCodecContext->width = 352;
    avCodecContext->height = 288;
    /* frames per second */
    avCodecContext->time_base = (AVRational){1,25};

    /* emit one intra frame every ten frames
     * check frame pict_type before passing frame
     * to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
     * then gop_size is ignored and the output of encoder
     * will always be I frame irrespective to gop_size
     */
     
    avCodecContext->gop_size = 10;
    avCodecContext->max_b_frames = 1;
    avCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    if (codec_id == AV_CODEC_ID_H264)
    av_opt_set(avCodecContext->priv_data, "preset", "slow", 0);
	
    /* open it */
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) 
    {
        fprintf(stderr, "Could not open avCodec\n");
        exit(1);
    }

    f = fopen(filename, "wb");
    if (!f) 
    {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
	
    frame = av_frame_alloc();
    if (!frame) 
   {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
	
    frame->format = avCodecContext->pix_fmt;
    frame->width  = avCodecContext->width;
    frame->height = avCodecContext->height;

	
    /* the image can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, avCodecContext->width, avCodecContext->height,
                         avCodecContext->pix_fmt, 32);
    if (ret < 0) 
    {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }

	
    /* encode 1 second of video */
    for (i = 0; i < 25; i++) {
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;
        fflush(stdout);
		
        /* prepare a dummy image */
        /* Y */
        for (y = 0; y < avCodecContext->height; y++) {
            for (x = 0; x < avCodecContext->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }
		
        /* Cb and Cr */
        for (y = 0; y < avCodecContext->height/2; y++) {
            for (x = 0; x < avCodecContext->width/2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }
		
        frame->pts = i;
        /* encode the image */
        ret = avcodec_encode_video2(avCodecContext, &pkt, frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, f);
            av_packet_unref(&pkt);
        }
    }
	
    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        fflush(stdout);
        ret = avcodec_encode_video2(avCodecContext, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            fwrite(pkt.data, 1, pkt.size, f);
            av_packet_unref(&pkt);
        }
    }
    /* add sequence end code to have a real mpeg file */
    fwrite(endcode, 1, sizeof(endcode), f);
    fclose(f);
    avcodec_close(avCodecContext);
    av_free(avCodecContext);
    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    printf("\n");
}
/*
 * Video decoding example
 */
static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
    FILE *f;
    int i;
    f = fopen(filename,"w");
    fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (i = 0; i < ysize; i++)
        fwrite(buf + i * wrap, 1, xsize, f);
    fclose(f);
}
static int decode_write_frame(const char *outfilename, AVCodecContext *avctx,
                              AVFrame *frame, int *frame_count, AVPacket *pkt, int last)
{
    int len, got_frame;
    char buf[1024];
    len = avcodec_decode_video2(avctx, frame, &got_frame, pkt);
    if (len < 0) {
        fprintf(stderr, "Error while decoding frame %d\n", *frame_count);
        return len;
    }
    if (got_frame) {
        printf("Saving %sframe %3d\n", last ? "last " : "", *frame_count);
        fflush(stdout);
        /* the picture is allocated by the decoder, no need to free it */
        snprintf(buf, sizeof(buf), outfilename, *frame_count);
        pgm_save(frame->data[0], frame->linesize[0],
                 frame->width, frame->height, buf);
        (*frame_count)++;
    }
    if (pkt->data) {
        pkt->size -= len;
        pkt->data += len;
    }
    return 0;
}
static void video_decode_example(const char *outfilename, const char *filename)
{
    AVCodec *avCodec;
    AVCodecContext *avCodecContext= NULL;
    int frame_count;
    FILE *f;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;
    av_init_packet(&avpkt);
    /* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
    memset(inbuf + INBUF_SIZE, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    printf("Decode video file %s to %s\n", filename, outfilename);
    /* find the mpeg1 video decoder */
    avCodec = avcodec_find_decoder(AV_CODEC_ID_MPEG1VIDEO);
    if (!avCodec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    avCodecContext = avcodec_alloc_context3(avCodec);
    if (!avCodecContext) {
        fprintf(stderr, "Could not allocate video avCodec context\n");
        exit(1);
    }
    if (avCodec->capabilities & AV_CODEC_CAP_TRUNCATED)
        avCodecContext->flags |= AV_CODEC_FLAG_TRUNCATED; // we do not send complete frames
    /* For some codecs, such as msmpeg4 and mpeg4, width and height
       MUST be initialized there because this information is not
       available in the bitstream. */
    /* open it */
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        fprintf(stderr, "Could not open avCodec\n");
        exit(1);
    }
    f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame_count = 0;
    for (;;) {
        avpkt.size = fread(inbuf, 1, INBUF_SIZE, f);
        if (avpkt.size == 0)
            break;
        /* NOTE1: some codecs are stream based (mpegvideo, mpegaudio)
           and this is the only method to use them because you cannot
           know the compressed data size before analysing it.
           BUT some other codecs (msmpeg4, mpeg4) are inherently frame
           based, so you must call them with all the data for one
           frame exactly. You must also initialize 'width' and
           'height' before initializing them. */
        /* NOTE2: some codecs allow the raw parameters (frame size,
           sample rate) to be changed at any frame. We handle this, so
           you should also take care of it */
        /* here, we use a stream based decoder (mpeg1video), so we
           feed decoder and see if it could decode a frame */
        avpkt.data = inbuf;
        while (avpkt.size > 0)
            if (decode_write_frame(outfilename, avCodecContext, frame, &frame_count, &avpkt, 0) < 0)
                exit(1);
    }
    /* some codecs, such as MPEG, transmit the I and P frame with a
       latency of one frame. You must do the following to have a
       chance to get the last frame of the video */
    avpkt.data = NULL;
    avpkt.size = 0;
    decode_write_frame(outfilename, avCodecContext, frame, &frame_count, &avpkt, 1);
    fclose(f);
    avcodec_close(avCodecContext);
    av_free(avCodecContext);
    av_frame_free(&frame);
    printf("\n");
}


int main(int argc, char **argv)
{
    const char *output_type;
    /* register all the codecs */
    avcodec_register_all();
    if (argc < 2) 
    {
        printf("usage: %s output_type\n"
               "API example program to decode/encode a media stream with libavcodec.\n"
               "This program generates a synthetic stream and encodes it to a file\n"
               "named test.h264, test.mp2 or test.mpg depending on output_type.\n"
               "The encoded stream is then decoded and written to a raw data output.\n"
               "output_type must be chosen between 'h264', 'mp2', 'mpg'.\n",
               argv[0]);
        return 1;
    }
	
    output_type = argv[1];
    if (!strcmp(output_type, "h264")) {
        video_encode_example("test.h264", AV_CODEC_ID_H264);
    } else if (!strcmp(output_type, "mp2")) {
        audio_encode_example("test.mp2");
        audio_decode_example("test.pcm", "test.mp2");
    } else if (!strcmp(output_type, "mpg")) {
        video_encode_example("test.mpg", AV_CODEC_ID_MPEG1VIDEO);
        video_decode_example("test%02d.pgm", "test.mpg");
    } else {
        fprintf(stderr, "Invalid output type '%s', choose between 'h264', 'mp2', or 'mpg'\n",
                output_type);
        return 1;
    }
    return 0;
}
