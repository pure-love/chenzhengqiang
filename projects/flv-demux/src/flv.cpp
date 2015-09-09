#include "flv.h"
int AllocStruct_File_Header(File_Header ** fileheader)
{
	File_Header *fileheader_t = * fileheader;
	if ((fileheader_t = (File_Header *)calloc(1,sizeof(File_Header))) == NULL)
	{
		printf ("Error: Allocate Meory To AllocStruct AllocStruct_File_Header Buffer Failed ");
		return getchar();
	}
	* fileheader = fileheader_t;
	return 1;
}

int FreeStruct_File_Header(File_Header * fileheader)
{
	if (fileheader)
	{
		free(fileheader);
		fileheader = NULL;
	}
	return 1;
}

int ReadStruct_File_Header(unsigned char * Buf , unsigned int length ,File_Header * fileheader)
{
	int File_Header_pos = 0;
	fileheader->Signature_1 = Buf[File_Header_pos];
	fileheader->Signature_2 = Buf[File_Header_pos +1];
	fileheader->Signature_3 = Buf[File_Header_pos +2];
	fileheader->version = Buf[File_Header_pos +3];
	File_Header_pos +=4;
    fileheader->TypeFlagsReserved_1 = Buf[File_Header_pos] >> 3;
	fileheader->TypeFlagsAudio = ( Buf[File_Header_pos] >> 2) & 0x01;
	fileheader->TypeFlagsReserved_2 = Buf[File_Header_pos] & 0x02;
	fileheader->TypeFlagsVideo = Buf[File_Header_pos] & 0x01;
	File_Header_pos ++;
    fileheader->DataOffset = 
		Buf[File_Header_pos]     << 24 |
		Buf[File_Header_pos +1]  << 16 |
		Buf[File_Header_pos +2]  << 8  |
		Buf[File_Header_pos +3];
	File_Header_pos +=4;
	if (length != File_Header_pos )
	{
		printf ("Error :ReadStruct_File_Header length \n");
		return getchar();
	}
	return 1;
}