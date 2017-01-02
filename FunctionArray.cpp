/*
 * FunctionArray.cpp

 *
 *  Created on: Dec 26, 2016
 *      Author: dragos
 */

#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "FunctionArray.hpp"

#define SEARCH_BY_SIZE 20
#define SEARCH_BY_TYPE 21
#define SEARCH_BY_NAME 22
#define SEARCH_BY_BIGGER_SIZE 23
#define SEARCH_BY_SMALLER_SIZE 24

#define OPTION pair<int, char *>

using namespace std;

int FunctionArray::client = 1;
int FunctionArray::servent = 2;
short FunctionArray::DELETE = 130;
short FunctionArray::DOWNLOAD = 131;
short FunctionArray::FIND = 132;
short FunctionArray::PAUSE = 133;
short FunctionArray::RESUME = 134;
short FunctionArray::QUIT = 135;

void setSignalError()
{
	perror("Error while setting signal handler");
	exit(EXIT_FAILURE);
}

void FunctionArray::quitSignalHandler(int signal)
{
	sendInfoToServer(&QUIT, 2);
	cout << endl << "Good bye!" << endl;
	exit(EXIT_SUCCESS);
}

void FunctionArray::setSignalHandler()
{
	if(signal(SIGINT, quitSignalHandler) == SIG_ERR)
		setSignalError();
}

void FunctionArray::writeError()
{
	sendInfoToServer(&QUIT, 2);
	perror("Write error");
	exit(EXIT_FAILURE);
}

void FunctionArray::readError()
{
	sendInfoToServer(&QUIT, 2);
	perror("Read error");
	exit(EXIT_FAILURE);
}

void FunctionArray::sendInfoToServer(void * data, unsigned int dataSize)
{
	if(write(client, data, dataSize) == -1)
		writeError();
}

bool isDigit(char ch) {
	return (ch >= '0' && ch <= '9');
}

bool isLetter(char ch) {
	return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
}

bool isPunctuationSign(char ch) {
	return (!isDigit(ch) && !isLetter(ch) && ch != '\n' && ch != '\t' && ch != ' ');
}

void FunctionArray::deleteFromActiveList(char command[MAX_COMMAND_SIZE]) { }

bool containsNonDigit(char * str, unsigned int size)
{
	for(unsigned int index = 0; index < size; ++index)
	{
		if(isLetter(str[index]) || isPunctuationSign(str[index]))
			return true;
	}
	return false;
}

void setPeerInfo(sockaddr_in &destination, char ip[15], uint16_t port)
{
	destination.sin_addr.s_addr = inet_addr(ip);
	destination.sin_port = htons(port);
}

void FunctionArray::download(char fileID[MAX_COMMAND_SIZE])
{
	int readBytes, ipSize, fileNameSize;
	char peerIP[15], fileName[100];
	unsigned int idSize, peerCount = 0, length;
	uint16_t peerPort;
	sockaddr_in destinationPeer;

	cout << endl;
	idSize = strlen(fileID);
	if(idSize > 0 && !containsNonDigit(fileID, idSize))
	{
		sendInfoToServer(&DOWNLOAD, 2);
		sendInfoToServer(&idSize, 4);
		sendInfoToServer(fileID, idSize);

		if(read(client, &fileNameSize, 4) == -1)
			readError();
		if(fileNameSize != -1)
		{
			if(read(client, fileName, fileNameSize) == -1)
				readError();
			fileName[fileNameSize] = '\0';
			cout << fileName << endl;
			length = sizeof(destinationPeer);
			destinationPeer.sin_family = AF_INET;
			while((readBytes = read(client, &ipSize, 4)) > 0 && ipSize != -1 &&
					(readBytes = read(client, peerIP, ipSize)) > 0 &&
					(readBytes = read(client, &peerPort, sizeof(uint16_t))) > 0)
			{
				peerIP[ipSize] = '\0';
				fileName[fileNameSize] = '\0';
				++peerCount;
				setPeerInfo(destinationPeer, peerIP, peerPort);
				if(sendto(servent, &fileNameSize, 4, 0, (sockaddr *)&destinationPeer, length) == -1 ||
						sendto(servent, fileName, fileNameSize, 0, (sockaddr *)&destinationPeer, length) == -1)
					writeError();
				cout << "Starting communication with " << inet_ntoa(destinationPeer.sin_addr) << ' ' << ntohs(destinationPeer.sin_port) << endl;
			}
			if(readBytes == -1)
				readError();
		}
		else cout << "Wanted file does not exist..." << endl;
	}
	else cout << "Wrong arguments : ID must contain only digits and must have size greater than 0" << endl;
}

void FunctionArray::quit(char command[MAX_COMMAND_SIZE])
{
	sendInfoToServer(&QUIT, 2);
	cout << "Good bye!" << endl;
	exit(EXIT_SUCCESS);
}

int getTokenID(char token[MAX_COMMAND_SIZE])
{
	if(strcmp(token, "-n") == 0)
		return SEARCH_BY_NAME;
	else if(strcmp(token, "-t") == 0)
		return SEARCH_BY_TYPE;
	if(strcmp(token, "-s") == 0)
		return SEARCH_BY_SIZE;
	if(strcmp(token, "-bs") == 0)
		return SEARCH_BY_BIGGER_SIZE;
	if(strcmp(token, "-ss") == 0)
		return SEARCH_BY_SMALLER_SIZE;
	return -1;
}

void FunctionArray::find(char command[MAX_COMMAND_SIZE])
{
	int id, readBytes, fileNameSize;
	char * p, fileName[1024];
	unsigned int optionCount, restrictionSize, size, hashID;
	vector<OPTION> option;

	cout << endl;
	p = strtok(command, " ");
	while(p && p[0] == '-')
	{
		if((id = getTokenID(p)) == -1)
		{
			cout << "Unrecognized token : " << p << endl;
			return;
		}
		if(id != SEARCH_BY_NAME)
		{
			p = strtok(NULL, " ");
			if(p == NULL || p[0] == '-' ||
				(id == SEARCH_BY_SIZE && !isDigit(p[0])) ||
				(id == SEARCH_BY_TYPE && !isLetter(p[0])) ||
				(id == SEARCH_BY_BIGGER_SIZE && !isDigit(p[0])) ||
				(id == SEARCH_BY_SMALLER_SIZE && !isDigit(p[0])))
			{
				cout << "Wrong arguments" << endl;
				return;
			}
			option.push_back(make_pair(id, p));
		}
		else option.push_back(make_pair(id, (char *)(NULL)));
		p = strtok(NULL, " ");
	}
	if(p != NULL)
	{
		sendInfoToServer(&FIND, 2);
		strcpy(fileName, p);
		p = strtok(NULL, "\n");
		if(p != NULL)
			strcat(fileName, p);
		fileNameSize = strlen(fileName);
		optionCount = option.size();
		sendInfoToServer(&optionCount, 4);
		for(unsigned int index = 0; index < option.size(); ++index)
		{
			sendInfoToServer(&option[index].first, 4);
			if(option[index].first != SEARCH_BY_NAME)
			{
				restrictionSize = strlen(option[index].second);
				sendInfoToServer(&restrictionSize, 4);
				sendInfoToServer(option[index].second, restrictionSize);
			}
		}
		sendInfoToServer(&fileNameSize, 4);
		sendInfoToServer(fileName, fileNameSize);
		while((readBytes = read(client, &fileNameSize, 4)) > 0 &&
				fileNameSize != -1 &&
				(readBytes = read(client, fileName, fileNameSize)) > 0 &&
				(readBytes = read(client, &size, 4)) > 0 &&
				(readBytes = read(client, &hashID, 4)) > 0)
		{
			fileName[fileNameSize] = '\0';
			cout << fileName << "     " << size << " bytes" <<  "     " << hashID << endl;
		}
		cout << endl;
		if(readBytes == -1)
			readError();
	}
	else cout << "A file name is needed..." << endl;
}

void FunctionArray::pause(char command[MAX_COMMAND_SIZE]) { }

void FunctionArray::resume(char command[MAX_COMMAND_SIZE]) { }

FunctionArray::FunctionArray()
{
	this->function.push_back(make_pair("delete", deleteFromActiveList));
	this->function.push_back(make_pair("download", download));
	this->function.push_back(make_pair("find", find));
	this->function.push_back(make_pair("pause", pause));
	this->function.push_back(make_pair("quit", quit));
	this->function.push_back(make_pair("resume", resume));
}

FunctionArray::~FunctionArray() { }

unsigned int FunctionArray::size() {
	return function.size();
}

string FunctionArray::getName(unsigned int index) {
	return function[index].first;
}

int FunctionArray::exists(string commandName, int lowerBound, int upperBound)
{
	if(lowerBound <= upperBound)
	{
		int mid = (lowerBound + upperBound) / 2;
		int difference = commandName.compare(getName(mid));
		if(difference == 0)
			return mid;
		else if(difference < 0)
			return exists(commandName, lowerBound, mid - 1);
		return exists(commandName, mid + 1, upperBound);
	}
	return -1;
}

void FunctionArray::execute(unsigned int functionIndex, char command[MAX_COMMAND_SIZE]) {
	(function[functionIndex].second)(command);
}

void FunctionArray::setClient(int clientSD) {
	client = clientSD;
}

void FunctionArray::setServent(int serventSD) {
	servent = serventSD;
}

int FunctionArray::getServent() {
	return servent;
}
