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
#include "database_operation.hpp"

#define PORT 1234
#define BACKLOG_SIZE 10

using namespace std;

void createSocket(int &socketDescriptor)
{
	if((socketDescriptor = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error while creating socket");
		exit(EXIT_FAILURE);
	}
}

void setServerInformation(struct sockaddr_in &server)
{
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = htons(INADDR_ANY);
}

void bindServer(int &socketDescriptor, struct sockaddr_in * server)
{
	if(bind(socketDescriptor, (struct sockaddr *)server, sizeof(struct sockaddr)) == -1)
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

bool acceptClient(int &client, int &socketDescriptor, struct sockaddr_in * from)
{
	unsigned int size = sizeof(struct sockaddr);
	if((client = accept(socketDescriptor, (struct sockaddr *)from, &size)) == -1)
	{
		perror("Accept error");
		return false;
	}
	return true;
}

void * solveRequest(void * args)
{
	DatabaseQuery * parameters = (DatabaseQuery *)args;
	MYSQL * database = parameters->getDatabase();
	MYSQL_RES * queryResult;
	MYSQL_ROW resultRow;
	int client = *parameters->getClient();
	unsigned int fieldsCount;
	queryResult = query(database, "select sysdate() from dual");
	fieldsCount = mysql_num_fields(queryResult);
	while(resultRow = mysql_fetch_row(queryResult))
	{
		for(unsigned int index = 0; index < fieldsCount; ++index)
		{
			if(write(client, resultRow[index], sizeof(resultRow[index])) == -1 || write(client, "\n" , 1) == -1)
				perror("Write error");
		}
	}
}

int main()
{
	int socketDescriptor;
	struct sockaddr_in server, from;
	MYSQL * databaseConnection;
	createSocket(socketDescriptor);
	setServerInformation(server);
	bindServer(socketDescriptor, &server);
	listenSocket(socketDescriptor);
	connectToDatabase(databaseConnection);
	while(true)
	{
		int clientSocket;
		pthread_t thread;
		cout << "Waiting for a client to connect..." << endl;
		if(!acceptClient(clientSocket, socketDescriptor, &from))
			continue;
		DatabaseQuery threadParameters(databaseConnection, &clientSocket);
		pthread_create(&thread, NULL, solveRequest, (void *)&threadParameters);
		pthread_detach(thread);
	}
	return 0;
}
