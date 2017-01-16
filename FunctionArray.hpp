/*
 * FunctionArray.hpp
 *
 *  Created on: Dec 26, 2016
 *      Author: dragos
 */

#ifndef FUNCTIONARRAY_HPP_
#define FUNCTIONARRAY_HPP_

#include <vector>
#include <string>
#include <set>
#include <deque>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

#define MAX_COMMAND_SIZE 1024
#define FUNCTION pair<string, void(*)(char *)>
#define PEER pair<char *, uint16_t>

struct DownloadProcedureParameter
{
	char fileID[16];
	bool type;
	unsigned long long int startOffset, endOffset;

	DownloadProcedureParameter(char fileID[16], bool type, unsigned long long int startOffset = 0, unsigned long long int endOffset = 0)
	{
		strcpy(this->fileID, fileID);
		this->type = type;
		this->startOffset = startOffset;
		this->endOffset = endOffset;
	}
};

struct PeerComparator
{
	int operator()(PEER peer1, PEER peer2)
	{
		unsigned int difference = strcmp(peer1.first, peer2.first);
		if(difference == 0)
			return peer1.second < peer2.second;
		else
			return difference < 0;
	}
};

struct ActiveObject
{
	string fileName;
	string status;
	double percentage;
	string fileID;

	ActiveObject(string fileName, string status = "seeding", double percentage = 0.0, string fileID = "")
	{
		this->fileName = fileName;
		this->status = status;
		this->percentage = percentage;
		this->fileID = fileID;
	}
};

struct ActiveObjectComparator
{
	int operator()(ActiveObject obj1, ActiveObject obj2)
	{
		int difference = obj1.fileID.compare(obj2.fileID);
		return difference < 0;
	}
};

struct ActiveObjectThread
{
	string fileID;
	pthread_t thread;
	bool active;

	ActiveObjectThread(string fileID, pthread_t thread, bool active = true)
	{
		this->fileID = fileID;
		this->thread = thread;
		this->active = active;
	}
};

class FunctionArray
{
private:
	static int client, servent;
	vector<FUNCTION> function;
	static deque<ActiveObjectThread> activeDownload;
	static set<PEER, PeerComparator> alreadyConnected;
	static deque<DownloadProcedureParameter> unfinishedDownload;
	static set<ActiveObject, ActiveObjectComparator> activeList;
	static short DOWNLOAD, FIND, QUIT, DOWNLOAD_FINISHED;

	static void sendInfoToServer(void * data, unsigned int dataSize);
	static void displayActiveList(char command[MAX_COMMAND_SIZE]);
	static void find(char command[MAX_COMMAND_SIZE]);
	static void download(char fileID[MAX_COMMAND_SIZE]);
	static void pause(char command[MAX_COMMAND_SIZE]);
	static void resume(char command[MAX_COMMAND_SIZE]);
	static void deleteFromActiveList(char command[MAX_COMMAND_SIZE]);
	static void quit(char command[MAX_COMMAND_SIZE]);
	static void readError();
	static void writeError();
	static void quitSignalHandler(int signal);
	static void * downloadFileChunk(void * args);
	static void initFileTransfer(vector<PEER> peer, char fileName[100], unsigned int fileNameSize, unsigned long long int startOffset, unsigned long long int endOffset, char * fileID);
	static void sendFileChunk(int &peer, char fileName[100], unsigned long long int startOffset, unsigned long long int endOffset);
	static void * startDownloadProcedure(void * args);
	static bool downloadFinished(char fileID[16]);
	static void downloadAcknowledgement(char fileID[16]);
	static void deleteFromActiveDownload(ActiveObjectThread obj);
	static ActiveObjectThread * getActiveObjectThread(pthread_t threadID);

public:
	FunctionArray();
	virtual ~FunctionArray();
	unsigned int size();
	string getName(unsigned int index);
	int exists(string commandArray, int lowerBound, int upperBound);
	void execute(unsigned int functionIndex, char command[MAX_COMMAND_SIZE]);
	static void setClient(int clientSD);
	static void setServent(int serventSD);
	static void setSignalHandler();
	static int getServent();
	static void * solveDownloadRequest(void * args);
	static void finishDownloads();
};

struct DownloadParameter
{
	int peer;
	sockaddr_in from;

	DownloadParameter(int peer, sockaddr_in from)
	{
		this->peer = peer;
		this->from = from;
	}
};

#endif /* FUNCTIONARRAY_HPP_ */
