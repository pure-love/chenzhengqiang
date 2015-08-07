/*
@this is an example of using speex library to denoise
*/

#define NN 882
 
int main()
{
    short in[NN];
    int i;
    SpeexPreprocessState *st;
    int count=0;
    float f;
 
    FILE *fp_src_wav, *fp_dst_wav;
    RIFF_HEADER riff;
    FMT_BLOCK fmt;
    FACT_BLOCK fact;
    DATA_BLOCK data;
    int samples, len_to_read, actual_len;//
    BYTE *src_all, *dst_all;
    //BYTE *src_left, *src_right;
    int frame_num;
 
    //各个文件路径名
    char src_wav_name[]=".\\input_output\\sc2_one_ch.wav";
    char dst_wav_name[]=".\\input_output\\output.wav";
 
    st = speex_preprocess_state_init(NN, 44100);
    i=1;
    speex_preprocess_ctl(st, SPEEX_PREPROCESS_SET_DENOISE, &i);
 
    fp_src_wav=fopen(src_wav_name, "rb");
    if( !fp_src_wav )
    {
        printf("failed to open src wav file, exit...\n");
        exit(1);
    }
 
    fp_dst_wav=fopen(dst_wav_name, "wb");
    if(!fp_dst_wav)
    {
        printf("failed to open dst wav file, exit...\n");
        exit(1);
    }
 
    read_file_header(&fmt, &riff, &fact, &data, fp_src_wav);
    write_file_header(&fmt, &riff, &fact, &data, fp_dst_wav);
 
    if(fmt.wavFormat.wBitsPerSample==8)
    {
        printf("8 bits sample can't be dealed! exit...\n");
        fclose(fp_src_wav);
        fclose(fp_dst_wav);
        exit(1);
    }
 
    samples=NN;
    len_to_read=samples*fmt.wavFormat.wChannels*(fmt.wavFormat.wBitsPerSample/8);
    //分配内存空间
    src_all=(BYTE*)malloc(sizeof(BYTE)*len_to_read);
    dst_all=(BYTE*)malloc(sizeof(BYTE)*len_to_read);
 
    //依次读取wav内容、处理、输出
    frame_num=-1;
    for(;;)
    {
        //从文件读取数据
        actual_len=fread(src_all, 1, len_to_read, fp_src_wav);
        if(!actual_len)
        {
            //数据已经读取完毕
            break;
        }
        frame_num++;
        //添加噪声
        if(fmt.wavFormat.wChannels==1)
        {
            speex_preprocess_run(st, (short*)src_all);
        }
        else
        {
        }
        fwrite(src_all, 1, actual_len, fp_dst_wav);
    }//end for(;;)
 
    //释放空间
    free(src_all);
    free(dst_all);
 
    //关闭文件
    fclose(fp_src_wav);
    fclose(fp_dst_wav);
 
    speex_preprocess_state_destroy(st);
    return 0;
}

