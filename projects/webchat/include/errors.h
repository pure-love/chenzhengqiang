/*
@author:chenzhengqiang
@start date:2015/12/23
@modified date:
@desc:self defined eror codes
*/

#ifndef _CZQ_ERRORS_H_
#define _CZQ_ERRORS_H_

namespace czq
{
	enum ERRORS
	{
		MISS_OK = 0,
		MISS_SYSTEM_ERROR = -99,
		MISS_ARGUMENT_ERROR = -98,
		MISS_FILE_EOF = -97,
		MISS_FILE_FORMAT_ERROR = -96,
		MISS_FILE_LENGTH_ERROR = -95,
		MISS_LENGTH_OVERFLOW = -94,
		MISS_STREAM_FORMAT_ERROR = -93,
		MISS_STREAM_LENGTH_ERROR = -92,
		MISS_CONFIG_ERROR = -91
	};
};
#endif
