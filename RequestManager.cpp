/*
 * RequestManager.cpp*
 *
 *  Created on: Dec 30, 2016
 *      Author: dragos
 */

#include "RequestManager.hpp"
#include <unistd.h>
#include "database_operation.hpp"

#define DOWNLOAD 131
#define FIND 132
#define QUIT 135
#define SEARCH_BY_SIZE 20
#define SEARCH_BY_TYPE 21
#define SEARCH_BY_NAME 22
#define SEARCH_BY_BIGGER_SIZE 23
#define SEARCH_BY_SMALLER_SIZE 24

RequestManager::RequestManager() { }

RequestManager::~RequestManager() { }

void RequestManager::downloadProcedure(int &client, MYSQL * database) { }

void RequestManager::findProcedure(int &client, MYSQL * database)
{
	bool searchByName = false;
	int  fileNameSize;
	unsigned int optionCount, option, restrictionSize, size;
	char sqlCommand[1024], restriction[100], sqlCondition[512], fileName[1024], conditions[512];
	MYSQL_RES * result;
	MYSQL_ROW row;

	conditions[0] = '\0';
	if(read(client, &optionCount, 4) == -1)
		readError();
	for(unsigned int i = 0; i < optionCount; ++i)
	{
		if(read(client, &option, 4) == -1)
			readError();
		if(option != SEARCH_BY_NAME)
		{
			 if(read(client, &restrictionSize, 4) == -1 || read(client, restriction, restrictionSize) == -1)
				 readError();
			 restriction[restrictionSize] = '\0';
			 if(option == SEARCH_BY_TYPE)
				 sprintf(sqlCondition, "%s%s%s", " and FileName like '\%", restriction, "'");
			 else if(option == SEARCH_BY_SIZE)
				 sprintf(sqlCondition, " and size = %d", atoi(restriction));
			 else if(option == SEARCH_BY_BIGGER_SIZE)
				 sprintf(sqlCondition, " and size > %d", atoi(restriction));
			 else
				 sprintf(sqlCondition, " and size < %d", atoi(restriction));
			 if(conditions[0] == '\0')
				 strcpy(conditions, sqlCondition);
			 else
				 strcat(conditions, sqlCondition);
			 sqlCondition[0] = '\0';
			 restriction[0] = '\0';
		}
		else searchByName = true;
	}
	if(read(client, &fileNameSize, 4) == -1 || read(client, fileName, fileNameSize) == -1)
		readError();
	fileName[fileNameSize] = '\0';
	if(searchByName)
		sprintf(sqlCommand, "select distinct FileName, size, HashValue from Files where FileName = '%s'", fileName);
	else
		sprintf(sqlCommand, "select distinct FileName, size, HashValue from Files where instr(FileName, '%s') > 0", fileName);
	strcat(sqlCommand, conditions);
	result = query(database, sqlCommand);
	while((row = mysql_fetch_row(result)))
	{
		fileNameSize = strlen(row[0]);
		size = strlen(row[1]);
		if(write(client, &fileNameSize, 4) == -1 || write(client, row[0], fileNameSize) == -1 ||
				write(client, &size, 4) == -1 || write(client, row[1], size) == -1)
			writeError();
	}
	fileNameSize = -1;
	if(write(client, &fileNameSize, 4) == -1)
		writeError();
}

void RequestManager::quitProcedure(MYSQL * database, char * clientIP)
{
	char sqlCommand[200];
	sprintf(sqlCommand, "update UserStatus set status = 'offline' where ip = '%s'", clientIP);
	query(database, sqlCommand);
}

void RequestManager::receiveRequests(int &client, MYSQL * database, sockaddr_in clientInfo)
{
	short readStatus, requestType;
	char sqlCommand[200];

	while((readStatus = read(client, &requestType, 2)) > 0)
	{
		switch(requestType)
		{
		case DOWNLOAD : downloadProcedure(client, database); break;
		case FIND : findProcedure(client, database); break;
		case QUIT : quitProcedure(database, inet_ntoa(clientInfo.sin_addr)); return;
		default : ;
		}
	}
	sprintf(sqlCommand, "update UserStatus set status = 'offline' where ip = '%s'", inet_ntoa(clientInfo.sin_addr));
	query(database, sqlCommand);
	if(readStatus == -1)
		readError();

}
