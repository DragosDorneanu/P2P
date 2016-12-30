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
#include "FunctionArray.hpp"

#define SEARCH_BY_SIZE 20
#define SEARCH_BY_TYPE 21
#define SEARCH_BY_NAME 22
#define OPTION pair<int, char *>

using namespace std;

int FunctionArray::client = 1;
int FunctionArray::DELETE = 130;
int FunctionArray::DOWNLOAD = 131;
int FunctionArray::FIND = 132;
int FunctionArray::PAUSE = 133;
int FunctionArray::START = 134;
int FunctionArray::QUIT = 135;

void setSignalError()
{
	perror("Error while setting signal handler");
	exit(EXIT_FAILURE);
}

void FunctionArray::quitSignalHandler(int signal)
{
	sendInfoToServer(&QUIT, 4);
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
	sendInfoToServer(&QUIT, 4);
	perror("Write error");
	exit(EXIT_FAILURE);
}

void FunctionArray::readError()
{
	sendInfoToServer(&QUIT, 4);
	perror("Read error");
	exit(EXIT_FAILURE);
}

void FunctionArray::sendInfoToServer(void * data, unsigned int dataSize)
{
	if(write(client, data, dataSize) == -1)
		writeError();
}

void FunctionArray::deleteFromActiveList(char command[MAX_COMMAND_SIZE]) { }

void FunctionArray::download(char command[MAX_COMMAND_SIZE]) { }

void FunctionArray::quit(char command[MAX_COMMAND_SIZE])
{
	sendInfoToServer(&QUIT, 4);
	cout << "Good bye!" << endl;
	exit(EXIT_SUCCESS);
}

bool isDigit(char ch) {
	return (ch >= '0' && ch <= '9');
}

bool isLetter(char ch) {
	return ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '.');
}

int getTokenID(char token[MAX_COMMAND_SIZE])
{
	if(strcmp(token, "-n") == 0)
		return SEARCH_BY_NAME;
	else if(strcmp(token, "-t") == 0)
		return SEARCH_BY_TYPE;
	if(strcmp(token, "-s") == 0)
		return SEARCH_BY_SIZE;
	return -1;
}

void FunctionArray::find(char command[MAX_COMMAND_SIZE])
{
	int id, readBytes, fileNameSize;
	char * p, fileName[1024], size[12];
	unsigned int optionCount, restrictionSize, sizeDigits;
	vector<OPTION> option;

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
			if(p == NULL || p[0] == '-' || (id == SEARCH_BY_SIZE && !isDigit(p[0])) || (id == SEARCH_BY_TYPE && !isLetter(p[0])))
			{
				cout << "Wrong arguments" << endl;
				return;
			}
			option.push_back(make_pair(id, p));
		}
		else option.push_back(make_pair(id, new char));
		p = strtok(NULL, " ");
	}
	if(p != NULL)
	{
		sendInfoToServer(&FIND, 4);
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
				(readBytes = read(client, &sizeDigits, 4)) > 0 &&
				(readBytes = read(client, &size, sizeDigits)) > 0)
		{
			fileName[fileNameSize] = '\0';
			size[sizeDigits] = '\0';
			cout << fileName << "   " << size << " bytes" << endl;
		}
		cout << endl;
		if(readBytes == -1)
			readError();
	}
	else cout << "A file name is needed..." << endl;
}

void FunctionArray::pause(char command[MAX_COMMAND_SIZE]) { }

void FunctionArray::start(char command[MAX_COMMAND_SIZE]) { }

FunctionArray::FunctionArray(int clientSD)
{
	this->function.push_back(make_pair("delete", deleteFromActiveList));
	this->function.push_back(make_pair("download", download));
	this->function.push_back(make_pair("find", find));
	this->function.push_back(make_pair("pause", pause));
	this->function.push_back(make_pair("quit", quit));
	this->function.push_back(make_pair("start", start));
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
