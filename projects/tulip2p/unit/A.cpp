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
static const int LocalPort = 12345;
static const char *ServerIP="192.168.0.110";
static const int ServerPort = 48748;

static const char *Ping= "PING / TULIP2P/2016.2.21\r\n\r\n";
static const char *Commit= "COMMIT /a.txt TULIP2P/2016.2.21\r\n\r\n";
static const char *Query = "QUERY /b.txt TULIP2P/2016.2.21\r\n\r\n";
static const char *Transfer = "TRANSFER /b.txt TULIP2P/2016.2.21\r\n\r\n";;
static const char *Balabala = "balabalabala";

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
	cout<<"REPLY:"<<buffer<<endl;


	bzero(buffer, sizeof(buffer));
	sendto(serverFd, Query, strlen(Query), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	receivedBytes = recvfrom(serverFd, buffer, sizeof(buffer), 0, 
                			(struct sockaddr *)&(peerAddr), &peerAddrLen );
	cout<<"REPLY:"<<buffer<<endl;

	char *curPos = strstr(buffer, "Nat-Address:");
	char *prevPos = curPos;
	if ( prevPos != 0 )	
	{
		prevPos = prevPos+12;
		curPos = strstr(prevPos, "\r\n");
		if ( curPos != 0 )
		{
			char resourceAddr[30]={0};
			strncpy(resourceAddr, prevPos, curPos-prevPos);
			cout<<"RESOURCE ADDR:"<<resourceAddr<<endl;
			curPos = strchr(resourceAddr, ':');
			char resourceIP[16]={0};
			strncpy(resourceIP, resourceAddr,curPos-resourceAddr);
			cout<<"RESOURCE IP:"<<resourceIP<<endl;
			int resourcePort = atoi(curPos+1);
			cout<<"RESOURCE PORT:"<<resourcePort<<endl;

			//dig hole to the remote first
			//so the remote client can send file to me
			peerAddr.sin_family = AF_INET;
      			peerAddr.sin_addr.s_addr = inet_addr(resourceIP);
      			peerAddr.sin_port = resourcePort;

			sendto(serverFd, Balabala, strlen(Balabala), 0, (struct sockaddr *)&peerAddr, sizeof(peerAddr));
			sendto(serverFd, Transfer, strlen(Transfer), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
			cout<<"WAITING FOR THE RESOURCE MACHINE SEND THE FILE......"<<endl;
			bzero(buffer, sizeof(buffer));
			receivedBytes = recvfrom(serverFd, buffer, sizeof(buffer), 0, 
                			(struct sockaddr *)&(peerAddr), &peerAddrLen );
			
			cout<<"THE FILE'S DATA:"<<buffer<<endl;
			
		}
		
	}
	return 0;  
}
