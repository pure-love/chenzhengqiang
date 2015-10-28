/*
*@author:chenzhengqiang
*@start date:2015/7/9
*@modified date:
*@desc:generate the personal signalling
*/

/*
*@args:
*@returns:true indicates generate successed,false otherwise
*@desc:generate the personal signalling
  //the buf_size must greater than 64
  //and the channel's length must less equal than 24
 // the flags only support 0 and 1,0 indicates camera,1 indicates pc
 // the action only supports 0 and 1,0 indicates CREATE AND JOIN, 1 indicates CANCEL AND LEAVE
*/
#include "signalling.h"
#include<cstring>
#include<sstream>
#include<time.h>
bool generate_signalling(char *signalling_buf,size_t buf_size,
                                              const char *channel,int flags, int action )
{
    if( buf_size < SIGNALLING_LENGTH || strlen(channel) > CHANNEL_LENGTH )
    return false;
    if( flags != CAMERA && flags != PC )
    return false;

    if( action != 0 && action != 1)
    return false;
    
    std::ostringstream OSS_signalling;
    if( flags == CAMERA )
    {
        if(action == CREATE )
        {
            OSS_signalling<<"CAMERA_CREATE_"<<channel<<"_";
        }
        else
        {
            OSS_signalling<<"CAMERA_CANCEL_"<<channel<<"_";
        }
    }
    else
    {
        if( action == JOIN )
        {
            OSS_signalling<<"PC_JOIN_"<<channel<<"_";
        }
        else
        {
            OSS_signalling<<"PC_LEAVE_"<<channel<<"_";
        }
    }
    
    time_t now = time(NULL);
    OSS_signalling<<now;
    if(OSS_signalling.str().length() > buf_size )
    return false;    
    strcpy(signalling_buf,OSS_signalling.str().c_str());
    return true;
}
