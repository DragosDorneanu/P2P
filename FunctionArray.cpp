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
	//send number of find criteria
	//send defines corresponding to the search criteria
	//send the name of the searched file
	//read results that come from the server
	int id;
	char * p = strtok(command, " ");
	while(p)
	{
		if((id = getTokenID(p)) == -1)
		{
			cout << "Unrecognized token : " << p << endl;
			return;
		}
		if(write(client, &id, 4) == -1)
			writeError();
		p = strtok(NULL, " ");
	}
}

void FunctionArray::pause(char command[MAX_COMMAND_SIZE]) { }

void FunctionArray::start(char command[MAX_COMMAND_SIZE]) { }

FunctionArray::FunctionArray(int clientSD)
{
	this->function.push_back(make_pair("delete", download));
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
