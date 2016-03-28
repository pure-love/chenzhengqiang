#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define MAXSIZE 1024

typedef struct 
{
	unsigned short token;
	unsigned short port;
	unsigned int ip;
}clientinfo;

//void parsemessage(char* buf,int fd,struct sockaddr_in *from);

int main(int argc,char *argv[])
{
	unsigned short port;
	if(argc != 2)
	{
		printf("%s : portnumber\n",argv[0]);
		exit(0);
	}
	if((port =atoi(argv[1]))<0)
	{	
		printf("illegal portnumber\n");
		exit(-1);
	}
	unsigned short token =0x1111 ;
	int fd;
	fd = socket(PF_INET,SOCK_DGRAM,0);
	if(fd == -1)perror("create failed!"),exit(-1);
	struct sockaddr_in cli_addr;
	struct sockaddr_in ser_addr;
	struct sockaddr_in from_addr;
	struct sockaddr_in to_addr;
	to_addr.sin_family = PF_INET;
	from_addr.sin_family = PF_INET;
	memset(&from_addr,0,sizeof(from_addr));

	int cli_len = sizeof(cli_addr);
	cli_addr.sin_family = PF_INET;
	cli_addr.sin_port = htons(port);
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(fd,(struct sockaddr*)&cli_addr,cli_len)<0)
	{
		perror("bind failed!1");
		exit(0);
	}
 	else
	printf("bind %d success!\n",port);	
	int ser_len = sizeof(ser_addr);
	ser_addr.sin_family = PF_INET;
	ser_addr.sin_port = htons(48748);
	ser_addr.sin_addr.s_addr = inet_addr("112.74.113.158");
	char buf[MAXSIZE]={0};
	memcpy(buf,&token,2);
	int from_len = sizeof(from_addr);
	if(sendto(fd,buf,2,0,(struct sockaddr*)&ser_addr,ser_len)<0)
	{
		perror("send failed!1");
		exit(0);
	}
//	else
//	printf("into while\n");
	while(1)
	{
		printf(" wait server message!\n");
		if(recvfrom(fd,buf,MAXSIZE,0,(struct sockaddr*)&from_addr,&from_len)<0)
		{
			perror("recv failed!");
			exit(1);
		}
//		printf("hehe1");
//		parsemessage(buf,fd,&from_addr);
//		
		unsigned short token1 = 0x0000;
		memcpy(&token1,buf,2);
		token1 = ntohs(token1);
 		if( token1 == 0x1111)
		{
//			clientinfo ui;
//        		clientinfo *p;
//			memcpy(&ui,buf,8);
			unsigned short port1;
			unsigned int ip1;
			memcpy(&port1,buf+2,2);
			memcpy(&ip1,buf+4,4);
			struct sockaddr_in addr;
			int addr_len = sizeof(addr);
			addr.sin_family = PF_INET;
			addr.sin_port = port1;
 			addr.sin_addr.s_addr = ip1;
			to_addr.sin_port = port1;
			to_addr.sin_addr.s_addr = ip1;
			printf("begin connecting!\n");
			if(sendto(fd,"",0,0,(struct sockaddr*)&addr,addr_len)<0) 
			{
				perror("send failed");
				exit(0);
			}
				
			break;
		}
		else
		{
//			printf("hello123456");
			printf("receive message from %s:%s",inet_ntoa(from_addr.sin_addr), from_addr.sin_port);
		}	
	}
//	printf("hello");
	fd_set read;
	int fn;
	while(1)
	{
//		printf("hello");
		memset(buf,0,MAXSIZE);
		FD_ZERO(&read);
		FD_SET(fd,&read);
		FD_SET(0,&read);
		fn = select(fd+1,&read,NULL,NULL,NULL);
		if(fn == -1)
		{
			perror("select error!");
			exit(1);
		}
		else if(fn>0)
		{
			if(FD_ISSET(0,&read))
			{
				if(fgets(buf,MAXSIZE,stdin)== NULL)
				{
					printf(" too many characters!\n");
					continue;
				}
				if(sendto(fd,buf,strlen(buf),0,(struct sockaddr*)&to_addr,sizeof(to_addr))<0)
				{
					perror("send failed!3");
					continue;
				}
			}
			if(FD_ISSET(fd,&read))
			{
				if(recvfrom(fd,buf,MAXSIZE,0,(struct sockaddr*)&from_addr,&from_len)<0)
				{
					perror("receive failed!");
					continue;			
				}
				else
				{
					printf("says: from %s:%d\n", inet_ntoa(from_addr.sin_addr), ntohs(from_addr.sin_port));
				}
			}
		}
		
	}

	close(fd);
	return 0;
}
