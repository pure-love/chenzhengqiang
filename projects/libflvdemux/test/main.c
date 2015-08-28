/********************************************************************
created:	2010/6.1	
filename: 	main.c
author:		shaozhong.liang

purpose:	test for flv decoder
*********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <arpa/inet.h>

#include "demuxer.h"

int main(int argc, char **argv)
{
	const char *out_filename = "./out_h264_pts.dat";
	void* pdemuxer = NULL;
	uint8_t outDecodeBuf[10*1024];

	uint32_t        data_type,data_len,pts;
	uint64_t        timestamp;
	FILE           *file_out;
	
	uint16_t        frame_type,frame_len;
	
	printf("Starting ...\n");
		
	file_out = fopen(out_filename, "wb");
	if (!file_out) 
	{
		fprintf(stderr, "could not open %s\n", out_filename);
		return -1;
	}
	pdemuxer = open_flvdemux(argv[1]);
	if(pdemuxer == NULL)
	{
		fclose(file_out);
		return -2;
	}
	
	while(1) 
	{
		data_len = demux_read_frame(pdemuxer, outDecodeBuf, &data_type, &timestamp);
		
		if(data_len <= 0)
			break;

		printf("date type=%d size=%-4d dts=%lld\n",data_type, data_len, timestamp);
		
		// Is this a packet from the video or audio stream?
		if(data_type == 1) 
		{
			frame_type = htons(0);
		}else
		{
			frame_type = htons(1);
		}
		frame_len = (uint16_t)data_len;
		
		fwrite(&frame_type, sizeof(frame_type), 1,file_out);
			
		frame_len = htons(frame_len);
		fwrite(&frame_len, sizeof(frame_len), 1,file_out);
			
		pts = htonl((uint32_t)timestamp);
		fwrite(&pts, sizeof(pts), 1,file_out);
			
		fwrite(outDecodeBuf, data_len, 1,file_out);
	}
	close_flvdemux(pdemuxer);
	
	printf("End...\n");
	
	fclose(file_out);

	printf("Bye!\n");
		
	return 0;
}


