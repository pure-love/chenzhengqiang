/*
@author:chenzhengqiang
@start date:2015/12/23
@modified date:
@desc:self defined eror codes
*/

#ifndef _CZQ_ERRORS_H_
#define _CZQ_ERRORS_H_

enum ERRORS
{
	OK = 0,
	SYSTEM_ERROR = -99,
	ARGUMENT_ERROR = -98,
	FILE_EOF = -97,
	FILE_FORMAT_ERROR = -96,
	FILE_LENGTH_ERROR = -95,
	LENGTH_OVERFLOW = -94,
	STREAM_FORMAT_ERROR = -93,
	STREAM_LENGTH_ERROR = -92
};
#endif
