/*
 * MPEG Audio decoder
 * Copyright (c) 2001, 2002 Fabrice Bellard.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file aacdec.c
 * AAC Audio decoder.
 */

#include "bitstream.h"
#include "internal.h"
#include "avformat.h"
#include "avio.h"

#define ADTS_HEADER_SIZE 7

typedef struct {
    int write_adts;
    int objecttype;
    int sample_rate_index;
    int channel_conf;
} ADTSContext;


static int decode_extradata(AVCodecContext *avctx, ADTSContext *adts, uint8_t *buf, int size)
{
    GetBitContext gb;
	
    init_get_bits(&gb, buf, size * 8);
    adts->objecttype = get_bits(&gb, 5) - 1;
    adts->sample_rate_index = get_bits(&gb, 4);
    adts->channel_conf = get_bits(&gb, 4);
	
    if (adts->objecttype > 3) {
        av_log(NULL, AV_LOG_ERROR, "MPEG-4 AOT %d is not allowed in ADTS\n", adts->objecttype+1);
        return -1;
    }
    if (adts->sample_rate_index == 15) {
        av_log(NULL, AV_LOG_ERROR, "Escape sample rate index illegal in ADTS\n");
        return -1;
    }
    if (adts->channel_conf == 0) {
        av_log(NULL, AV_LOG_ERROR, "PCE based channel configuration");
        return -1;
    }
	
    adts->write_adts = 1;
	
    return 0;
}

static int aac_decode_init(AVCodecContext *avctx)
{
    ADTSContext *adts = avctx->priv_data;
	
    if(avctx->extradata_size > 0 &&
		decode_extradata(avctx, adts, avctx->extradata, avctx->extradata_size) < 0)
        return -1;

	return 0;
}

static int aac_write_frame_header(AVCodecContext *avctx, uint8_t *out_buf,int size)
{
    ADTSContext *ctx = avctx->priv_data;
    PutBitContext pb;
    //uint8_t buf[ADTS_HEADER_SIZE];
	
    //init_put_bits(&pb, buf, ADTS_HEADER_SIZE);
	
	init_put_bits(&pb, out_buf, ADTS_HEADER_SIZE);

    /* adts_fixed_header */
    put_bits(&pb, 12, 0xfff);   /* syncword */
    put_bits(&pb, 1, 0);        /* ID */
    put_bits(&pb, 2, 0);        /* layer */
    put_bits(&pb, 1, 1);        /* protection_absent */
    put_bits(&pb, 2, ctx->objecttype); /* profile_objecttype */
    put_bits(&pb, 4, ctx->sample_rate_index);
    put_bits(&pb, 1, 0);        /* private_bit */
    put_bits(&pb, 3, ctx->channel_conf); /* channel_configuration */
    put_bits(&pb, 1, 0);        /* original_copy */
    put_bits(&pb, 1, 0);        /* home */
	
    /* adts_variable_header */
    put_bits(&pb, 1, 0);        /* copyright_identification_bit */
    put_bits(&pb, 1, 0);        /* copyright_identification_start */
    put_bits(&pb, 13, ADTS_HEADER_SIZE + size); /* aac_frame_length */
    put_bits(&pb, 11, 0x7ff);   /* adts_buffer_fullness */
    put_bits(&pb, 2, 0);        /* number_of_raw_data_blocks_in_frame */
	
    flush_put_bits(&pb);
	//memcpy(out_buf,buf,ADTS_HEADER_SIZE);
    //put_buffer(s->pb, buf, ADTS_HEADER_SIZE);
	
    return ADTS_HEADER_SIZE;
}

static int aac_decode_frame(AVCodecContext * avctx,
							void *data, int *data_size,
							const uint8_t * buf, int buf_size)
{
	int len = 0;
	ADTSContext *adts = avctx->priv_data;
	uint8_t *out_buf = (uint8_t *)data;

    if (!buf_size)
        return 0;
    if(adts->write_adts)
	{
        len = aac_write_frame_header(avctx, out_buf, buf_size);
		out_buf += len;
	}
	memcpy(out_buf,buf,buf_size);
	*data_size = (len + buf_size);

	return 0;
    //put_buffer(pb, buf, pkt->size);
    //put_flush_packet(pb);
}

AVCodec aac_decoder =
{
    "aac",
    CODEC_TYPE_AUDIO,
    CODEC_ID_AAC,
    sizeof(ADTSContext),
    aac_decode_init,
    NULL,
    NULL,
    aac_decode_frame,
    CODEC_CAP_PARSE_ONLY,
    NULL,NULL,NULL,NULL,NULL
};

