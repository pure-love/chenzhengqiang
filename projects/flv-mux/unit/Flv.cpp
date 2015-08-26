#include "Flv.h"

int WriteStruct_File_Header(unsigned char * Buf , unsigned int length)
{
	//写文件头
	File_Header fileheader;
	fileheader.Signature_1 = 0x46;
	fileheader.Signature_2 = 0x4C;
	fileheader.Signature_3 = 0x56;
	fileheader.version = 0x01;
	fileheader.TypeFlagsReserved_1 = 0x00;
	fileheader.TypeFlagsAudio = 0x01;
	fileheader.TypeFlagsReserved_2 = 0x00;
	fileheader.TypeFlagsVideo = 0x01;
	fileheader.DataOffset = 0x09;
	//填写文件头buf
	Buf[0] = fileheader.Signature_1;
	Buf[1] = fileheader.Signature_2;
	Buf[2] = fileheader.Signature_3;
	Buf[3] = fileheader.version;
	Buf[4] = (fileheader.TypeFlagsReserved_1 << 3) | (fileheader.TypeFlagsAudio << 2) | (fileheader.TypeFlagsReserved_2 << 1) | (fileheader.TypeFlagsVideo);
	Buf[5] = (fileheader.DataOffset  & 0xFF000000) >> 24;
	Buf[6] = (fileheader.DataOffset  & 0x00FF0000) >> 16;
	Buf[7] = (fileheader.DataOffset  & 0x0000FF00) >> 8;
	Buf[8] = (fileheader.DataOffset  & 0x000000FF);
	return 1;
}