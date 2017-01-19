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
#define START_DOWNLOAD true
#define FINISH_DOWNLOAD false

using namespace std;

int FunctionArray::client = 1;
int FunctionArray::servent = 1;
short FunctionArray::DOWNLOAD = 131;
short FunctionArray::FIND = 132;
short FunctionArray::QUIT = 135;
short FunctionArray::DOWNLOAD_FINISHED = 136;

deque<ActiveObjectThread> FunctionArray::activeDownload;
set<Peer, PeerComparator> FunctionArray::alreadyConnected;
set<ActiveObject, ActiveObjectComparator> FunctionArray::activeList;
deque<DownloadProcedureParameter> FunctionArray::unfinishedDownload;

struct InitFileTransferParameter
{
	unsigned int fileNameSize;
	unsigned long long int fileSize;
	vector<PEER> peer;
	char fileName[100];

	InitFileTransferParameter(vector<PEER> peer, char fileName[100], unsigned int fileNameSize, unsigned long long int fileSize)
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
	unsigned long long int startOffset;
	unsigned long long int endOffset;
	char fileName[100], fileID[16];

	MakeDownloadRequestParameter(int requestSocket, unsigned int fileNameSize, char fileName[100], unsigned long long int startOffset, unsigned long long int endOffset, char fileID[16])
	{
		this->requestSocket = requestSocket;
		this->fileNameSize = fileNameSize;
		strcpy(this->fileName, fileName);
		this->startOffset = startOffset;
		this->endOffset = endOffset;
		strcpy(this->fileID, fileID);
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
	close(client);
	close(servent);
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

void FunctionArray::deleteFromActiveList(char command[MAX_COMMAND_SIZE])
{
	unsigned int index;
	string toDeleteFileID(command);

	for(auto it = activeList.begin(); it != activeList.end(); ++it)
	{
		if(it->fileID == toDeleteFileID)
		{
			pause((char *)toDeleteFileID.c_str());
			remove(it->fileName.c_str());
			for(index = 0; index < activeDownload.size(); ++index)
			{
				if(activeDownload[index].fileID == toDeleteFileID)
				{
					pthread_cancel(activeDownload[index].thread);
					activeDownload.erase(activeDownload.begin() + index);
					--index;
				}
			}
			for(auto peer = alreadyConnected.begin(); peer != alreadyConnected.end(); ++peer)
			{
				if(peer->fileID == toDeleteFileID)
				{
					close(peer->requestSocket);
					alreadyConnected.erase(peer);
				}
			}
			activeList.erase(it);
			cout << "Active download deleted" << endl;
			return;
		}
	}
	cout << "Active download not found" << endl;
}

bool containsNonDigit(char * str, unsigned int size)
{
	for(unsigned int index = 0; index < size; ++index)
	{
		if(isLetter(str[index]) || isPunctuationSign(str[index]))
			return true;
	}
	return false;
}

void setPeerInfo(sockaddr_in &destination, string ip, uint16_t port)
{
	destination.sin_addr.s_addr = inet_addr(ip.c_str());
	destination.sin_port = port;
}

bool connectRequestSocket(int &requestSocket, sockaddr_in peer)
{
	if(connect(requestSocket, (sockaddr *)&peer, sizeof(sockaddr)) == -1)
	{
		perror("Connecting peer error");
		return false;
	}
	return true;
}

ActiveObjectThread * FunctionArray::getActiveObjectThread(pthread_t threadID)
{
	for(unsigned int index = 0; index < activeDownload.size(); ++index)
	{
		if(activeDownload[index].thread == threadID)
			return &activeDownload[index];
	}
	return (ActiveObjectThread *)NULL;
}

void * FunctionArray::downloadFileChunk(void * args)
{
	int readBytes;
	char fileChunk[CHUNK_SIZE];
	MakeDownloadRequestParameter * parameter = (MakeDownloadRequestParameter *)(args);
	int downloadFile = open(parameter->fileName, O_CREAT | O_WRONLY , 0644);
	long long int currentOffset;
	sockaddr_in peerInfo;
	unsigned int peerInfoSize = sizeof(sockaddr);
	ActiveObjectThread * activeDownloadThread = getActiveObjectThread(pthread_self());
	set<Peer, PeerComparator>::iterator found;

	getpeername(parameter->requestSocket, (sockaddr *)&peerInfo, &peerInfoSize);
	lseek(downloadFile, parameter->startOffset, SEEK_SET);
	if(write(parameter->requestSocket, &parameter->fileNameSize, 4) == -1 ||
			write(parameter->requestSocket, parameter->fileName, parameter->fileNameSize) == -1 ||
			write(parameter->requestSocket, &parameter->startOffset, 8) == -1 ||
			write(parameter->requestSocket, &parameter->endOffset, 8) == -1)
	{
		perror("Send file information error");
		return (void *)(NULL);
	}

	do
	{
		while(activeDownloadThread->active &&
				(readBytes = read(parameter->requestSocket, fileChunk, CHUNK_SIZE)) > 0)
		{
			if(write(downloadFile, fileChunk, readBytes) == -1)
			{
				perror("Write file chunk error");
				break;
			}
		}
		if(readBytes == -1)
			break;
		while(!activeDownloadThread->active) { }

	} while(readBytes > 0);

	if(readBytes == -1)
		perror("Receive file chunk error");
	if((currentOffset = lseek(downloadFile, 0, SEEK_CUR)) == -1)
		changeOffsetError();
	if((unsigned long long int)currentOffset < parameter->endOffset - 1)
		unfinishedDownload.push_back(DownloadProcedureParameter(parameter->fileID, FINISH_DOWNLOAD, currentOffset, parameter->endOffset));
	found = alreadyConnected.find(Peer(inet_ntoa(peerInfo.sin_addr), peerInfo.sin_port, parameter->fileID));
	if(found != alreadyConnected.end())
		alreadyConnected.erase(found);
	close(parameter->requestSocket);
	close(downloadFile);
	delete parameter;
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

void FunctionArray::deleteFromActiveDownload(ActiveObjectThread obj)
{
	for(auto it = activeDownload.begin(); it != activeDownload.end(); ++it)
	{
		if(it->thread == obj.thread && it->fileID == obj.fileID)
		{
			activeDownload.erase(it);
			return;
		}
	}
}

void FunctionArray::initFileTransfer(vector<Peer> peer, char fileName[100], unsigned int fileNameSize, unsigned long long int startOffset, unsigned long long int endOffset, char * fileID)
{
	sockaddr_in destinationPeer;
	unsigned int index;
	unsigned long long int chunkSize = ceil((double)(endOffset - startOffset) / (peer.size()));
	unsigned long long int peerStartOffset = startOffset, peerEndOffset = startOffset + chunkSize;
	vector<pthread_t> downloadThread;
	deque<ActiveObjectThread>::iterator found;

	downloadThread.reserve(peer.size());
	destinationPeer.sin_family = AF_INET;

	for(index = 0; index < peer.size(); ++index)
	{
		int client;
		pthread_t requestDownloadThread;
		MakeDownloadRequestParameter * parameter = NULL;

		if(!createSocket(client))
			continue;

		setPeerInfo(destinationPeer, peer[index].ip, peer[index].port);
		if(!connectRequestSocket(client, destinationPeer))
			continue;

		peer[index].requestSocket = client;
		parameter = new MakeDownloadRequestParameter(client, fileNameSize, fileName, peerStartOffset, peerEndOffset, fileID);
		alreadyConnected.insert(peer[index]);
		pthread_create(&requestDownloadThread, NULL, downloadFileChunk, (void *)parameter);
		downloadThread.push_back(requestDownloadThread);
		activeDownload.push_back(ActiveObjectThread(fileID, requestDownloadThread));
		peerStartOffset = peerEndOffset;

		if(peer.size() > 1)
		{
			if(index < peer.size() - 2)
				peerEndOffset += chunkSize;
			else
				peerEndOffset = endOffset;
		}
	}

	for(index = 0; index < downloadThread.size(); ++index)
	{
		pthread_join(downloadThread[index], (void **)(NULL));
		deleteFromActiveDownload(ActiveObjectThread(fileID, downloadThread[index]));
	}
}

bool FunctionArray::downloadFinished(char fileID[16])
{
	for(unsigned int index = 0; index < unfinishedDownload.size(); ++index)
	{
		if(strcmp(unfinishedDownload[index].fileID, fileID) == 0)
			return false;
	}
	return true;
}

void FunctionArray::downloadAcknowledgement(char fileID[16])
{
	unsigned int idSize = strlen(fileID);
	sendInfoToServer(&DOWNLOAD_FINISHED, 2);
	sendInfoToServer(&idSize, 4);
	sendInfoToServer(fileID, idSize);
}

void * FunctionArray::startDownloadProcedure(void * args)
{
	DownloadProcedureParameter * parameter = (DownloadProcedureParameter *)(args);
	int readBytes, ipSize, fileNameSize;
	long long int fileSize;
	char peerIP[15], fileName[100];
	uint16_t peerPort;
	vector<Peer> peer;

	pthread_detach(pthread_self());
	if(read(client, &fileNameSize, 4) == -1)
		readError();

	if(fileNameSize != -1)
	{
		if(read(client, fileName, fileNameSize) == -1 || read(client, &fileSize, 8) == -1)
			readError();
		fileName[fileNameSize] = '\0';
		if(parameter->type == START_DOWNLOAD)
			activeList.insert(ActiveObject(fileName, 0, "downloading", 0.0, parameter->fileID));

		while((readBytes = read(client, &ipSize, 4)) > 0 && ipSize != -1 &&
					(readBytes = read(client, peerIP, ipSize)) > 0 &&
					(readBytes = read(client, &peerPort, sizeof(uint16_t))) > 0)
		{
			peerIP[ipSize] = '\0';
			Peer peerInfo(peerIP, peerPort, parameter->fileID);
			if(alreadyConnected.find(peerInfo) == alreadyConnected.end())
				peer.push_back(peerInfo);
		}
		if(readBytes == -1)
			readError();
		if(peer.size() > 0)
		{
			if(parameter->type == START_DOWNLOAD)
			{
				cout << fileName << " is downloading" << endl;
				initFileTransfer(peer, fileName, fileNameSize, 0, fileSize, parameter->fileID);
			}
			else initFileTransfer(peer, fileName, fileNameSize, parameter->startOffset, parameter->endOffset, parameter->fileID);
			if(downloadFinished(parameter->fileID))
			{
				set<ActiveObject, ActiveObjectComparator>::iterator found = activeList.find(ActiveObject(fileName, 0, "downloading", 0.0, parameter->fileID));
				if(found != activeList.end())
					activeList.erase(found);
				downloadAcknowledgement(parameter->fileID);
			}
		}
		else unfinishedDownload.push_back(DownloadProcedureParameter(parameter->fileID, parameter->type, parameter->startOffset, parameter->endOffset));
	}
	else cout << "Wanted file does not exist..." << endl;
	return (void *)NULL;
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
		DownloadProcedureParameter * parameter = new DownloadProcedureParameter(fileID, START_DOWNLOAD);
		pthread_create(&startDownloadProcedureThread, NULL, startDownloadProcedure, (void *)parameter);
		sleep(1);
	}
	else cout << "Wrong arguments : ID must contain only digits and must have size greater than 0" << endl;
}

void FunctionArray::displayActiveList(char command[MAX_COMMAND_SIZE])
{
	for(auto it = activeList.begin(); it != activeList.end(); ++it)
	{
		cout << it->fileName << "     " << it->status <<  "     ";
		//if(it->status != "seeding")
			//cout << it->percentage << '%' << "     ";
		 cout << it->fileID << endl;
	}
}

void FunctionArray::quit(char command[MAX_COMMAND_SIZE])
{
	sendInfoToServer(&QUIT, 2);
	close(client);
	close(servent);
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

void FunctionArray::pause(char command[MAX_COMMAND_SIZE])
{
	string toPauseFileID(command);
	for(unsigned int index = 0; index < activeDownload.size(); ++index)
	{
		if(activeDownload[index].fileID == toPauseFileID)
			activeDownload[index].active = false;
	}
}

void FunctionArray::resume(char command[MAX_COMMAND_SIZE])
{
	string toResumeFileID(command);
	for(unsigned int index = 0; index < activeDownload.size(); ++index)
	{
		if(activeDownload[index].fileID == toResumeFileID)
			activeDownload[index].active = true;
	}
}

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

int readFileChunk(int &file, char fileChunk[CHUNK_SIZE], unsigned int chunkSize)
{
	int readBytes;
	if((readBytes = read(file, fileChunk, chunkSize)) == -1)
	{
		perror("File read error");
		return -1;
	}
	return readBytes;
}

bool sendFileChunkToPeer(int &peer, char fileChunk[CHUNK_SIZE], unsigned int chunkSize)
{
	if(write(peer, fileChunk, chunkSize) == -1)
	{
		//perror("Send file chunk");
		return false;
	}
	return true;
}

void freeMemory(int &peer, int &file)
{
	close(peer);
	close(file);
}

void FunctionArray::sendFileChunk(int &peer, char fileName[100], unsigned long long int startOffset, unsigned long long int endOffset)
{
	int file, readBytes;
	long long int currentOffset;
	char fileChunk[CHUNK_SIZE];

	if((file = open(fileName, O_RDONLY)) == -1)
	{
		openError(fileName);
		close(peer);
		return;
	}
	if(lseek(file, startOffset, SEEK_SET) == -1)
	{
		changeOffsetError();
		freeMemory(peer, file);
		return;
	}
	while((currentOffset = lseek(file, 0, SEEK_CUR)) != -1 &&
			(unsigned long long int)currentOffset + CHUNK_SIZE < endOffset)
	{
		if((readBytes = readFileChunk(file, fileChunk, CHUNK_SIZE)) == -1 ||
				!sendFileChunkToPeer(peer, fileChunk, readBytes))
		{
			freeMemory(peer, file);
			return;
		}
	}
	if(currentOffset == -1)
	{
		changeOffsetError();
		freeMemory(peer, file);
		return;
	}
	if((unsigned long long int)currentOffset < endOffset)
	{
		if(readFileChunk(file, fileChunk, endOffset - currentOffset) == -1 ||
				!sendFileChunkToPeer(peer, fileChunk, endOffset - currentOffset))
		{
			freeMemory(peer, file);
			return;
		}
	}
	freeMemory(peer, file);
}

void * FunctionArray::solveDownloadRequest(void * args)
{
	DownloadParameter * parameter = (DownloadParameter *)(args);
	char fileName[100];
	unsigned int fileNameSize;
	unsigned long long startOffset, endOffset;
	set<ActiveObject, ActiveObjectComparator>::iterator found;

	pthread_detach(pthread_self());
	if(read(parameter->peer, &fileNameSize, 4) == -1 || read(parameter->peer, fileName, fileNameSize) == -1)
		readError();
	fileName[fileNameSize] = '\0';

	ActiveObject seedingFile(fileName);
	found = activeList.find(seedingFile);
	if(found != activeList.end())
	{
		ActiveObject seedingFile(fileName, found->downloadCount + 1);
		activeList.erase(found);
		activeList.insert(seedingFile);
	}
	else activeList.insert(seedingFile);

	if(read(parameter->peer, &startOffset, 8) == -1 || read(parameter->peer, &endOffset, 8) == -1)
		readError();
	sendFileChunk(parameter->peer, fileName, startOffset, endOffset);

	found = activeList.find(seedingFile);
	if(found != activeList.end())
	{
		if(found->downloadCount == 1)
			activeList.erase(activeList.find(seedingFile));
	else
		--(found->downloadCount);
	}
	return (void *)(NULL);
}

void FunctionArray::finishDownloads()
{
	unsigned int idSize;
	while(true)
	{
		if(unfinishedDownload.size())
		{
			sendInfoToServer(&DOWNLOAD, 2);
			idSize = strlen(unfinishedDownload[0].fileID);
			sendInfoToServer(&idSize, 4);
			sendInfoToServer(unfinishedDownload[0].fileID, idSize);

			pthread_t startDownloadProcedureThread;
			pthread_create(&startDownloadProcedureThread, NULL, startDownloadProcedure, (void *)&unfinishedDownload[0]);
			unfinishedDownload.pop_front();
		}
		sleep(5);
	}
}

/*void FunctionArray::resumeUnfinishedDownloads()
{
	bool downloadType;
	string type, fileID;
	ifstream toFinish("./to_finish.in");
	unsigned long long int startOffset, endOffset;

	cout << "Reading to_finish.in" <<endl;
	while(toFinish >> type)
	{
		toFinish >> fileID;
		if(type == "active")
		{
			toFinish >> startOffset >> endOffset;
			cout << "Inserting active" << endl;
			unfinishedDownload.push_back(DownloadProcedureParameter((char *)(fileID.c_str()), FINISH_DOWNLOAD, startOffset, endOffset));
		}
		else
		{
			toFinish >> downloadType >> startOffset >> endOffset;
			cout << "Inserting unfinished" << endl;
			unfinishedDownload.push_back(DownloadProcedureParameter((char *)fileID.c_str(), downloadType, startOffset, endOffset));
		}
	}
	toFinish.close();
}

void FunctionArray::saveUnfinishedDownloads()
{
	cout << "Writing to_finish.in" <<endl;
	ofstream toFinish("./to_finish.in");

	for(auto it = unfinishedDownload.begin(); it != unfinishedDownload.end(); ++it)
		toFinish << "unfinished" << ' ' << it->fileID << ' ' << it->type << ' ' << it->startOffset << ' ' << it->endOffset << endl;

	for(auto it = activeDownload.begin(); it != activeDownload.end(); ++it)
		toFinish << "active" << ' ' << it->fileID << ' ' << it->startOffset << ' ' << it->endOffset << endl;

	toFinish.close();
}
*/
