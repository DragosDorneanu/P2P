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
#include "RequestManager.hpp"

#define SIGN_IN_ERROR 4
#define SIGN_IN_SUCCESS 5

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

void successfulLoginProcedure(MYSQL *& database, int &client, struct sockaddr_in clientInfo, MYSQL_ROW id)
{
	int idValue = atoi(id[0]);

	loginSuccessMessage(client);
	updateUserStatus(database, clientInfo, idValue);
	dropUserAvailableFiles(database, idValue);
	insertUserAvailableFiles(database, client, idValue);
}

void signInServerProcedure(MYSQL *& database, ClientThreadParameter * parameters)
{
	MYSQL_RES * queryResult;
	char username[50], password[50], sqlCommand[300];
	unsigned int sizeOfUsername, sizeOfPassword;
	//pthread_mutex_t dbLock = PTHREAD_MUTEX_INITIALIZER;

	if(read(parameters->client, &parameters->clientInfo.sin_port, sizeof(parameters->clientInfo.sin_port)) == -1 ||
			read(parameters->client, &sizeOfUsername, 4) == -1 || read(parameters->client, username, sizeOfUsername) == -1 ||
			read(parameters->client, &sizeOfPassword, 4) == -1 || read(parameters->client, password, sizeOfPassword) == -1)
	{
		loginFailMessage(parameters->client);
		readError();
	}
	username[sizeOfUsername] = '\0';
	password[sizeOfPassword] = '\0';
	sprintf(sqlCommand, "select id from UserInfo where username = '%s' and password = '%s' and id in (select id from UserStatus where status = 'offline')",
			username, getSHA256Hash(password));

	//pthread_mutex_lock(&dbLock);
	queryResult = query(database, sqlCommand);
	//pthread_mutex_unlock(&dbLock);

	if(mysql_num_rows(queryResult))
	{
		//pthread_mutex_lock(&dbLock);
		successfulLoginProcedure(database, parameters->client, parameters->clientInfo, mysql_fetch_row(queryResult));
		//pthread_mutex_unlock(&dbLock);
		RequestManager::receiveRequests(parameters->client, database, parameters->clientInfo);
	}
	else loginFailMessage(parameters->client);
	mysql_close(database);
}


#endif /* SIGNINSERVERPROCEDURE_HPP_ */
