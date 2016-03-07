/*
*@filename:tulip2pclient.cpp
*@author:chenzhengqiang
*@start date:2016/02/21 15:11:34
*@modified date:
*@desc: unit test for tesing tulip2p server
*/

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <iostream>
#include <cstdlib>
using namespace std;

static const char *LocalIP = "192.168.0.110";
static const int LocalPort = 23456;
static const char *ServerIP="192.168.0.110";
static const int ServerPort = 48748;

static const char *Ping= "PING / TULIP2P/2016.2.21\r\n\r\n";
static const char *Commit= "COMMIT /b.txt TULIP2P/2016.2.21\r\n\r\n";
static const char *FileData="TULIP2P/2016.2.21\r\nContent-Type:text/plain\r\nContent-Length:4\r\n\r\nb.txt";

int main( int argc, char ** argv )
{
	struct sockaddr_in localAddr,serverAddr, peerAddr;
      	int serverFd;
      	bzero(&localAddr, sizeof(localAddr));

      	localAddr.sin_family = AF_INET;
      	localAddr.sin_addr.s_addr = inet_addr(LocalIP);
      	localAddr.sin_port = htons(LocalPort);
		
	serverAddr.sin_family = AF_INET;
      	serverAddr.sin_addr.s_addr = inet_addr(ServerIP);
      	serverAddr.sin_port = htons(ServerPort);	

       serverFd = socket(AF_INET, SOCK_DGRAM, 0);
       bind(serverFd, (struct sockaddr *)&localAddr, sizeof(localAddr));

	char buffer[1024]={0};
	ssize_t receivedBytes = 0;
	socklen_t peerAddrLen = sizeof(peerAddr);
	
	sendto(serverFd, Ping, strlen(Ping), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	receivedBytes = recvfrom(serverFd, buffer, sizeof(buffer), 0, 
                			(struct sockaddr *)&(peerAddr), &peerAddrLen );
	cout<<"REPLY:"<<buffer<<endl;

	
	sendto(serverFd, Commit, strlen(Commit), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	bzero(buffer, sizeof(buffer));
	recvfrom(serverFd, buffer, sizeof(buffer), 0, 
                			(struct sockaddr *)&(peerAddr), &peerAddrLen );
	cout<<"REPLY:"<<buffer<<endl<<endl;


	cout<<"WAITING FOR THE DIG MESSAGE FROM OTHERS:......"<<endl;
	bzero(buffer, sizeof(buffer));
	bzero(&peerAddr, sizeof(peerAddr));
	recvfrom(serverFd, buffer, sizeof(buffer), 0, 
                			(struct sockaddr *)&(peerAddr), &peerAddrLen );
	cout<<"DIG MESSAGE:"<<buffer<<endl<<endl;
	
	//it should receive the transfer request from tulip2p server
	cout<<"WAITING FOR THE TRANSFER MESSAGE FROM SERVER:......"<<endl;
	bzero(buffer, sizeof(buffer));
	bzero(&peerAddr, sizeof(peerAddr));
	recvfrom(serverFd, buffer, sizeof(buffer), 0, 
                			(struct sockaddr *)&(peerAddr), &peerAddrLen );
	cout<<"TRANSFER MESSAGE:"<<buffer<<endl;
	char *curPos = strstr(buffer, "Nat-Address:");
	char *prevPos = curPos;
	if ( prevPos != 0 )	
	{
		prevPos = prevPos+12;
		curPos = strstr(prevPos, "\r\n");
		if ( curPos != 0 )
		{
			char digAddr[30]={0};
			strncpy(digAddr, prevPos, curPos-prevPos);
			cout<<"DIG ADDR:"<<digAddr<<endl;
			curPos = strchr(digAddr, ':');
			char digIP[16]={0};
			strncpy(digIP, digAddr,curPos-digAddr);
			cout<<"DIG IP:"<<digIP<<endl;
			int digPort = atoi(curPos+1);
			cout<<"DIG PORT:"<<digPort<<endl;

			peerAddr.sin_family = AF_INET;
      			peerAddr.sin_addr.s_addr = inet_addr(digIP);
      			peerAddr.sin_port = digPort;

			sendto(serverFd, FileData, strlen(FileData), 0, (struct sockaddr *)&peerAddr, sizeof(peerAddr));
			
		}
		
	}
	return 0;  
}
