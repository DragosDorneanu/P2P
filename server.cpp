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
#include <openssl/sha.h>
#include "database_operation.hpp"
#include "SignInServerProcedure.hpp"
#include "SignUpServerProcedure.hpp"

#define PORT 1234
#define BACKLOG_SIZE 10
#define SIGN_UP 1
#define SIGN_IN 2

using namespace std;

void readError() {
	perror("Read Error");
}

void writeError() {
	perror("Write Error");
}

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
	server.sin_addr.s_addr = htonl(INADDR_ANY);
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

char * getSHA256Hash(char * str)
{
	char hexPassword[64];
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha;
	SHA256_Init(&sha);
	SHA256_Update(&sha, str, strlen(str));
	SHA256_Final(hash, &sha);
	for(unsigned int index = 0; index < sizeof(hash); ++index)
		sprintf(hexPassword + 2 * index, "%02x", hash[index]);
	return hexPassword;
}

void * solveRequest(void * args)
{
	DatabaseQueryParameters * parameters = (DatabaseQueryParameters *)args;
	int client = *(parameters->getClient());
	int option;
	if(read(client, &option, 4) == -1)
		readError();
	if(option == SIGN_UP)
		signUpServerProcedure(parameters);
	else
		signInServerProcedure(parameters);
}

int main()
{
	int socketDescriptor, enableReuse = 1;
	struct sockaddr_in server, from;
	MYSQL * databaseConnection;

	createSocket(socketDescriptor);
	setServerInformation(server);
	setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEADDR, &enableReuse, 4);
	setsockopt(socketDescriptor, SOL_SOCKET, SO_REUSEPORT, &enableReuse, 4);
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
		DatabaseQueryParameters threadParameters(databaseConnection, &clientSocket, from);
		pthread_create(&thread, NULL, solveRequest, (void *)&threadParameters);
		pthread_detach(thread);
	}
	return 0;
}
