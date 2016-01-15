/*
*@filename:process.h
*@author:chenzhengqiang
*@start date:2016/01/11 09:58:03
*@modified date:
*@desc: 
*/



#ifndef _CZQ_PROCESS_H_
#define _CZQ_PROCESS_H_
#include<unistd.h>
namespace czq
{
	//write the function prototypes or the declaration of variables here
	class process
	{
		public:
			pid_t pid_;
			int pipeFd_[2];
	};
};	
#endif
