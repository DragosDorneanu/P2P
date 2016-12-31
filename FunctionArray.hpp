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

using namespace std;

#define MAX_COMMAND_SIZE 1024
#define FUNCTION pair<string, void(*)(char *)>
#define ACTIVE_OBJECT pair<string, pair<string, string> >

class FunctionArray
{
private:
	static int client, servent;
	vector<FUNCTION> function;
	multiset<ACTIVE_OBJECT> activeList;
	static int DELETE, DOWNLOAD, FIND, PAUSE, START, QUIT;

	static void sendInfoToServer(void * data, unsigned int dataSize);
	static void find(char command[MAX_COMMAND_SIZE]);
	static void download(char command[MAX_COMMAND_SIZE]);
	static void pause(char command[MAX_COMMAND_SIZE]);
	static void start(char command[MAX_COMMAND_SIZE]);
	static void deleteFromActiveList(char command[MAX_COMMAND_SIZE]);
	static void quit(char command[MAX_COMMAND_SIZE]);
	static void readError();
	static void writeError();
	static void quitSignalHandler(int signal);

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
};

#endif /* FUNCTIONARRAY_HPP_ */
