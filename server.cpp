/*
 * server.cpp
 *
 *  Created on: Dec 11, 2016
 *      Author: dragos
 */
#include <iostream>
#include <pthread.h>
#include <sys/socket.h>
#include <cstdlib>
#include <cstdio>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <unordered_set>
#include "SignInServerProcedure.hpp"
#include "SignUpServerProcedure.hpp"
#include "ConnectionDecryptor.hpp"

#define PORT 1234
#define BACKLOG_SIZE 10
#define SIGN_UP 1
#define SIGN_IN 2

using namespace std;

void createSocket(int &socketDescriptor)
{
	if((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error while creating socket");
		exit(EXIT_FAILURE);
	}
}

void setServerInformation( sockaddr_in &server)
{
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
}

void bindServer(int &socketDescriptor,  sockaddr_in * server)
{
	if(bind(socketDescriptor, ( sockaddr *)server, sizeof( sockaddr)) == -1)
	{
		perror("Error while binding server");
		exit(EXIT_FAILURE);
	}
}

void listenSocket(int &socketDescriptor)
{
	if(listen(socketDescriptor, BACKLOG_SIZE) == -1)
	{
		perror("Listen error");
		exit(EXIT_FAILURE);
	}
}

bool acceptClient(int &client, int &socketDescriptor, sockaddr_in * from)
{
	unsigned int size = sizeof(from);
	if((client = accept(socketDescriptor, (sockaddr *)from, &size)) == -1)
	{
		perror("Accept error");
		return false;
	}
	return true;
}

void * solveRequest(void * args)
{
	ClientThreadParameter * parameters = (ClientThreadParameter *)args;
	int option;
	MYSQL * database;

	pthread_detach(pthread_self());
	connectToDatabase(database);

	if(read(parameters->client, &option, 4) == -1)
		readError();
	if(option == SIGN_UP)
		signUpServerProcedure(database, parameters);
	else
		signInServerProcedure(database, parameters);
	close(parameters->client);
	mysql_thread_end();
	return (void *)(NULL);
}

int main()
{
	int socketDescriptor, enableReuse = 1;
	sockaddr_in server;
	MYSQL * databaseConnection;

	createSocket(socketDescriptor);
	setServerInformation(server);
	setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &enableReuse, 4);
	setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEPORT, &enableReuse, 4);
	bindServer(socketDescriptor, &server);
	listenSocket(socketDescriptor);
	connectToDatabase(databaseConnection);
	setAllClientsOffline(databaseConnection);
	mysql_close(databaseConnection);

	while(true)
	{
		int clientSocket;
		sockaddr_in from;
		pthread_t thread;

		cout << "Waiting for a client to connect..." << endl;
		if(!acceptClient(clientSocket, socketDescriptor, &from))
			continue;

		ClientThreadParameter * threadParameters = new ClientThreadParameter(clientSocket, from);
		pthread_create(&thread, NULL, solveRequest, (void *)threadParameters);
	}
	return 0;
}
