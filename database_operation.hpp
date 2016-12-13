/*
 * database_operation.h
 *
 *  Created on: Dec 12, 2016
 *      Author: dragos
 */

#ifndef DATABASE_OPERATION_HPP_
#define DATABASE_OPERATION_HPP_

#include <mysql/my_global.h>
#include <mysql/mysql.h>
#include <cstdio>
#include <cstdlib>
#include <arpa/inet.h>
#include <netinet/in.h>

#define HOST "localhost"
#define PASSWORD "password"
#define USER "root"
#define DB "P2P"

class DatabaseQueryParameters
{
private:
	MYSQL * database;
	int * client;
	struct sockaddr_in clientInfo;

public:
	DatabaseQueryParameters(MYSQL * database, int * client, struct sockaddr_in clientInfo)
	{
		this->database = database;
		this->client = client;
		this->clientInfo = clientInfo;
	}

	MYSQL * getDatabase() {
		return this->database;
	}

	int * getClient() {
		return this->client;
	}

	struct sockaddr_in getClientInfo() {
		return this->clientInfo;
	}
};

void readError()
{
	perror("Read error");
	exit(EXIT_FAILURE);
}

void databaseConnectionError()
{
	perror("Error while connecting to database...");
	exit(EXIT_FAILURE);
}

void connectToDatabase(MYSQL *& databaseConnection)
{
	databaseConnection = mysql_init(NULL);
	if(databaseConnection == NULL)
		databaseConnectionError();
	if(mysql_real_connect(databaseConnection, HOST, USER, PASSWORD, DB, 0, NULL, 0) == NULL)
		databaseConnectionError();
}

MYSQL_RES * query(MYSQL * databaseConnection, char * sqlInstruction)
{
	if(mysql_query(databaseConnection, sqlInstruction))
	{
		perror("Query error");
		return NULL;
	}
	MYSQL_RES * queryResult = mysql_store_result(databaseConnection);
	return queryResult;
}

void updateUserInfo(MYSQL * database, struct sockaddr_in clientInfo, char username[50])
{
	char sqlCommand[100];
	sprintf(sqlCommand, "update Users set status = 'online', ip = '%s' where username = '%s'", inet_ntoa(clientInfo.sin_addr), username);
	query(database, sqlCommand);
}

void dropUserAvailableFile(MYSQL * database, char username[50])
{
	char sqlCommand[100];
	sprintf(sqlCommand, "delete from Files where username = '%s'", username);
	query(database, sqlCommand);
}

void sendDownloadPathToClient(MYSQL * database, int &client, char username[50])
{
	char sqlCommand[100];
	MYSQL_RES * result;
	MYSQL_ROW outputRow;
	sprintf(sqlCommand, "select DownloadPath from Users where username = '%s'", username);
	result = mysql_store_result(database);
	outputRow = mysql_fetch_row(result);
	if(write(client, outputRow[0], sizeof(outputRow[0])) == -1)
	{
		perror("Write error");
		exit(EXIT_FAILURE);
	}
}

void insertUserAvailableFiles(MYSQL * database, int &client, char username[50])
{
	char sqlCommand[100];
	char sizeStr[30], fileName[100], fileHash[64];
	int sizeOfFile, readBytes, sizeOfFileName, sizeOfHash;
	while(true)
	{
		//user must send fileName, sizeOfFile, md5 hash for file
		if((readBytes = read(client, &sizeOfFileName, 4)) == -1)
			readError();
		if(readBytes == 0)
			break;
		if((readBytes = read(client, fileName, sizeOfFileName)) == -1)
			readError();
		if(read(client, &sizeOfFile, 4) == -1)
			readError();
		if(read(client, &sizeOfHash, 4) == -1)
			readError();
		if(read(client, fileHash, sizeOfHash) == -1)
			readError();
		sprintf(sqlCommand, "insert into Files value ('%s', '%s', %d, '%s')", username, fileName, sizeOfFile, fileHash);
		query(database, sqlCommand);
	}
}

#endif /* DATABASE_OPERATION_HPP_ */
