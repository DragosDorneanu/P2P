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
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cmath>
#include <set>
#include "FunctionArray.hpp"

#define SEARCH_BY_SIZE 20
#define SEARCH_BY_TYPE 21
#define SEARCH_BY_NAME 22
#define SEARCH_BY_BIGGER_SIZE 23
#define SEARCH_BY_SMALLER_SIZE 24
#define OPTION pair<int, char *>
#define PEER pair<char *, uint16_t>
#define CHUNK_SIZE 10240

using namespace std;

int FunctionArray::client = 1;
int FunctionArray::servent = 1;
short FunctionArray::DELETE = 130;
short FunctionArray::DOWNLOAD = 131;
short FunctionArray::FIND = 132;
short FunctionArray::PAUSE = 133;
short FunctionArray::RESUME = 134;
short FunctionArray::QUIT = 135;
multiset<ACTIVE_OBJECT> FunctionArray::activeList;

struct InitFileTransferParameter
{
	unsigned int fileNameSize;
	unsigned int fileSize;
	vector<PEER> peer;
	char fileName[100];

	InitFileTransferParameter(vector<PEER> peer, char fileName[100], unsigned int fileNameSize, unsigned int fileSize)
	{
		this->peer = peer;
		strcpy(this->fileName, fileName);
		this->fileNameSize = fileNameSize;
		this->fileSize = fileSize;
	}
};

struct MakeDownloadRequestParameter
{
	int requestSocket;
	unsigned int fileNameSize;
	unsigned int startOffset;
	unsigned int endOffset;
	char fileName[100];

	MakeDownloadRequestParameter(int reqSocket, unsigned int fileNameLen, char file[100], unsigned int sOffset, unsigned int eOffset)
	{
		requestSocket = reqSocket;
		fileNameSize = fileNameLen;
		strcpy(fileName, file);
		startOffset = sOffset;
		endOffset = eOffset;
	}
};

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

void openError(char fileName[100])
{
	cout << "File Name : " << fileName << endl;
	perror("Error while opening file");
}

void changeOffsetError() {
	perror("LSEEK error");
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
	destination.sin_port = port;
}

void connectRequestSocket(int &requestSocket, sockaddr_in peer)
{
	if(connect(requestSocket, (sockaddr *)&peer, sizeof(sockaddr)) == -1)
	{
		perror("Connecting peer error");
		exit(EXIT_FAILURE);
	}
}

void * downloadFileChunk(void * args)
{
	int readBytes;
	char fileChunk[CHUNK_SIZE];
	MakeDownloadRequestParameter * parameter = (MakeDownloadRequestParameter *)(args);
	int downloadFile = open(parameter->fileName, O_CREAT | O_WRONLY , S_IRWXU);

	lseek(downloadFile, parameter->startOffset, SEEK_SET);
	if(write(parameter->requestSocket, &parameter->fileNameSize, 4) == -1 ||
			write(parameter->requestSocket, parameter->fileName, parameter->fileNameSize) == -1 ||
			write(parameter->requestSocket, &parameter->startOffset, 4) == -1 ||
			write(parameter->requestSocket, &parameter->endOffset, 4) == -1)
	{
		perror("Send file information error");
		return (void *)(NULL);
	}

	while((readBytes = read(parameter->requestSocket, fileChunk, CHUNK_SIZE)) > 0)
	{
		if(write(downloadFile, fileChunk, readBytes) == -1)
		{
			perror("Write file chunk error");
			break;
		}
	}
	if(readBytes == -1)
		perror("Receive file chunk error");
	delete parameter;
	close(parameter->requestSocket);
	return (void *)(NULL);
}

bool createSocket(int &client)
{
	if((client = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("Error while creating socket");
		return false;
	}
	return true;
}

void initFileTransfer(vector<PEER> peer, char fileName[100], unsigned int fileNameSize, unsigned int fileSize)
{
	sockaddr_in destinationPeer;
	unsigned int index;
	unsigned int chunkSize = ceil((double)(fileSize) / (peer.size()));
	unsigned int startOffset = 0, endOffset = chunkSize;
	vector<pthread_t> downloadThread;

	downloadThread.reserve(peer.size());
	destinationPeer.sin_family = AF_INET;

	for(index = 0; index < peer.size(); ++index)
	{
		int client;
		pthread_t requestDownloadThread;
		MakeDownloadRequestParameter * parameter = NULL;

		if(!createSocket(client))
			continue;

		parameter = new MakeDownloadRequestParameter(client, fileNameSize, fileName, startOffset, endOffset);
		setPeerInfo(destinationPeer, peer[index].first, peer[index].second);
		connectRequestSocket(client, destinationPeer);
		cout << "Connecting to " << peer[index].first << " and port " << peer[index].second << endl;

		pthread_create(&requestDownloadThread, NULL, downloadFileChunk, (void *)parameter);
		downloadThread.push_back(requestDownloadThread);
		startOffset = endOffset + 1;
		endOffset += chunkSize;
	}

	for(index = 0; index < downloadThread.size(); ++index)
		pthread_join(downloadThread[index], (void **)(NULL));
}

void * FunctionArray::startDownloadProcedure(void * args)
{
	int readBytes, ipSize, fileNameSize, fileSize;
	char peerIP[15], fileName[100];
	uint16_t peerPort;
	vector<PEER> peer;

	pthread_detach(pthread_self());
	if(read(client, &fileNameSize, 4) == -1)
		readError();

	if(fileNameSize != -1)
	{
		if(read(client, fileName, fileNameSize) == -1 || read(client, &fileSize, 4) == -1)
			readError();
		fileName[fileNameSize] = '\0';

		while((readBytes = read(client, &ipSize, 4)) > 0 && ipSize != -1 &&
					(readBytes = read(client, peerIP, ipSize)) > 0 &&
					(readBytes = read(client, &peerPort, sizeof(uint16_t))) > 0)
			{
				peerIP[ipSize] = '\0';
				peer.push_back(make_pair(peerIP, peerPort));
			}
		if(readBytes == -1)
			readError();
		if(peer.size() > 0)
		{
			cout << fileName << " is downloading" << endl;
			initFileTransfer(peer, fileName, fileNameSize, fileSize);
		}
	}
	else cout << "Wanted file has no seeders for the moment..." << endl;
	return (void *)(NULL);
}

void FunctionArray::download(char fileID[MAX_COMMAND_SIZE])
{
	unsigned int idSize;

	cout << endl;
	idSize = strlen(fileID);
	if(idSize > 0 && !containsNonDigit(fileID, idSize))
	{
		sendInfoToServer(&DOWNLOAD, 2);
		sendInfoToServer(&idSize, 4);
		sendInfoToServer(fileID, idSize);

		pthread_t startDownloadProcedureThread;
		pthread_create(&startDownloadProcedureThread, NULL, startDownloadProcedure, (void *)(NULL));
		sleep(1);
	}
	else cout << "Wrong arguments : ID must contain only digits and must have size greater than 0" << endl;
}

void FunctionArray::displayActiveList(char command[MAX_COMMAND_SIZE])
{
	for(auto it = activeList.begin(); it != activeList.end(); ++it)
		cout << it->first << "     " << it->second.first << "     " << it->second.second << endl;
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
	this->function.push_back(make_pair("active", displayActiveList));
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

bool readFileChunk(int &file, char filechunk[CHUNK_SIZE], unsigned int chunkSize)
{
	if(read(file, filechunk, chunkSize) == -1)
	{
		perror("File read error");
		return false;
	}
	return true;
}

bool sendFileChunkToPeer(int &peer, char filechunk[CHUNK_SIZE], unsigned int chunkSize)
{
	if(write(peer, filechunk, chunkSize) == -1)
	{
		perror("Send file chunk error");
		return false;
	}
	return true;
}

void FunctionArray::sendFileChunk(int &peer, char fileName[100], unsigned int startOffset, unsigned int endOffset)
{
	int file, currentOffset;
	char fileChunk[CHUNK_SIZE];

	cout << get_current_dir_name() << endl;
	if((file = open(fileName, O_RDONLY)) == -1)
	{
		openError(fileName);
		return;
	}
	if(lseek(file, startOffset, SEEK_SET) == -1)
	{
		changeOffsetError();
		return;
	}
	while((currentOffset = lseek(file, 0, SEEK_CUR)) != -1 &&
			currentOffset + CHUNK_SIZE <= endOffset)
	{
		if(!readFileChunk(file, fileChunk, CHUNK_SIZE))
			return;
		if(!sendFileChunkToPeer(peer, fileChunk, CHUNK_SIZE))
			return;
	}
	if(currentOffset == -1)
	{
		changeOffsetError();
		return;
	}
	if(currentOffset < endOffset)
	{
		if(!readFileChunk(file, fileChunk, endOffset - currentOffset))
			return;
		sendFileChunkToPeer(peer, fileChunk, endOffset - currentOffset);
	}
	cout << "Sent !!!" << endl;
}

void * FunctionArray::solveDownloadRequest(void * args)
{
	DownloadParameter * parameter = (DownloadParameter *)(args);
	char fileName[100];
	unsigned int fileNameSize, startOffset, endOffset;

	pthread_detach(pthread_self());
	if(read(parameter->peer, &fileNameSize, 4) == -1 || read(parameter->peer, fileName, fileNameSize) == -1)
		readError();
	fileName[fileNameSize] = '\0';
	cout << endl << "Request : " << endl;
	cout << "File name : " << fileName << endl;
	cout << "From : " << "IP " << inet_ntoa(parameter->from.sin_addr) << " PORT " << parameter->from.sin_port << endl;
	if(read(parameter->peer, &startOffset, 4) == -1 || read(parameter->peer, &endOffset, 4) == -1)
		readError();
	cout << "Chunck Range : " << startOffset << " => " << endOffset << endl;
	sendFileChunk(parameter->peer, fileName, startOffset, endOffset);
	close(parameter->peer);
	return (void *)(NULL);
}
