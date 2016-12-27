/*
 * SignInServerProcedure.hpp
 *
 *  Created on: Dec 15, 2016
 *      Author: dragos
 */

#ifndef SIGNINSERVERPROCEDURE_HPP_
#define SIGNINSERVERPROCEDURE_HPP_

#include <cstdio>
#include <cstdlib>
#include "database_operation.hpp"

#define SIGN_IN_ERROR 4
#define SIGN_IN_SUCCESS 5
#define DOWNLOAD 131
#define FIND 132
#define QUIT 135
#define SEARCH_BY_SIZE 20
#define SEARCH_BY_TYPE 21
#define SEARCH_BY_NAME 22

char * getSHA256Hash(char * str);

void loginFailMessage(int &client)
{
	unsigned int signInError = SIGN_IN_ERROR;
	if(write(client, &signInError, 4) == -1)
		writeError();
}

void loginSuccessMessage(int &client)
{
	unsigned int signInSuccess = SIGN_IN_SUCCESS;
	if(write(client, &signInSuccess, 4) == -1)
		writeError();
}

void successfulLoginProcedure(MYSQL * database, int &client, struct sockaddr_in clientInfo, MYSQL_ROW id)
{
	int idValue = atoi(id[0]);
	loginSuccessMessage(client);
	updateUserStatus(database, clientInfo, idValue);
	dropUserAvailableFiles(database, idValue);
	insertUserAvailableFiles(database, client, idValue);
}

void downloadProcedure(int &client, MYSQL * database) { }

void findProcedure(int &client, MYSQL * database)
{
	bool searchByName = false;
	unsigned int optionCount, option, restrictionSize, fileNameSize, columnSize;
	char sqlCommand[1024], restriction[30], sqlCondition[512], *fileName, *end, conditions[] = "\0";
	MYSQL_RES * result;
	MYSQL_ROW row;

	fileName = new char;
	end = new char;
	if(read(client, &optionCount, 4) == -1)
		readError();
	for(unsigned int i = 0; i < optionCount; ++i)
	{
		if(read(client, &option, 4) == -1)
			readError();
		if(option != SEARCH_BY_NAME)
		{
			 if(read(client, &restrictionSize, 4) == -1 && read(client, restriction, restrictionSize) == -1)
				 readError();
			 if(option == SEARCH_BY_TYPE)
			 {
				 strcpy(sqlCondition, " and fileName like '%");
				 strcat(restriction, "'");
			 }
			 else sprintf(sqlCondition, " and size = %ld", strtol(restriction, &end, 10));
			 strcat(conditions, sqlCondition);
		}
		else searchByName = true;
	}
	if(read(client, &fileNameSize, 4) == -1 || read(client, fileName, fileNameSize) == -1)
		readError();
	if(searchByName)
		sprintf(sqlCommand, "select FileName, size, HashValue from Files where fileName = '%s'", fileName);
	else
		sprintf(sqlCommand, "select FileName, size, HashValue from Files where instr(fileName, '%s') > 0", fileName);
	strcat(sqlCommand, conditions);
	result = query(database, sqlCommand);
	while((row = mysql_fetch_row(result)))
	{
		columnSize = strlen(row[0]);
		if(write(client, &columnSize, 4) == -1 || write(client, &row[0], columnSize) == -1 || write(client, &row[1], 4) == -1)
			writeError();
	}
}

void receiveRequests(int &client, MYSQL * database)
{
	unsigned int requestType;
	while(true)
	{
		if(read(client, &requestType, 4) == -1)
		readError();
		switch(requestType)
		{
		case DOWNLOAD : downloadProcedure(client, database); break;
		case FIND : findProcedure(client, database); break;
		case QUIT : return;
		default : ;
		}
	}
}

void signInServerProcedure(DatabaseQueryParameters * parameters)
{
	MYSQL * database = parameters->getDatabase();
	int client = *(parameters->getClient());
	MYSQL_RES * queryResult;
	char username[50], password[50], sqlCommand[200];
	unsigned int sizeOfUsername, sizeOfPassword;

	if(read(client, &sizeOfUsername, 4) == -1 || read(client, username, sizeOfUsername) == -1 ||
			read(client, &sizeOfPassword, 4) == -1 || read(client, password, sizeOfPassword) == -1)
	{
		loginFailMessage(client);
		readError();
	}
	username[sizeOfUsername] = '\0';
	password[sizeOfPassword] = '\0';
	sprintf(sqlCommand, "select id from UserInfo where username = '%s' and password = '%s'", username, getSHA256Hash(password));
	queryResult = query(database, sqlCommand);
	if(mysql_num_rows(queryResult))
	{
		successfulLoginProcedure(database, client, parameters->getClientInfo(), mysql_fetch_row(queryResult));
		receiveRequests(client, database);
	}
	else loginFailMessage(client);
}


#endif /* SIGNINSERVERPROCEDURE_HPP_ */
