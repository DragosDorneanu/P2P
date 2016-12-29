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

void readError() {
	perror("Read error");
}

void writeError() {
	perror("Write error");
}

char * getSHA256Hash(char * str)
{
	char * hexPassword = new char[65];
	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha;
	SHA256_Init(&sha);
	SHA256_Update(&sha, str, strlen(str));
	SHA256_Final(hash, &sha);
	for(unsigned int index = 0; index < sizeof(hash); ++index)
		sprintf(hexPassword + 2 * index, "%02x", hash[index]);
	return hexPassword;
}

void databaseConnectionError() {
	perror("Error while connecting to database...");
}

void databaseQueryError() {
	perror("Query error");
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
		databaseQueryError();
	MYSQL_RES * queryResult = mysql_store_result(databaseConnection);
	return queryResult;
}

void updateUserStatus(MYSQL * database, struct sockaddr_in clientInfo, int id)
{
	char sqlCommand[100];
	sprintf(sqlCommand, "update UserStatus set status = 'online', ip = '%s', port = %d where id = %d", inet_ntoa(clientInfo.sin_addr), clientInfo.sin_port, id);
	query(database, sqlCommand);
}

void dropUserAvailableFiles(MYSQL * database, int id)
{
	char sqlCommand[100];
	sprintf(sqlCommand, "delete from Files where id = %d", id);
	query(database, sqlCommand);
}

void insertUserAvailableFiles(MYSQL * database, int &client, int id)
{
	char sqlCommand[512];
	char fileName[100], fileHash[65];
	int sizeOfFile, readBytes, sizeOfFileName, hashSize;

	while((readBytes = read(client, &sizeOfFileName, 4)) > 0 &&
			sizeOfFileName > 0 &&
			(readBytes = read(client, fileName, sizeOfFileName)) > 0 &&
			(readBytes = read(client, &sizeOfFile, 4)) > 0 &&
			(readBytes = read(client, &hashSize, 4)) > 0 &&
			(readBytes = read(client, fileHash, hashSize)) > 0)
	{
		fileName[sizeOfFileName] = '\0';
		fileHash[hashSize] = '\0';
		sprintf(sqlCommand, "insert into Files value (%d, '%s', %d, '%s')", id, fileName, sizeOfFile, fileHash);
		query(database, sqlCommand);
	}
	if(readBytes == -1)
		readError();
}

void insertInUserInfo(MYSQL * database, char * username, char * password)
{
	char sqlCommand[1024];
	sprintf(sqlCommand, "insert into UserInfo value (NULL, '%s', '%s')", username, getSHA256Hash(password));
	query(database, sqlCommand);
}

void insertInUserStatus(MYSQL * database, struct sockaddr_in clientInfo)
{
	char sqlCommand[100];
	sprintf(sqlCommand, "insert into UserStatus value (NULL, 'offline', '%s', %d)", inet_ntoa(clientInfo.sin_addr), clientInfo.sin_port);
	query(database, sqlCommand);
}

#endif /* DATABASE_OPERATION_HPP_ */
