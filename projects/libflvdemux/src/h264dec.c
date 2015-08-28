/*
 * H.26L/H.264/AVC/JVT/14496-10/... encoder/decoder
 * Copyright (c) 2003 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file h264.c
 * H.264 / AVC / MPEG4 part10 codec.
 * @author Michael Niedermayer <michaelni@gmx.at>
 */

#include "bitstream.h"
#include "internal.h"
#include "avformat.h"
#include "avio.h"

//#undef NDEBUG
#include <assert.h>


typedef struct H264Context{
    int nal_ref_idc;
    int nal_unit_type;

    /**
      * Used to parse AVC variant of h264
      */
    int is_avc; ///< this flag is != 0 if codec is avc1
    int got_avcC; ///< flag used to parse avcC data only once
    int nal_length_size; ///< Number of bytes used for nal length (1, 2 or 4)
}H264Context;


static av_cold int decode_init(AVCodecContext *avctx){
    H264Context *h= avctx->priv_data;

    if(avctx->extradata_size > 0 && avctx->extradata &&
        *(char *)avctx->extradata == 1)
    {
        h->is_avc = 1;
        h->got_avcC = 0;
    } 
    else 
    {
        h->is_avc = 0;
    }
    
    return 0;
}

static int decode_nal_units(H264Context *h, uint8_t *buf, int buf_size){
    int buf_index=0;

    for(;;)
    {
        int i, nalsize = 0;

        if(h->is_avc) 
        {
            if(buf_index >= buf_size) break;
            nalsize = 0;
            for(i = 0; i < h->nal_length_size; i++)
            {
                nalsize = (nalsize << 8) | buf[buf_index];
                buf[buf_index++] = 0;
            }
            buf[buf_index-1] = 1;
            
            if(nalsize <= 1 || (nalsize+buf_index > buf_size))
            {
                if(nalsize == 1)
                {
                    buf_index++;
                    continue;
                }
                else
                {
                    av_log(NULL, AV_LOG_ERROR, "AVC: nal size %d\n", nalsize);
                    break;
                }
            }
            buf_index+=nalsize;
        } else {
            // start code prefix search
            for(; buf_index + 3 < buf_size; buf_index++){
                // This should always succeed in the first iteration.
                if(buf[buf_index] == 0 && buf[buf_index+1] == 0 && buf[buf_index+2] == 1)
                    break;
            }
            if(buf_index+3 >= buf_size) break;

            buf_index+=3;
        }
    }
    
    return buf_index;
}

static int decode_frame(AVCodecContext *avctx,
                             void *data, int *data_size,
                             const uint8_t *buf, int buf_size)
{
    H264Context *h = avctx->priv_data;

    int buf_index;

   /* end of stream, output what is still in the buffers */
    if (buf_size == 0) 
    {
        return 0;
    }

    if(h->is_avc && !h->got_avcC) 
    {
        int i, cnt, nalsize,len=0;
        unsigned char *p = avctx->extradata;
        unsigned char *data_ptr = (unsigned char *)data;

        if(avctx->extradata_size < 7) 
        {
            av_log(avctx, AV_LOG_ERROR, "avcC too short\n");
            return -1;
        }
        //av_log(avctx, AV_LOG_ERROR, "got_avcC extradata_size=%d\n",avctx->extradata_size);
        
        if(*p != 1) 
        {
            av_log(avctx, AV_LOG_ERROR, "Unknown avcC version %d\n", *p);
            return -1;
        }
        // Now store right nal length size, that will be use to parse all other nals
        h->nal_length_size = ((*(((char*)(avctx->extradata))+4))&0x03)+1;
        
        // Decode sps from avcC
        cnt = *(p+5) & 0x1f; // Number of sps
        p += 6;
        for (i = 0; i < cnt; i++) 
        {
            nalsize = AV_RB16(p);
            p += 2;
            
            *data_ptr++ = 0;
            *data_ptr++ = 0;
            if(h->nal_length_size > 3)
            {
                *data_ptr++ = 0;
            }
            *data_ptr++ = 1;
            memcpy(data_ptr, p, nalsize);
            data_ptr += nalsize;
            len += h->nal_length_size + nalsize;

            p += nalsize;
        }
        // Decode pps from avcC
        cnt = *(p++); // Number of pps
        for (i = 0; i < cnt; i++) 
        {
            nalsize = AV_RB16(p) ;
            p += 2;

            *data_ptr++ = 0;
            *data_ptr++ = 0;
            if(h->nal_length_size > 3)
            {
                *data_ptr++ = 0;
            }
            *data_ptr++ = 1;
            memcpy(data_ptr, p, nalsize);
            data_ptr += nalsize;
            len += h->nal_length_size + nalsize;

            p += nalsize;
        }
        // Do not reparse avcC
        h->got_avcC = 1;
        
        buf_index=decode_nal_units(h, (uint8_t *)buf, buf_size);         
        if(buf_index > 0)
        {
        	memcpy(data_ptr, buf, buf_index);
            len += buf_index;
        }        
        *data_size = len;

        return 0;
    }

    if(!h->got_avcC && !h->is_avc && avctx->extradata_size){
        if(decode_nal_units(h, avctx->extradata, avctx->extradata_size) < 0)
            return -1;
        h->got_avcC = 1;
    }

    buf_index=decode_nal_units(h, (uint8_t *)buf, buf_size);
    if(buf_index < 0)
        return -1;
    
    memcpy(data,buf,buf_index);
    *data_size = buf_index;

    return 0;
}

static av_cold int decode_end(AVCodecContext *avctx)
{

    return 0;
}


AVCodec h264_decoder = {
    "h264",
    CODEC_TYPE_VIDEO,
    CODEC_ID_H264,
    sizeof(H264Context),
    decode_init,
    NULL,
    decode_end,
    decode_frame,
    /*CODEC_CAP_DRAW_HORIZ_BAND |*/ CODEC_CAP_DR1 | CODEC_CAP_DELAY,
    NULL,
    NULL,
};
