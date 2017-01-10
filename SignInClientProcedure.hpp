/*
 * SignIn.hpp
 *
 *  Created on: Dec 15, 2016
 *      Author: dragos
 */

#ifndef SIGNINCLIENTPROCEDURE_HPP_
#define SIGNINCLIENTPROCEDURE_HPP_

#include <iostream>
#include <fstream>
#include <termios.h>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>

#include "ValidationProcedures.hpp"
#include "FunctionArray.hpp"

#define SIGN_IN_ERROR 4
#define SIGN_IN_SUCCESS 5

using namespace std;

void commandPrompt(FunctionArray * commandArray)
{
	string commandName;
	char command[MAX_COMMAND_SIZE];
	int functionIndex;

	cout << endl << "p2p> ";
	while(cin >> commandName)
	{
		if((functionIndex = commandArray->exists(commandName, 0, commandArray->size() - 1)) != -1)
		{
			cin.getline(command, MAX_COMMAND_SIZE);
			commandArray->execute(functionIndex, command);
		}
		else cout << "Command does not exist..." << endl;
		cout << endl << "p2p> ";
	}
}

void * makeRequests(void * args)
{
	FunctionArray * commandArray = (FunctionArray *)(args);
	commandPrompt(commandArray);
	return (void *)(NULL);
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

void * acceptDownloadRequests(void * args)
{
	int servent, readBytes;
	FunctionArray * commandArray = (FunctionArray *)(args);

	servent = commandArray->getServent();
	while(true)
	{
		int peer;
		pthread_t thread;
		sockaddr_in from;

		if(!acceptClient(peer, servent, &from))
			continue;
		DownloadParameter parameter(peer, from);
		pthread_create(&thread, NULL, FunctionArray::solveDownloadRequest, &parameter);
	}
	if(readBytes == -1)
		readError();
	return (void *)(NULL);
}

void * checkUnfinishedDownloads(void * args)
{
	FunctionArray::finishDownloads();
	return (void *)(NULL);
}

void signInProcedure(int &client, int &servent)
{
	char username[50], password[50], downloadPath[512];
	unsigned int usernameSize, passwordSize, signinStatus;
	ifstream configFile("path.conf");
	FunctionArray commandArray;
	pthread_t makeRequestThread, acceptDownloadRequestThread, fileShareThread, checkUnfinishedDownloadThread;

	commandArray.setClient(client);
	commandArray.setServent(servent);
	commandArray.setSignalHandler();

	cin.ignore(1, '\n');
	cout << "Username : ";
	cin.getline(username, 50);
	usernameSize = strlen(username);
	readPasswordInHiddenMode(password, passwordSize);

	sendUserInfoToServer(client, usernameSize, username, passwordSize, password);
	if(read(client, &signinStatus, 4) == -1)
		readError();
	configFile.getline(downloadPath, 512);
	if(signinStatus == SIGN_IN_SUCCESS)
	{
		FileShareParameter * parameter = new FileShareParameter(client, downloadPath);
		pthread_create(&fileShareThread, NULL, shareFiles, (void *)parameter);
		sleep(3);
		cout << "You have signed in successfully!!!" << endl;
		chdir(downloadPath);

		pthread_create(&makeRequestThread, NULL, makeRequests, (void *)&commandArray);
		pthread_create(&acceptDownloadRequestThread, NULL, acceptDownloadRequests, (void *)&commandArray);
		pthread_create(&checkUnfinishedDownloadThread, NULL, checkUnfinishedDownloads, (void *)NULL);

		pthread_join(makeRequestThread, (void **)NULL);
		pthread_join(acceptDownloadRequestThread, (void **)NULL);
		pthread_join(checkUnfinishedDownloadThread, (void **)NULL);
		delete parameter;
	}
	else
	{
		cout << "Sign in failed!!! Wrong username, password or you are already signed in..." << endl;
		exit(EXIT_FAILURE);
	}
}

#endif /* SIGNINCLIENTPROCEDURE_HPP_ */
