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
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

#define MAX_COMMAND_SIZE 1024
#define FUNCTION pair<string, void(*)(char *)>
#define ACTIVE_OBJECT pair<string, pair<string, string> >
#define PEER pair<char *, uint16_t>

class FunctionArray
{
private:
	static int client, servent;
	vector<FUNCTION> function;
	static multiset<ACTIVE_OBJECT> activeList;
	static short DELETE, DOWNLOAD, FIND, PAUSE, RESUME, QUIT;

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
	static void sendFileChunk(int &peer, char fileName[100], unsigned int startOffset, unsigned int endOffset);
	static void * startDownloadProcedure(void * args);

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
