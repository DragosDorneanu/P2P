/*
 * client.cpp
 *
 *  Created on: Dec 15, 2016
 *      Author: dragos
 */

#include <iostream>
#include <sys/socket.h>
#include <cstdio>
#include <cstdlib>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "SignInClientProcedure.hpp"
#include "SignUpClientProcedure.hpp"

#define SERVENT_PORT 0
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define BACKLOG_SIZE 20

using namespace std;

int getUserOption()
{
	int option;
	do
	{
		cout << "1. Sign up" << endl;
		cout << "2. Sign in" << endl;
		cout << "Select 1 or 2..." << endl;
		cin >> option;
	}while(option != 1 && option != 2);
	return option;
}

void createTCPSocket(int &client)
{
	if((client = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error while creating socket...");
		exit(EXIT_FAILURE);
	}
}

void setSuperServerInfo(sockaddr_in &superServer)
{
	superServer.sin_family = AF_INET;
	superServer.sin_port = htons(SERVER_PORT);
	superServer.sin_addr.s_addr = inet_addr(SERVER_IP);
}

void setClientServerInfo(sockaddr_in &clientServer)
{
	clientServer.sin_family = AF_INET;
	clientServer.sin_port = htons(SERVENT_PORT);
	clientServer.sin_addr.s_addr = htonl(INADDR_ANY);
}

void bindClientServer(int &servent, sockaddr_in &clientServer)
{
	if(bind(servent, (sockaddr *)&clientServer, sizeof(clientServer)) == -1)
	{
		perror("Bind error");
		exit(EXIT_FAILURE);
	}
}

void listenServent(int &servent)
{
	if(listen(servent, BACKLOG_SIZE) == -1)
	{
		perror("Listen error");
		exit(EXIT_FAILURE);
	}
}

void connectToServer(int &client, sockaddr_in &server)
{
	if(connect(client, (sockaddr *)&server, sizeof(sockaddr)) == -1)
	{
		perror("Connect error");
		exit(EXIT_FAILURE);
	}
}

uint16_t getServentPort(int &servent, sockaddr_in clientServer)
{
	unsigned int length = sizeof(clientServer);
	if(getsockname(servent, (sockaddr *)&clientServer, &length) == -1)
	{
		perror("Error while getting servent port");
		exit(EXIT_FAILURE);
	}
	return clientServer.sin_port;
}

int main()
{
	int option, servent, requestSocket, socketDescriptor, enableReuse = 1;
	sockaddr_in superServer, clientServer;
	uint16_t serventPort;

	setSuperServerInfo(superServer);
	setClientServerInfo(clientServer);
	createTCPSocket(socketDescriptor);
	createTCPSocket(servent);
	createTCPSocket(requestSocket);

	setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &enableReuse, 4);
	setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEPORT, &enableReuse, 4);
	setsockopt(servent, SOL_SOCKET, SO_REUSEADDR, &enableReuse, 4);
	setsockopt(servent, SOL_SOCKET, SO_REUSEPORT, &enableReuse, 4);
	setsockopt(requestSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, 4);
	setsockopt(requestSocket, SOL_SOCKET, SO_REUSEPORT, &enableReuse, 4);

	bindClientServer(servent, clientServer);
	listenServent(servent);
	option = getUserOption();
	connectToServer(socketDescriptor, superServer);

	if(write(socketDescriptor, &option, 4) == -1)
		writeError();
	if(option == 1)
		signUpProcedure(socketDescriptor);
	else
	{
		serventPort = getServentPort(servent, clientServer);
		if(write(socketDescriptor, &serventPort, sizeof(uint16_t)) == -1)
			writeError();
		signInProcedure(socketDescriptor, servent, requestSocket);
	}
	close(socketDescriptor);
	return 0;
}


