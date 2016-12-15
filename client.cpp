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

using namespace std;

#define SERVENT_PORT 2048
#define BACKLOG_SIZE 10
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1234
#define SIGN_IN_ERROR 4
#define SIGN_IN_SUCCESS 5

void readError()
{
	perror("Read Error");
	exit(EXIT_FAILURE);
}

void writeError()
{
	perror("Write Error");
	exit(EXIT_FAILURE);
}

int getUserOption()
{
	int option;
	do
	{
		cout << "Wrong option" << endl;
		cout << "1. Sign up" << endl;
		cout << "2. Sign in" << endl;
		cout << "Select 1 or 2..." << endl;
		cin >> option;
	}while(option != 1 && option != 2);
	return option;
}

void createSocket(int &client)
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

void bindClientServer(int &servent, struct sockaddr_in &clientServer)
{
	if(bind(servent, (struct sockaddr *)&clientServer, sizeof(struct sockaddr)) == -1)
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

void connectToServer(int &servent, struct sockaddr_in &server)
{
	if(connect(servent, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
	{
		perror("Connect error");
		exit(EXIT_FAILURE);
	}
}

int main()
{
	int option, servent, socketDescriptor;
	sockaddr_in superServer, clientServer;

	setSuperServerInfo(superServer);
	setClientServerInfo(clientServer);
	createSocket(servent);
	createSocket(socketDescriptor);
	bindClientServer(servent, clientServer);
	listenServent(servent);
	option = getUserOption();
	connectToServer(socketDescriptor, superServer);
	if(write(socketDescriptor, &option, 4) == -1)
		writeError();
	if(option == 1)
		signUpProcedure(socketDescriptor);
	else
		signInProcedure();
	return 0;
}


