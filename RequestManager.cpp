/*
 * RequestManager.cpp*
 *
 *  Created on: Dec 30, 2016
 *      Author: dragos
 */

#include "RequestManager.hpp"
#include <unistd.h>
#include <cstring>
#include "database_operation.hpp"
#include <fstream>
#include <arpa/inet.h>
#include <netinet/in.h>

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

void RequestManager::downloadProcedure(int &client, MYSQL *& database, sockaddr_in clientInfo)
{
	int ipSize;
	uint16_t port;
	unsigned int idSize, fileNameSize, idValue, userID;
	unsigned long long int fileSize;
	char id[12], sqlCommand[200], getFileSQLCommand[150], getUserIDCommand[150];
	MYSQL_RES * result;
	MYSQL_ROW row;

	if(read(client, &idSize, 4) == -1 || read(client, id, idSize) == -1)
		readError();
	id[idSize] = '\0';
	idValue = atoi(id);

	sprintf(getUserIDCommand, "select id from UserStatus where ip = '%s' and port = %d", inet_ntoa(clientInfo.sin_addr), clientInfo.sin_port);
	sprintf(getFileSQLCommand, "select distinct FileName, size from Files natural join FileID where HashID = %d", idValue);

	result = query(database, getUserIDCommand);
	row = mysql_fetch_row(result);
	userID = atoi(row[0]);
	sprintf(sqlCommand, "select distinct ip, port from UserStatus natural join Files natural join FileID where HashID = %d and status = 'online' and UserStatus.id != %d", idValue, userID);
	result = query(database, getFileSQLCommand);

	if(mysql_num_rows(result))
	{
		row = mysql_fetch_row(result);
		fileNameSize = strlen(row[0]);
		fileSize = atol(row[1]);
		if(write(client, &fileNameSize, 4) == -1 || write(client, row[0], fileNameSize) == -1 || write(client, &fileSize, 8) == -1)
			writeError();

		result = query(database, sqlCommand);

		while((row = mysql_fetch_row(result)))
		{
			ipSize = strlen(row[0]);
			port = atoi(row[1]);
			if(write(client, &ipSize, 4) == -1 || write(client, row[0], ipSize) == -1)
				writeError();
			if(write(client, &port, sizeof(uint16_t)) == -1)
				writeError();
		}
	}
	else
	{
		fileNameSize = -1;
		if(write(client, &fileNameSize, 4) == -1)
			writeError();
		return;
	}
	ipSize = -1;
	if(write(client, &ipSize, 4) == -1)
		writeError();
}

void RequestManager::findProcedure(int &client, MYSQL *& database)
{
	bool searchByName = false;
	int  fileNameSize;
	unsigned int optionCount, option, restrictionSize, size, hashID;
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
		sprintf(sqlCommand, "select distinct FileName, size, HashID from Files natural join FileID where FileName = '%s'", fileName);
	else
		sprintf(sqlCommand, "select distinct FileName, size, HashID from Files natural join FileID where instr(FileName, '%s') > 0", fileName);
	strcat(sqlCommand, conditions);

	result = query(database, sqlCommand);

	while((row = mysql_fetch_row(result)))
	{
		fileNameSize = strlen(row[0]);
		size = atoi(row[1]);
		hashID = atoi(row[2]);
		if(write(client, &fileNameSize, 4) == -1 || write(client, row[0], fileNameSize) == -1 ||
				write(client, &size, 4) == -1 || write(client, &hashID, 4) == -1)
			writeError();
	}
	fileNameSize = -1;
	if(write(client, &fileNameSize, 4) == -1)
		writeError();
}

void RequestManager::quitProcedure(MYSQL *& database, char * clientIP, unsigned int port)
{
	char sqlCommand[200];
	sprintf(sqlCommand, "update UserStatus set status = 'offline' where ip = '%s' and port = %d", clientIP, port);
	query(database, sqlCommand);
}

void RequestManager::receiveRequests(int &client, MYSQL *& database, sockaddr_in clientInfo)
{
	short readStatus, requestType;
	char sqlCommand[200];

	while((readStatus = read(client, &requestType, 2)) > 0)
	{
		switch(requestType)
		{
		case DOWNLOAD : downloadProcedure(client, database, clientInfo); break;
		case FIND : findProcedure(client, database); break;
		case QUIT : quitProcedure(database, inet_ntoa(clientInfo.sin_addr), clientInfo.sin_port); return;
		default : ;
		}
	}

	sprintf(sqlCommand, "update UserStatus set status = 'offline' where ip = '%s' and port = %d", inet_ntoa(clientInfo.sin_addr), clientInfo.sin_port);
	query(database, sqlCommand);
	if(readStatus == -1)
		readError();

}
