/*
 * FLV DEMUX
 * Shaozhong.Liang@gmail.com
 * 10/6/2010
 */

#ifndef AV_DEMUXER_H
#define AV_DEMUXER_H

#define  AV_VIDEO_DATA  1
#define  AV_AUDIO_DATA  2

void* open_flvdemux(const char *filename);
int close_flvdemux(const void* pDemux_ptr);
int demux_read_frame(const void* pDemux_ptr, uint8_t *buff, uint32_t *data_type, uint64_t *timestamp);


#endif /* AVUTIL_BASE64_H */
