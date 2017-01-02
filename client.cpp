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
#define SERVER_IP "86.124.185.69"
#define SERVER_PORT 1234

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

void createUDPSocket(int &servent)
{
	if((servent = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
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

void connectToServer(int &client, sockaddr_in &server)
{
	if(connect(client, (sockaddr *)&server, sizeof(sockaddr)) == -1)
	{
		perror("Connect error");
		exit(EXIT_FAILURE);
	}
}

int main()
{
	int option, servent, socketDescriptor, enableReuse = 1;
	sockaddr_in superServer, clientServer;

	setSuperServerInfo(superServer);
	setClientServerInfo(clientServer);
	createTCPSocket(socketDescriptor);
	createUDPSocket(servent);
	setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &enableReuse, 4);
	setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEPORT, &enableReuse, 4);
	setsockopt(servent, SOL_SOCKET, SO_REUSEADDR, &enableReuse, 4);
	setsockopt(servent, SOL_SOCKET, SO_REUSEPORT, &enableReuse, 4);

	bindClientServer(servent, clientServer);
	option = getUserOption();
	connectToServer(socketDescriptor, superServer);

	if(write(socketDescriptor, &option, 4) == -1)
		writeError();
	if(option == 1)
		signUpProcedure(socketDescriptor);
	else
		signInProcedure(socketDescriptor, servent);
	close(socketDescriptor);
	return 0;
}


