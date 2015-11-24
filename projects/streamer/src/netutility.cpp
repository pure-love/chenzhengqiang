/*
 *@Author:chenzhengqiang
 *@company:swwy
 *@date:2013/3/26
 *@modified:
 *@desc:
 @version:1.0
 */


#include "netutility.h"
#include "common.h"
#include "logging.h"

static const int LISTENQ=10;

using std::string;


/*
#@args:the "host" stands for the hostname，
# and the "service" stands for our streaming server's name which registered in /etc/services
#@returns:return the listen socket descriptor that has been registerd in server
*/
int tcp_listen( const char *host, const char *service )
{
    int listen_fd,ret;
    int REUSEADDR_ON;
    struct addrinfo servaddr,*res,*saveaddr;
    servaddr.ai_family = AF_UNSPEC;
    servaddr.ai_flags = AI_PASSIVE;
    servaddr.ai_protocol = 0;
    servaddr.ai_socktype = SOCK_STREAM;
    ret=getaddrinfo(host,service,&servaddr,&res);
    if( ret != 0 )
    {
        fprintf(stderr,"[$ERROR$]--tcp_listen error when calling getaddr_info %s",gai_strerror(ret));
        return -1;
    }
    REUSEADDR_ON=1;
    listen_fd = -1;
    saveaddr = res;
    do
    {
        listen_fd = socket(res->ai_family,res->ai_socktype,res->ai_protocol);
        if( listen_fd < 0 )
        {
            continue;
        }
        ret=setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&REUSEADDR_ON,sizeof(REUSEADDR_ON));
        if( ret != 0 )
        {
            fprintf(stderr,"[$ERROR$]--tcp_listen-->setsockopt error for %s\n",strerror(errno));
        }
        ret=bind(listen_fd,res->ai_addr,res->ai_addrlen);
        if(  ret != 0 )
        {
            fprintf(stderr,"[$ERROR$]--tcp_listen-->bind error for %s\n",strerror(errno));
        }
        ret = listen(listen_fd,LISTENQ);
        if( ret == 0 )
        {
            break;
        }
        else
        {
            fprintf(stderr,"[$ERROR$]--tcp_listen-->listen error for fd:%d;errror info:%s\n",listen_fd,strerror(errno));
        }
        close(listen_fd);
    }while((res=res->ai_next)!=NULL);

    if( res == NULL )
    {
        fprintf(stderr,"[$ERROR$]-->tcp_listen error for %s %s",host,service);
    }
    freeaddrinfo(saveaddr);
    return listen_fd;
}



/*
#@args:
#@:returns:return a descriptor for communicating with another streaming server
#@:as its name desceibed
*/
int tcp_connect( const char *IP, int PORT )
{
    struct sockaddr_in serv_addr;
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(PORT);
    if(inet_pton(AF_INET,IP,&serv_addr.sin_addr) != 1 )
    {
        log_module(LOG_INFO,"TCP_CONNECT","INET_PTON FAILED:%s",LOG_LOCATION);
        return -1;
    }
    bzero(&(serv_addr.sin_zero),sizeof(serv_addr.sin_zero));
    socklen_t serv_addr_len = sizeof(serv_addr);
    int local_fd = -1;
    if ((local_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        log_module(LOG_INFO,"TCP_CONNECT","SOCKET FAILED:%s",LOG_LOCATION);
        return -1;
    }
    sdk_set_nonblocking(local_fd);
    int ret = connect(local_fd, (struct sockaddr *)&serv_addr, serv_addr_len);
    if( ret < 0 && errno != EINPROGRESS )
    {
        log_module(LOG_INFO,"TCP_CONNECT","CONNECT FAILED:%s",LOG_LOCATION);
        return -1;
    }
    return local_fd;
}




/*
#@args:
#@:returns:return a descriptor for communicating with another streaming server
#@:as its name desceibed
*/
int register_tcp_server(const char *IP, int PORT )
{
    int listen_fd=-1;
    int REUSEADDR_ON=1;
    struct sockaddr_in server_addr;
    if(( listen_fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0)
    {
        perror("FAILED TO CREATE SOCKET");
        exit(EXIT_FAILURE);
    }

    sdk_set_nonblocking( listen_fd );
    sdk_set_tcpnodelay( listen_fd );
    sdk_set_keepalive( listen_fd );

    int ret=setsockopt( listen_fd, SOL_SOCKET, SO_REUSEADDR, &REUSEADDR_ON, sizeof(REUSEADDR_ON) );
    if( ret != 0 )
    {
        fprintf(stderr,"[$ERROR$]--TCP_LISTEN--SETSOCKOPT ERROR:%s\n",strerror(errno));
        exit(EXIT_FAILURE);
    }

    bzero(server_addr.sin_zero,sizeof(server_addr.sin_zero));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);
    if(bind(listen_fd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
    {
        perror("FAILED TO BIND SOCKET");
        exit(EXIT_FAILURE);
    }

    if(listen(listen_fd,LISTENQ) < 0)
    {
        perror("FAILED TO LISTEN");
        exit(EXIT_FAILURE);
    }

    return listen_fd;
}




/*
*@args:
*@returns:socket file descriptor
*@desc:as its name described,register udp server and return the socked descriptor
*/
int register_udp_server(const char *IP, int PORT )
{
      struct sockaddr_in local_addr;
      int sock_fd;
      bzero(&local_addr, sizeof(local_addr));

      local_addr.sin_family = AF_INET;
      local_addr.sin_addr.s_addr = inet_addr(IP);
      local_addr.sin_port = htons(PORT);

      sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
      if (-1 == sock_fd)
      {
          log_module(LOG_INFO,"REGISTER_UDP_SERVER","SOCKET ERROR:%s",strerror(errno));
          exit(EXIT_FAILURE);
      }
      
      if( -1 == bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)))
      {
          log_module(LOG_INFO,"REGISTER_UDP_SERVER","BIND ERROR:%s",strerror(errno));
          exit(EXIT_FAILURE);
      }
      
      return sock_fd;

}



/*
#@args:
# @returns:
*/
int read_specify_size( int fd, void *buffer, size_t total_bytes)
{
    //log_module( LOG_DEBUG,"READ_SPECIFY_SIZE","+++++START+++++");	
    size_t  left_bytes;
    int received_bytes;
    uint8_t *buffer_forward;
    buffer_forward = (uint8_t *)buffer;
    left_bytes = total_bytes;

    while ( true )
    {
        if ( (received_bytes = read(fd, buffer_forward, left_bytes)) <= 0)
        {
            if (  received_bytes < 0 )
            {
                if( errno == EINTR )
                {
                    received_bytes = 0;
                }
                else if( errno == EAGAIN || errno == EWOULDBLOCK )
                {
		        if( (total_bytes-left_bytes) == 0 )
		        continue;		
		        log_module( LOG_DEBUG,"RECEIVE_STREAM_CB","+++++DONE+++++");
		        return ( total_bytes-left_bytes );
                }
                else
		   {
	 	        log_module( LOG_ERROR,"READ_SPECIFY_SIZE","SOCKET READ ERROR OCCURRED:%s", strerror( errno ) );
		        log_module( LOG_DEBUG,"READ_SPECIFY_SIZE","+++++DONE+++++" );	
                     return 0;
		   }
            }
            else
	     {
		    log_module( LOG_INFO,"READ_SPECIFY_SIZE","READ 0 BYTE FROM CLIENT ERROR:%s",strerror(errno));
		    log_module( LOG_DEBUG,"READ_SPECIFY_SIZE","+++++DONE+++++");
                 return 0; // it indicates the camera has  stoped to push stream or unknown error occurred
	     }
        }
        
        left_bytes -= received_bytes;
        if( left_bytes == 0 )
            break;
        buffer_forward   += received_bytes;
    }

    log_module( LOG_DEBUG,"READ_SPECIFY_SIZE","+++++DONE+++++");	
    return ( total_bytes-left_bytes );
}



int read_specify_size2( int fd, void *buffer, size_t total_bytes)
{
    size_t  left_bytes;
    int received_bytes;
    uint8_t *buffer_forward;
    buffer_forward = (uint8_t *)buffer;
    left_bytes = total_bytes;
    while ( true )
    {
        if ( (received_bytes = read(fd, buffer_forward, left_bytes)) <= 0)
        {
            if (  received_bytes < 0 )
            {
                if( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK )
                {
                    received_bytes = 0;
                }
                else
                {
                    log_module( LOG_ERROR, "READ_SPECIFY_SIZE2", "SOCKET READ ERROR OCCURRED:%s", strerror(errno) );
                    return 0;
                }
            }
            else
            {
                log_module( LOG_INFO,"READ_SPECIFY_SIZE2","READ 0 BYTE FROM CLIENT ERROR MSG:%s",strerror(errno));
                return 0; // it indicates the camera has  stoped to push stream or unknown error occurred
            }
        }
        
        left_bytes -= received_bytes;
        if( left_bytes == 0 )
            break;
        buffer_forward   += received_bytes;
    }
    return (total_bytes-left_bytes);
}

/*
#@args:
# @returns:
*/
ssize_t   write_specify_size(int fd, const void *buffer, size_t total_bytes)
{
    size_t      left_bytes;
    ssize_t     sent_bytes;
    const uint8_t *buffer_forward= (const uint8_t *)buffer;
    left_bytes = total_bytes;
    while ( true )
    {
        if ( (sent_bytes = write( fd, buffer_forward, left_bytes)) <= 0)
        {
            if ( sent_bytes < 0 )
            {
                if( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK )
                {
                    log_module( LOG_DEBUG, "WRITE_SPECIFY_SIZE", "EAGAIN OR EINTER OCCURRED, JUST WAITING FOR THE NEXT");
                    return total_bytes-left_bytes;
                }
                else
                {
                    log_module( LOG_ERROR, "WRITE_SPECIFY_SIZE", "SOCKET WRITE ERROR OCCURRED:%s", strerror(errno) );
                    return -1;
                }
            }
            else
            {
                log_module( LOG_INFO, "WRITE_SPECIFY_SIZE", "WRITE ZERO BYTE, JUST WAITING FOR THE NEXT");
                return total_bytes-left_bytes;
            }

        }
        
        left_bytes -= sent_bytes;
        if( left_bytes == 0 )
            break;
        buffer_forward  += sent_bytes;
    }
    return(total_bytes);
}


/*
#@args:
# @returns:
*/
ssize_t   write_specify_size2(int fd, const void *buffer, size_t total_bytes)
{
    size_t      left_bytes;
    ssize_t     sent_bytes;
    const uint8_t *buffer_forward= (const uint8_t *)buffer;
    left_bytes = total_bytes;
    while ( true )
    {
        if ( (sent_bytes = write( fd, buffer_forward, left_bytes ) ) <= 0)
        {
            if ( sent_bytes < 0 )
            {
                if( errno == EINTR  || errno == EAGAIN || errno == EWOULDBLOCK )
                {
                    sent_bytes = 0;
                }
                else
                {
                    log_module( LOG_ERROR, "WRITE_SPECIFY_SIZE2", "SOCKET WRITE ERROR OCCURRED:%s", strerror(errno) );
                    return -1;
                }
            }
            else
            {
                sent_bytes = 0;
            }

        }
        
        left_bytes -= sent_bytes;
        if( left_bytes == 0 )
            break;
        buffer_forward += sent_bytes;
    }
    return(total_bytes);
}


/*
#@desc:set the file descriptor blocking
*/
int sdk_set_blocking(int sd)
{
    int flags;
    flags = fcntl(sd, F_GETFL, 0);
    if (flags < 0)
    {
        return flags;
    }
    return fcntl( sd, F_SETFL, flags & ~O_NONBLOCK );
}


/*
#@desc:set the file descriptor none blocking
*/
int sdk_set_nonblocking( int sd )
{
    int flags;
    flags = fcntl(sd, F_GETFL, 0);
    if (flags < 0)
    {
        return flags;
    }
    return fcntl(sd, F_SETFL, flags | O_NONBLOCK);
}


/*
#@desc:set the socket file descriptor keepalive property
*/
void  sdk_set_keepalive(int sd)
{
    int keepalive =1;//开启keepalive属性
    int keepidle =30;//如该连接在60秒内没有任何数据往来,则进行探测
    int keepinterval =5;//探测时发包的时间间隔为5 秒
    int keepcount =3;//探测尝试的次数。如果第1次探测包就收到响应了,则后2次的不再发。
    setsockopt(sd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive ,sizeof(keepalive ));
    setsockopt(sd, SOL_TCP, TCP_KEEPIDLE, (void*)&keepidle ,sizeof(keepidle ));
    setsockopt(sd, SOL_TCP, TCP_KEEPINTVL, (void*)&keepinterval ,sizeof(keepinterval ));
    setsockopt(sd, SOL_TCP, TCP_KEEPCNT, (void*)&keepcount ,sizeof(keepcount ));
}

/*
 *  * Disable Nagle algorithm on TCP socket.
 *   *
 *    * This option helps to minimize transmit latency by disabling coalescing
 *     * of data to fill up a TCP segment inside the kernel. Sockets with this
 *      * option must use readv() or writev() to do data transfer in bulk and
 *       * hence avoid the overhead of small packets.
 *        */
int sdk_set_tcpnodelay( int sd )
{
    int nodelay;
    socklen_t len;
    nodelay = 1;
    len = sizeof(nodelay);
    return setsockopt(sd, IPPROTO_TCP, TCP_NODELAY, &nodelay, len);
}


int sdk_set_sndbuf(int sd, int size)
{
    socklen_t len;

    len = sizeof(size);

    return setsockopt(sd, SOL_SOCKET, SO_SNDBUF, &size, len);
}

int sdk_set_rcvbuf(int sd, int size)
{
    socklen_t len;

    len = sizeof(size);

    return setsockopt(sd, SOL_SOCKET, SO_RCVBUF, &size, len);
}


int sdk_get_sndbuf(int sd)
{
    int status, size;
    socklen_t len;

    size = 0;
    len = sizeof(size);

    status = getsockopt(sd, SOL_SOCKET, SO_SNDBUF, &size, &len);
    if (status < 0) {
        return status;
    }

    return size;
}

int sdk_get_rcvbuf(int sd)
{
    int status, size;
    socklen_t len;

    size = 0;
    len = sizeof(size);

    status = getsockopt(sd, SOL_SOCKET, SO_RCVBUF, &size, &len);
    if (status < 0) {
        return status;
    }
    return size;
}



int sdk_set_rcvlowat( int sd, int recvlow )
{
    return setsockopt(sd, SOL_SOCKET, SO_RCVLOWAT,&recvlow, sizeof(recvlow));
}


std::string get_peer_info( int sock_fd , int flag )
{
    std::ostringstream OSS_peer_info;
    if( sock_fd < 0 )    
    return OSS_peer_info.str();
    struct sockaddr_in peer_addr;
    socklen_t sock_len = sizeof( peer_addr );
    int ret = getpeername( sock_fd, (struct sockaddr *)&peer_addr, &sock_len );
    if( ret != 0 )
    return OSS_peer_info.str();

    char *ip = inet_ntoa( peer_addr.sin_addr );
    int port = htons( peer_addr.sin_port );

    if( flag == 0 )
    {
	OSS_peer_info<<ip;
    }
    else if( flag == 1 )
    {
	OSS_peer_info<<port;
    }
    else if( flag == 2 )
    {
	OSS_peer_info<<ip<<":"<<port;
    }

    return OSS_peer_info.str();
}



