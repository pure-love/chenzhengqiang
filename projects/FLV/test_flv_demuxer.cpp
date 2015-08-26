#include <cstdio>
#include <iostream>
#include <cstdlib>


using namespace std;

FILE *fflv_handler=NULL;
FILE *faac_handler =NULL;



#define HTON16(x)  ((x>>8&0xff)|(x<<8&0xff00))
#define HTON24(x)  ((x>>16&0xff)|(x<<16&0xff0000)|x&0xff00)
#define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)|\
(x<<8&0xff0000)|(x<<24&0xff000000))

#define STR(x) (x.c_str())
#define FCUR(x) (ftell(x))
#define FSEEK(x,f) (fseek(f,x,SEEK_CUR))
#define FSET(x,f) (fseek(f,x,SEEK_SET))
#define LOG(x,f) (fprintf(f,STR(x)))

bool Read8(int &i8,FILE*f);
bool Read16(int &i16,FILE*f);
bool read_24_bit( int &bit24,FILE*f );
bool read_32_bit( int &bit32,FILE*f );
bool ReadTime(int &itime,FILE*f);
bool Peek8(int &i8,FILE*f);

//在flv中第一帧
struct FLVAACADST{
	unsigned char SamplIndex1:3;
	unsigned char OBjecttype:5;//2
	
	unsigned char other:3;//000
	unsigned char channel:4;
	unsigned char SamplIndex2:1;
	
};

//aac封装 每个帧头结构
struct AACADST{
	unsigned	char check1;
	
	unsigned  char protection:1;//误码校验1
	unsigned  char layer:2;//哪个播放器被使用0x00
	unsigned	char ver:1;//版本 0 for MPEG-4, 1 for MPEG-2
	unsigned	char check2:4;
	
	unsigned	char channel1:1;
	unsigned    char privatestream:1;//0
	unsigned	char SamplingIndex:4;//采样率
	unsigned	char ObjectType:2;
	
	unsigned	char length1:2;
	unsigned	char copyrightstart:1;//0
	unsigned    char copyrightstream:1;//0
	unsigned    char home:1;//0
	unsigned    char originality:1;//0
	unsigned	char channel2:2;
	
	unsigned	char length2;
	
	unsigned	char check3:5;
	unsigned	char length3:3;
	
	unsigned	char frames:2;//超过一块写
	unsigned	char check4:6;
};


void clear();
bool varify_flv_ok();
void flv_demux_for_aac();


char flvfilename[256]={0};
char aacfilename[256]={0};
bool hasname=false;

static const char *usage="usage:%s <flv file> <aac file>\n";
int main( int argc,char**argv )
{
	if( argc !=3 )
      {
          printf(usage,argv[0]);
          exit(EXIT_FAILURE);
      }
       
      fflv_handler = fopen(argv[1],"r" );
      faac_handler = fopen( argv[2],"w");
      
	if(!varify_flv_ok())
	{
	    cout<<"invalid format for flv file "<<argv[1]<<endl;
           clear();
           exit(EXIT_FAILURE);
	}
       cout<<"flv demux start"<<endl;
	flv_demux_for_aac();
	cout<<"flv demux done"<<endl;
	clear();
	return 0;
}

void clear()
{
	fclose(fflv_handler);
	fclose(faac_handler);
}

bool varify_flv_ok()
{
	int flv_header_length=0;
	int flv_signature=0;
	if(!read_24_bit( flv_signature,fflv_handler))
       return false;	
    
	int FIXED_FLV_SIGNATURE='FLV';
    
	if ( flv_signature != FIXED_FLV_SIGNATURE  )
	{
		return false;
	}

	FSEEK(2,fflv_handler );
	if (!read_32_bit(flv_header_length,fflv_handler ))
	return false;

       //just skip the header length size + 4 bytes
       //cause the first tag which hold 4 bytes is always 0
	fseek(fflv_handler,0,SEEK_SET);
	FSEEK(flv_header_length,fflv_handler);
	FSEEK(4,fflv_handler);
	return true;
}

void AACJIEXI(int datelength);

void flv_demux_for_aac()
{
	while(true)
	{		
	
		int type=0;
		int time=0;
		int htime=0;
		int datelength=0;
		int info=0;
	
		char buff[256]={0};
		if (!Read8(type,fflv_handler))
			break;
		if (!read_24_bit(datelength,fflv_handler))
			break;
		if(!ReadTime(time,fflv_handler))
			break;
		////////跳过StreamID/////
		FSEEK(3,fflv_handler);
		int pos=FCUR(fflv_handler);
		if(type==8)
			AACJIEXI(datelength);
		FSET(pos+datelength+4,fflv_handler);
	}
}



bool HasAudioSpecificConfig=false;//判断是否有AudioSpecificConfig

AACADST m_aacadst={0};
FLVAACADST m_flvadst={0};

int nClean=0;
int nSampelindex=0;
void AACJIEXI(int datelength)//关键
{
	int info=0;
	int aactype=0;
	Read8(info,fflv_handler);
	Read8(aactype,fflv_handler);
	if (aactype==0x00)
	{
		HasAudioSpecificConfig=true;
		fread(&m_flvadst,1,sizeof(m_flvadst),fflv_handler);//常见的有0x1210 0x1390,有时候会有7个字节，只要前面两个就行
		m_aacadst.check1=0xff;
		m_aacadst.check2=0xff;
		m_aacadst.check3=0xff;
		m_aacadst.check4=0xff;
		m_aacadst.protection=1;
		m_aacadst.ObjectType=0;
		m_aacadst.SamplingIndex=m_flvadst.SamplIndex2|m_flvadst.SamplIndex1<<1;
		m_aacadst.channel2=m_flvadst.channel;		
		return;
	}
	else
	{
		
	if (HasAudioSpecificConfig)
	{
		
			unsigned int size=datelength-2+7;
			m_aacadst.length1=(size>>11)&0x03;
			m_aacadst.length2=(size>>3)&0xff;
			m_aacadst.length3=size&0x07;
		
			fwrite((char*)&m_aacadst,1,sizeof(AACADST),faac_handler);
	}
	int templength=datelength-2;
	char*tempbuff=NULL;
	tempbuff=(char*)malloc(templength);
	fread(tempbuff,1,templength,fflv_handler);
	fwrite(tempbuff,1,templength,faac_handler);
	free(tempbuff);
	}
	
}


bool Peek8(int &i8,FILE*f)
{
	if(fread(&i8,1,1,f)!=1)
		return false;
	fseek(f,-1,SEEK_CUR);
	return true;
}

bool Read8(int &i8,FILE*f)
{
	if(fread(&i8,1,1,f)!=1)
		return false;
	return true;
}

bool Read16(int &i16,FILE*f)
{
	if(fread(&i16,2,1,f)!=1)
		return false;
	i16=HTON16(i16);
	return true;
}

bool read_24_bit( int &bit24, FILE*f )
{
	if(fread(&bit24,3,1,f)!=1)
		return false;
	bit24 =HTON24(bit24);
	return true;
}

bool read_32_bit( int &bit32, FILE*f )
{
	if(fread(&bit32,4,1,f)!=1)
		return false;
	bit32 =HTON32( bit32 );
	return true;
}

bool ReadTime(int &itime,FILE*f)
{
	int temp=0;
	if(fread(&temp,4,1,f)!=1)
		return false;
	itime=HTON24(temp);
	itime|=(temp&0xff000000);
	return true;
}

