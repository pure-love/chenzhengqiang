/*#include<cstdio>
#include<cmath>
#include "speech_mix.h"
#define PCMFILE1 "sample.pcm"
#define PCMFILE2 "sample1.pcm"
#define MIX_PCM_FILE "mix.pcm"
#define BUFFER_SIZE (640*10)

int main(int argc,char *argv[]) 
{
  (void)argc;
  (void)argv;
  FILE *pcm1, *pcm2, *mix;
 
  pcm1 = fopen(argv[1],"r");
  pcm2 = fopen(argv[2],"r");

  char pcm_buffer1[BUFFER_SIZE];
  char pcm_buffer2[BUFFER_SIZE];
  char pcm_buffer3[BUFFER_SIZE];
  
  mix = fopen(MIX_PCM_FILE, "w");

  fread(pcm_buffer1,sizeof(char),BUFFER_SIZE,pcm1);
  fread(pcm_buffer2,sizeof(char),BUFFER_SIZE,pcm2);
  //generate_speech_mix(pcm_buffer1,pcm_buffer2,pcm_buffer3,BUFFER_SIZE);
  fwrite(pcm_buffer3,sizeof(char),BUFFER_SIZE,mix);
  fclose(pcm1);
  fclose(pcm2);
  fclose(mix);

  return 0;
}*/

#include <stdio.h>  
#include <stdlib.h>  
#include <math.h>  
   
#define IN_FILE1 "sample.pcm"  
#define IN_FILE2 "sample1.pcm"  
#define OUT_FILE "remix.pcm"  
   
#define SIZE_AUDIO_FRAME (2)  
   
void Mix(char sourseFile[10][SIZE_AUDIO_FRAME],int number,char *objectFile)  
{  
    //πÈ“ªªØªÏ“Ù  
    int const MAX=32767;  
    int const MIN=-32768;  
   
    double f=1;  
    int output;  
    int i = 0,j = 0;  
    for (i=0;i<SIZE_AUDIO_FRAME/2;i++)  
    {  
        int temp=0;  
        for (j=0;j<number;j++)  
        {  
            temp+=*(short*)(sourseFile[j]+i*2);  
        }                  
        output=(int)(temp*f);  
        if (output>MAX)  
        {  
            f=(double)MAX/(double)(output);  
            output=MAX;  
        }  
        if (output<MIN)  
        {  
            f=(double)MIN/(double)(output);  
            output=MIN;  
        }  
        if (f<1)  
        {  
            f+=((double)1-f)/(double)32;  
        }  
        *(short*)(objectFile+i*2)=(short)output;  
    }  
}  
   

int main()  
{  
    FILE * fp1,*fp2,*fpm;  
    fp1 = fopen(IN_FILE1,"rb");  
    fp2 = fopen(IN_FILE2,"rb");  
    fpm = fopen(OUT_FILE,"wb");  
       
    short data1,data2,date_mix;  
    int ret1,ret2;  
    char sourseFile[10][2];  
    int count=0;
    while(1)  
    {  
        ret1 = fread(&data1,2,1,fp1);  
        ret2 = fread(&data2,2,1,fp2);  
        *(short*) sourseFile[0] = data1;  
        *(short*) sourseFile[1] = data2;  
           
        if(ret1>0 && ret2>0)  
        {  
            Mix(sourseFile,2,(char *)&date_mix);  
            if(date_mix > pow(2,16-1) || date_mix < -pow(2,16-1))  
                printf("mix error\n");  
        }  
        else if( (ret1 > 0) && (ret2==0))  
        {  
            date_mix = data1;  
        }  
        else if( (ret2 > 0) && (ret1==0))  
        {  
            date_mix = data2;  
        }  
        else if( (ret1 == 0) && (ret2 == 0))  
        {  
            break;  
        }  
        fwrite(&date_mix,2,1,fpm);
        ++count;
        if( count == 320*120)
        break;    
    }  
    fclose(fp1);  
    fclose(fp2);  
    fclose(fpm);  
    printf("Done!\n");  
} 

