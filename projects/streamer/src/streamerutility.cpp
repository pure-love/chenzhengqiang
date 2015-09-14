/*
 *@Author:chenzhengqiang
 *@company:swwy
 *@date:2013/3/26
 *@modified:
 *@desc:
 @version:1.0
 */


#include "streamerutility.h"
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
    if((listen_fd = socket(AF_INET,SOCK_STREAM,0)) < 0)
    {
        perror("FAILED TO CREATE SOCKET");
        exit(EXIT_FAILURE);
    }

    int ret=setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&REUSEADDR_ON,sizeof(REUSEADDR_ON));
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
*@args
*@returns
*@desc:
*/
void print_welcome(CONFIG & config)
{
    char heading[1024];
    snprintf(heading,1024,"\n" \
                          "\r    *\n"\
                          "\r  *   *  *      *     *  *     *     *  *       *\n" \
                          "\r *        *     *    *    *    *    *   *       *\n"\
                          "\r   *       *    *   *      *   *   *     *      *\n"\
                          "\r      *      * * * *        * * * *       * *  *\n"\
                          "\r  *   *       *   *          *   *             *\n"\
                          "\r    *                                         *\n"\
                          "\r                                             *"\
                          "\n\nsoftware version 1.7.1, Copyleft (c) 2015 SWWY\n" \
                          "this streaming server started on March 26 2015  with gcc 4.4.7 in CentOS\n" \
                          "programed by chenzhengqiang,based on http protocol, which registered at port:%d\n\n" \
                          "now streaming server start listening :......\n\n\n\n\n",config.streamer_port);

    std::cout<<heading<<std::endl;
}



/*
 *@args:
 *@returns:
 *@desc:
 */
void print_error(void)
{
    std::cerr<<"\r\nShort for options,you must enter at least one like the following:"<<std::endl;
    std::cerr<<"\r:streamer -h  or  ./streamer -v"<<std::endl;
    std::cerr<<"\r:streamer -f <config_file>";
    exit(EXIT_FAILURE);
}



/*
 *@args:
 *@returns:
 *@desc:
 */
void print_help( void )
{
    printf("\rUsage:streamer [OPTION]>...[OPTION]...\n");
    printf("\rStartup the streaming server or list information about it:version,help info etc.\n");
    printf("\rMandatory arguments to long options are mandatory for short options too.\n");
    printf("\r%-20s%s","  -f, --config-file","specify the config file\n");
    printf("\r%-20s%s","  -h, --help","display help and exit\n");
    printf("\r%-20s%s","  -v, --version","display version and exit\n");
    printf("\r\nExample:\n");
    printf("\rstreamer  //startup this streaming server through default config file\n");
    printf("\rstreamer -h\n");
    printf("\rstreamer -v\n");
    printf("\rstreamer --config-file /etc/streamer/streamer.conf\n");
    exit(EXIT_SUCCESS);
}


/*
 *@args:
 *@returns:
 *@desc:
 */

void print_version( const char * version )
{
    printf("\rStreamer Version:%s\n",version);
    printf("\rCopyright (C) 2015 SWWY,Inc.\n");
    printf("Started on March 26 2015  with gcc 4.4.7 (CentOS 6.5).Written By ChenZhengQiang.\n");
    exit(EXIT_SUCCESS);
}


/*
 *@args:
 *@returns:
 *@desc:
 */
void handle_cmd_options( int ARGC, char * * ARGV, struct CMD_OPTIONS & cmd_options )
{
    (void)ARGC;
    cmd_options.need_print_version = false;
    cmd_options.need_print_help = false;
    cmd_options.run_as_daemon = false;
    cmd_options.config_file="";

    ++ARGV;
    while( *ARGV != NULL )
    {
        if((*ARGV)[0]=='-')
        {
            if( strcmp( *ARGV,"-v") == 0 || strcmp(*ARGV,"--version") == 0 )
            {
                cmd_options.need_print_version = true;
            }
            else if( strcmp(*ARGV,"-h") ==0 || strcmp(*ARGV,"--help") == 0 )
            {
                cmd_options.need_print_help = true;
            }
            else if( strcmp(*ARGV,"-d") == 0 || strcmp(*ARGV,"--daemon") == 0 )
            {
                cmd_options.run_as_daemon = true;
            }
            else if( strcmp(*ARGV,"-f") == 0 || strcmp(*ARGV,"--config-file") == 0 )
            {
                ++ARGV;
                if( *ARGV == NULL )
                {
                    std::cerr<<"You must enter the config file when you specify the -f or --log-file option"<<std::endl;
                    print_error();
                }
                cmd_options.config_file.assign(*ARGV);
            }
        }
        ++ARGV;
    }
    if( cmd_options.config_file.empty() )
    {
         cmd_options.config_file="/etc/streamer/streamer.conf";
    }    
}



/*
 *@args:config_file[IN],config[OUT]
 *@returns:
 *@desc:read config information from config file
 */
void read_config( const char * config_file, CONFIG & config )
{
    std::ifstream ifile(config_file);
    if( !ifile )
    {
         std::cerr<<"FAILED TO OPEN FILE:"<<config_file<<std::endl;
         exit(EXIT_FAILURE);
    }
    
    std::string line;
    std::string heading,key, value;
    bool is_streamer = false;
    bool is_state = false;
    config.log_level = -1; 
    config.streamer_port = -1; 
    config.state_port = -1;
    config.run_as_daemon = -1;
    
    while(getline(ifile,line))
    {
         key="";
         value="";
         heading="";
         if( line.empty() )
         continue;
         std::string::const_iterator line_iter = line.begin();
         while( line_iter != line.end() )
         {
              if( *line_iter == ' ' )
              {
                   ++line_iter;
                   continue;
              }
              if( *line_iter =='[' )
              {
                  ++line_iter;
                  if( is_streamer )
                  is_streamer = false;
                  if( is_state )
                  is_state = false;
                  
                  while(*line_iter !=']')
                  {
                       heading.push_back(*line_iter);
                       ++line_iter;
                  }
                  if(heading == "streamer")
                  {
                      is_streamer = true;
                  }
                  else if( heading == "state-server" )
                  {
                      is_state = true;
                  }
                  break;
              }
              else if( *line_iter == '#' )
              {
                   break;
              }
              else
              {
                   while( line_iter !=line.end() && *line_iter != '=' )
                   {
                        if( *line_iter == ' ' )
                        {
                             ++line_iter;
                             continue;
                        }
                        key.push_back(*line_iter);
                         ++line_iter;
                   }
                   if( line_iter == line.end() )
                   break;
                   
                   ++line_iter;
                   while( *line_iter == ' ' && line_iter != line.end() )
                   {
                       ++line_iter;
                   }
                   
                   if( line_iter == line.end() )
                   break;
                   
                   while( line_iter !=line.end() )
                   {
                       if( *line_iter == ' ' )
                       break; 
                       value.push_back(*line_iter);
                       ++line_iter;
                   }
                   if(key == "log-dir")
                   {
                       config.log_file=value;
                   }
                   else if( key.compare("log-level") == 0 )
                   {
                       config.log_level =atoi(value.c_str()); 
                   }
                   else if( key.compare("lock-file") == 0 )
                   {
                       config.lock_file.assign(value);
                   }
                   else if( key.compare("version") == 0 )
                   {
                       config.version.assign(value);
                   }
                   else if( key.compare("notify-server-dir") == 0 )
                   {
                       config.notify_server_file=value; 
                   }
                   else if(key.compare("daemon") == 0)
                   {
                      config.run_as_daemon = atoi(value.c_str());
                   }
                   else if( key.compare("bind-address") == 0 )
                   {
                       if( is_streamer )
                       {
                            config.streamer_ip = value; 
                       }
                       else if( is_state )
                       {
                            config.state_ip = value; 
                       }
                   }
                   else if( key.compare("port") == 0 )
                   {
                       if( is_streamer )
                       {
                            config.streamer_port = atol(value.c_str()); 
                       }
                       else if( is_state )
                       {
                            config.state_port = atol(value.c_str()); 
                       }
                   }
               break;   
              }
           ++line_iter;   
         }
         line="";
    }
    if( config.log_file.empty()   || config.streamer_ip.empty() || config.lock_file.empty() || 
        config.state_ip.empty()  || config.log_level == -1       || 
        config.streamer_port == -1 || config.state_port == -1 ||
        config.run_as_daemon == -1 )
    {
         std::cerr<<"INVALID CONFIG FILE:"<<config_file<<std::endl;
         exit(EXIT_FAILURE);
    }
}



/*
 *@args:config_file[IN],config[OUT]
 *@returns:
 *@desc:read config information from config file
 */
void read_config2( const char * config_file,std::map<std::string, int> & notify_server_pool )
{
      std::ifstream ifile(config_file);
      if( !ifile )
     {
           log_module(LOG_INFO,"READ_CONFIG2","FAILED TO OPEN FILE:%s--%s",config_file,strerror(errno));
           return;
     }
     std::string line;
     std::string IP;
     std::string PORT;
     while(getline(ifile,line))
     {
          IP="";
          PORT="";
          if(line.empty())
          continue;
          std::string::const_iterator line_iter = line.begin();
          while( line_iter != line.end() )
          {
               if( *line_iter == '#' )
               break;
               while( *line_iter == ' ' && line_iter != line.end() )
               {
                     ++line_iter;
               }
               if( line_iter == line.end() )
               break;
               while( *line_iter !=':' && line_iter != line.end() )
               {
                   IP.push_back(*line_iter);
                   ++line_iter;
               }
               if( line_iter == line.end() )
               break;
               ++line_iter;
               while( line_iter != line.end() && *line_iter == ' ')
               {
                   ++line_iter;
               }
               while( line_iter != line.end() )
               {
                    if( *line_iter == ' ' )
                    break;    
                    PORT.push_back(*line_iter);
                    ++line_iter;
               }
               notify_server_pool.insert(std::make_pair(IP,atol(PORT.c_str())));
               break;
          }
     }
}



/*
 *@args:
 *@returns:
 *@desc:
 */
void get_src_info( const std::string & src, string & IP, int & PORT)
{
    std::string host;
    std::string port="";
    char sep = ':';
    std::string::const_iterator str_iter = src.begin();
    while( str_iter != src.end() && *str_iter != sep )
    {
        host.push_back(*str_iter);
        ++str_iter;
    }
    if( str_iter != src.end() )
    {
        ++str_iter;
        while( str_iter != src.end() )
        {
            port.push_back(*str_iter);
            ++str_iter;
        }
    }
    if(port.empty())
    {
        PORT=80;
    }
    else
    {
        PORT = atol(port.c_str());
    }
    IP=host;
}




/*
#@args:
#@returns:0 ok,-1 error or it indicates that clients has close the socket
          others indicates try again
#@desc:
*/
int  read_http_header(int fd, void *buffer, size_t buffer_size )
{
    #define JUDGE_RETURN(X) \
    if( (X) <= 0 )\
    {\
           if( errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK )\
           {\
               (X) = 0;\
           }\
           else\
           {\
                log_module(LOG_DEBUG,"READ_HTTP_HEADER","READ BYTES:%d READ ERROR:%s",(X),strerror(errno));\
                return 0;\
           }\
     }\
     else\
     return 0;  
              
    int received_bytes;
    size_t left_bytes = 0;
    size_t read_bytes = 0;
    char *buffer_forward = (char *)buffer;
    bool read_http_header_done = false;
    
    while(  true )
    {
         if( (received_bytes = read( fd,buffer_forward,sizeof(char))) <= 0 )
         {
              JUDGE_RETURN(received_bytes);
         }
         
         read_bytes += received_bytes;
         if( read_bytes > buffer_size )
         {
              log_module(LOG_DEBUG,"READ_HTTP_HEADER","READ_BYTES:%d--%s",read_bytes,LOG_LOCATION);
              return -1;
         }

         if( *buffer_forward == '\0' )
         return read_bytes;

         if( !read_http_header_done && *buffer_forward == '\r' )
         {
              buffer_forward+=received_bytes; 
              left_bytes = 3*sizeof(char);
              while( true )
              {
                   if((received_bytes = read( fd,buffer_forward,left_bytes)) <= 0)
                   {
                        JUDGE_RETURN(received_bytes);
                   }
                   read_bytes+=received_bytes;
                   if( read_bytes > buffer_size )
                   {
                       log_module(LOG_DEBUG,"READ_HTTP_HEADER","READ_BYTES:%d--%s",read_bytes,LOG_LOCATION);
                       return -1;
                   }
                   buffer_forward += received_bytes;
                   left_bytes -= received_bytes;
                   if( left_bytes == 0 )
                   break; 
              }
              if( *--buffer_forward == '\n' )
              {
                    read_http_header_done = true;
                    return read_bytes;
              }
              ++buffer_forward;
         }
         else
         {
              buffer_forward+=received_bytes; 
         }
    }
    return read_bytes;
}


/*
#@args:
# @returns:
*/
int read_specify_size( int fd, void *buffer, size_t total_bytes)
{
    log_module( LOG_DEBUG,"READ_SPECIFY_SIZE","+++++START+++++");	
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
		    if( (total_bytes-left_bytes) <= 0 )
		    continue;		
		    log_module(LOG_DEBUG,"RECEIVE_STREAM_CB","+++++DONE+++++");
                    return (total_bytes-left_bytes);
                }
                else
		{
	 	    log_module(LOG_DEBUG,"READ_SPECIFY_SIZE","READ ERROR %s",strerror(errno));
		    log_module(LOG_DEBUG,"READ_SPECIFY_SIZE","+++++DONE+++++");	
                    return 0;
		}
            }
            else
	    {
		log_module(LOG_DEBUG,"READ_SPECIFY_SIZE","READ 0 BYTE FROM CLIENT:%s",strerror(errno));
		log_module(LOG_DEBUG,"READ_SPECIFY_SIZE","+++++DONE+++++");
                return 0; // it indicates the camera has  stoped to push stream or unknown error occurred
	    }
        }
        left_bytes -= received_bytes;
        if( left_bytes == 0 )
            break;
        buffer_forward   += received_bytes;
    }
    log_module(LOG_DEBUG,"READ_SPECIFY_SIZE","+++++DONE+++++");	
    return (total_bytes-left_bytes);
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
                    return 0;
            }
            else
                return 0; // it indicates the camera has  stoped to push stream or unknown error occurred
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
        if ( (sent_bytes = write(fd, buffer_forward, left_bytes)) <= 0)
        {
            if ( sent_bytes < 0 )
            {
                if( errno == EINTR )
                {
                    sent_bytes = 0;
                }
                else if( errno == EAGAIN || errno == EWOULDBLOCK )
                {
                    if( (total_bytes - left_bytes) == 0)
                    continue;    
                    return total_bytes-left_bytes;
                }
                else
                {
                    return -1;
                }
            }
            else
                return -1;

        }
        left_bytes -= sent_bytes;
        if( left_bytes == 0 )
            break;
        buffer_forward   += sent_bytes;
    }
    return(total_bytes);
    return 0;
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
        if ( (sent_bytes = write(fd, buffer_forward, left_bytes)) <= 0)
        {
            if ( sent_bytes < 0 )
            {
                if( errno == EINTR  || errno == EAGAIN || errno == EWOULDBLOCK )
                {
                    sent_bytes = 0;
                }
                else
                {
                    return -1;
                }
            }
            else
                return -1;

        }
        left_bytes -= sent_bytes;
        if( left_bytes == 0 )
            break;
        buffer_forward   += sent_bytes;
    }
    return(total_bytes);
}



/*
#@args:'header' stands for the http request header
and the 'HTTP_REQUEST_INFO' is a reference used for storing the http header's key-values
#@desc:
save the http's request method,either key-values of url into parameter req_info
e.g: VIWER: GET live.swwy.com/channel=abcd&type=flv&token=JMB23rrx HTTP/1.1\r\n
or : CAMERA: POST live.swwy.com/channel=abcd&type=flv&token=JMB23rrx&src=202-10  HTTP/1.1\r\n
*/
void parse_http_request( char header[], HTTP_REQUEST_INFO & req_info )
{
    string key, value;
    while( *header == ' ' )
    {
        ++header;
    }
    while( *header != ' ' )
    {
        req_info.method.push_back(toupper(*header));
        ++header;
    }
    while( *header == ' ' )
    {
        ++header;
    }
    if( *header == '/' )
    {
        req_info.hostname="";
        ++header;
    }
    else
    {
        while( *header != '/' )
        {
            req_info.hostname.push_back(*header);
            ++header;
        }
        ++header;
    }

    while( *header == ' ' )
    {
        ++header;
    }
    
    while( *header != ' ' )
    {
        while( *header != ' ' && *header != '=')
        {
            key.push_back(*header);
            ++header;
        }
        
        if( *header == ' ' )
        {
            while( *header ==' ')
            {
                ++header;
            }
        }
        else if( *header == '=' )
        ++header;

        if( *header == ' ' )
        {
            while( *header == ' ' )
            {
                ++header;
            }
        }
        
        while( *header != ' ' && *header != '&' )
        {
            value.push_back(*header);
            ++header;
        }

        if( key == "channel" )
        {
            req_info.channel = value;
        }
        else if( key == "type" )
        {
            req_info.type = value;
        }
        else if( key == "token" )
        {
            req_info.token = value;
        }
        else if( key == "src" )
        {
            req_info.src=value;
        }
        if( *header == ' ' )
            break;
        key="";
        value="";
        ++header;
    }
    while( * header == ' ' )
    {
        ++header;
    }
    while( *header )
    {
        while( *header && *header != '\r')
        {
            if( *header == ' ' || *header == '\n')
                break;
            req_info.http_version.push_back(*header);
            ++header;
        }
        if( *header == '\n' || *header == ' ' || *header == '\r' )
            break;
    }
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
