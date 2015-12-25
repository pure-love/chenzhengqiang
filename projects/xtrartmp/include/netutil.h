/*
 * @Author:chenzhengqiang
 * @Date:2015/3/16
 * @Modified:
 * @desc:
 */
#ifndef _CZQ_NETUTIL_H_
#define _CZQ_NETUTIL_H_

namespace czq
{
	class NetUtil
	{
		public:
			static int registerTcpServer(const char *IP, int PORT );
	};
}
#endif
