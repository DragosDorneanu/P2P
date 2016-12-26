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
#include "FunctionArray.hpp"

#define DELETE 128
#define DOWNLOAD 129
#define FIND 130
#define PAUSE 131
#define START 132
#define QUIT 133

using namespace std;

int FunctionArray::client = 0;

void FunctionArray::writeError()
{
	perror("Write error");
	exit(EXIT_FAILURE);
}

void FunctionArray::readError()
{
	perror("Read error");
	exit(EXIT_FAILURE);
}

void FunctionArray::deleteFromActiveList(char command[MAX_COMMAND_SIZE]) { }

void FunctionArray::download(char command[MAX_COMMAND_SIZE]) { }

void FunctionArray::quit(char command[MAX_COMMAND_SIZE])
{
	cout << "Good bye" << endl;
	exit(EXIT_SUCCESS);
}

int FunctionArray::getTokenID(char token[MAX_COMMAND_SIZE])
{
	return -1;
}

void FunctionArray::find(char command[MAX_COMMAND_SIZE])
{
	int id;
	char * p, * fileName;
	unsigned int fileNameSize;
	vector<int> option;

	if(write(client, &FIND, 4) == -1)
		writeError();
	p = strtok(command, " ");
	while(p && p[0] == '-')
	{
		if((id = getTokenID(p)) == -1)
		{
			cout << "Unrecognized token : " << p << endl;
			return;
		}
		option.push_back(id);
		p = strtok(NULL, " ");
	}
	fileName = p;
	p = strtok(NULL, "\n");
	if(p != NULL)
		strcat(fileName, p);
	fileNameSize = strlen(fileName);
	if(write(client, &option.size(), 4) == -1)
		writeError();
	for(unsigned int index = 0; index < option.size(); ++index)
	{
		if(write(client, &option[index], 4) == -1)
			writeError();
	}
	if(write(client, &fileNameSize, 4) == -1 || write(client, fileName, fileNameSize) == -1)
			writeError();
	//receive info from server
}

void FunctionArray::pause(char command[MAX_COMMAND_SIZE]) { }

void FunctionArray::start(char command[MAX_COMMAND_SIZE]) { }

FunctionArray::FunctionArray(int clientSD)
{
	this->function.push_back(make_pair("delete", deleteFromActiveList));
	this->function.push_back(make_pair("download", download));
	this->function.push_back(make_pair("find", find));
	this->function.push_back(make_pair("pause", pause));
	this->function.push_back(make_pair("start", start));
	this->function.push_back(make_pair("quit", quit));
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
