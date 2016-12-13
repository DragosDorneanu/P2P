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
#include <string>

#define HOST "localhost"
#define PASSWORD "password"
#define USER "root"

class DatabaseQuery
{
private:
	MYSQL * database;
	int * client;

public:
	DatabaseQuery(MYSQL * database, int * client)
	{
		this->database = database;
		this->client = client;
	}

	MYSQL * getDatabase() {
		return this->database;
	}

	int * getClient() {
		return this->client;
	}
};

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
	if(mysql_real_connect(databaseConnection, HOST, USER, PASSWORD, NULL, 0, NULL, 0) == NULL)
		databaseConnectionError();
}

MYSQL_RES * query(MYSQL * databaseConnection, std::string sqlInstruction)
{
	if(mysql_query(databaseConnection, sqlInstruction.c_str()))
	{
		perror("Query error");
		return NULL;
	}
	MYSQL_RES * queryResult = mysql_store_result(databaseConnection);
	return queryResult;
}

#endif /* DATABASE_OPERATION_HPP_ */
