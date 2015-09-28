/*
 * @author:chenzhengqiang
 * @company:swwy
 * @date:2015/3/27
 * @modified-date:
 * @version:1.0
 * @desc:
 *  these are the basic headers needed
 */

#ifndef _COMMON_H_
#define _COMMON_H_
#include<cstdio>
#include<cstdlib>
#include<stdint.h>
#include<cstring>
#include<string>
#include<map>
#include<utility>
#include<queue>
#include<unistd.h>
#include<signal.h>
#include<sys/socket.h>
#include<netinet/tcp.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<vector>
#include<ev.h>
#include<iostream>
#include<netdb.h>
#include<fcntl.h>
#include<errno.h>
#include<sstream>
#include<memory>
#include<fstream>
#include<sys/select.h>
#include<sys/epoll.h>
#include<sys/resource.h>
extern int errno;

static const int DEFAULT = 0;
static const int SELECT=1;
static const int EPOLL = 2;
static const int LIBEV = 3;

static const int MAX_OPEN_FDS = 65535;
using std::string;
#endif
