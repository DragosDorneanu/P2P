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

void error()
{
	perror("Error");
	exit(EXIT_FAILURE);
}

void loginFailMessage(int &client)
{
	if(write(client, "Login failed\n", strlen("Login failed\n")) == -1)
		error();
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

void successfulLoginProcedure(MYSQL * database, int &client, struct sockaddr_in clientInfo, MYSQL_ROW id)
{
	updateUserStatus(database, clientInfo, id[0]);
	dropUserAvailableFiles(database, id[0]);
	sendDownloadPathToClient(database, client, id[0]);
	//client must send a list of available files(directory where to look is set at sign up and is called \"DownloadPath\" in Users table)
	insertUserAvailableFiles(database, client, id[0]);
}

void * solveRequest(void * args)
{
	DatabaseQueryParameters * parameters = (DatabaseQueryParameters *)args;
	MYSQL * database = parameters->getDatabase();
	int client = *(parameters->getClient());
	MYSQL_RES * queryResult;
	char username[50], password[50], sqlCommand[1024];
	unsigned int sizeOfUsername, sizeOfPassword;

	if(read(client, &sizeOfUsername, 4) == -1 || read(client, username, sizeOfUsername) == -1 || read(client, &sizeOfPassword, 4) == -1 || read(client, password, sizeOfPassword) == -1)
		error();
	sprintf(sqlCommand, "select id from UsersInfo where username = '%s' and password = '%s'", username, getSHA256Hash(password));
	queryResult = query(database, sqlCommand);
	if(mysql_num_rows(queryResult))
		successfulLoginProcedure(database, client, parameters->getClientInfo(), mysql_fetch_row(queryResult));
	else
		loginFailMessage(client);
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
		DatabaseQueryParameters threadParameters(databaseConnection, &clientSocket, from);
		pthread_create(&thread, NULL, solveRequest, (void *)&threadParameters);
		pthread_detach(thread);
	}
	return 0;
}
