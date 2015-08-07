/*
@author:chenzhengqiang
@start date:2015/8/7
@modified date:
@desc:simple implementation of udp's cross for centos 
*/

static const int ARGS_COUNT = 3;
static const int BUF_SIZE = 1024;
#define IP (argv[1])
#define PORT (argv[2])

#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/types.h>

int main( int argc, char ** argv )
{
    (void)argc;
    (void)argv;
    if( argc != ARGS_COUNT )
    int udp_sockfd = socket(AF_INET,SOCK_DGRAM,0);
    struct sockaddr_in local_addr;
    memset(&local_addr,0,sizeof(local_addr));
    local_addr.sin_addr.s_addr=inet_aton(IP);
    local_addr.sin_port = htons(PORT);
    local_addr.sin_family = AF_INET;

    
    bind(udp_sockfd,(struct sockaddr *)&local_addr,sizeof(local_addr));

    struct sockaddr_in client_addr1;
    socklen_t client_addr_len1 = sizeof(client_addr1);
    char recv_buf1[BUF_SIZE];
    recvfrom(udp_sockfd,recv_buf1,BUF_SIZE,0,(struct sockaddr *)&client_addr1,&client_addr_len1);

    struct sockaddr_in client_addr2;
    socklen_t client_addr_len2 = sizeof(client_addr2);
    char recv_buf2[BUF_SIZE];
    recvfrom(udp_sockfd,recv_buf2,BUF_SIZE,0,(struct sockaddr *)&client_addr1,&client_addr_len1);
}
