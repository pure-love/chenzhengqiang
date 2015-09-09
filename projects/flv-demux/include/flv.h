/*
@author:internet
@modified author:chenzhengqiang
@start date:2015/9/9
*/

#ifndef _CZQ_FLV_H_
#define _CZQ_FLV_H_

#include "common.h"
typedef struct Tag_File_Header
{
	unsigned char Signature_1;                 //always 'F' (0x46)
	unsigned char Signature_2;                 //always 'L' (0x4C)
	unsigned char Signature_3;                 //always 'V' (0x56)
	unsigned char version  ;                   //�汾�� ������0x01
	unsigned char TypeFlagsReserved_1;         //��5���ֽڵ�ǰ5λ����������Ϊ0��
	unsigned char TypeFlagsAudio;              //��5���ֽڵĵ�6λ��ʾ�Ƿ������ƵTag��
	unsigned char TypeFlagsReserved_2;         //��5���ֽڵĵ�7λ����������Ϊ0��
	unsigned char TypeFlagsVideo;              //��5���ֽڵĵ�8λ��ʾ�Ƿ������ƵTag��
	unsigned int  DataOffset;                  //��6-9���ֽ�ΪUI32���͵�ֵ����ʾ��File Header��ʼ��File Body��ʼ���ֽ������汾1����Ϊ9

}File_Header;

//����tag��header��tag��data
typedef struct Tag_Tag                           
{
	unsigned char Type ;                       //��Ƶ��0x08������Ƶ��0x09����script data��0x12����������
    unsigned int  DataSize;                    //������tagheader �ĳ���
	unsigned int  Timestamp;                   //��5-7�ֽ�ΪUI24���͵�ֵ����ʾ��Tag��ʱ�������λΪms������һ��Tag��ʱ�������0��
	unsigned char TimestampExtended;           //��8���ֽ�Ϊʱ�������չ�ֽڣ���24λ��ֵ����ʱ�����ֽ���Ϊ���λ��ʱ�����չΪ32λֵ��
	unsigned int  StreamID;                    //��9-11�ֽ�ΪUI24���͵�ֵ����ʾstream id������0��
	unsigned char * Data;                      //Tag Data����
}Tag;

int AllocStruct_File_Header(File_Header ** fileheader);
int FreeStruct_File_Header(File_Header * fileheader);
int ReadStruct_File_Header(unsigned char * Buf , unsigned int length ,File_Header * fileheader);
#endif