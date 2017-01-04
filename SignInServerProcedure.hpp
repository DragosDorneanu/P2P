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

void successfulLoginProcedure(MYSQL * database, int &client, struct sockaddr_in clientInfo, MYSQL_ROW id)
{
	int idValue = atoi(id[0]);
	loginSuccessMessage(client);
	updateUserStatus(database, clientInfo, idValue);
	dropUserAvailableFiles(database, idValue);
	insertUserAvailableFiles(database, client, idValue);
}

void signInServerProcedure(DatabaseQueryParameters * parameters)
{
	MYSQL * database = parameters->getDatabase();
	int client = *(parameters->getClient());
	MYSQL_RES * queryResult;
	char username[50], password[50], sqlCommand[300];
	unsigned int sizeOfUsername, sizeOfPassword;
	pthread_mutex_t dbLock;
	sockaddr_in clientInfo = parameters->getClientInfo();

	if(read(client, &clientInfo.sin_port, sizeof(clientInfo.sin_port)) == -1 ||
			read(client, &sizeOfUsername, 4) == -1 || read(client, username, sizeOfUsername) == -1 ||
			read(client, &sizeOfPassword, 4) == -1 || read(client, password, sizeOfPassword) == -1)
	{
		loginFailMessage(client);
		readError();
	}
	username[sizeOfUsername] = '\0';
	password[sizeOfPassword] = '\0';
	sprintf(sqlCommand, "select id from UserInfo where username = '%s' and password = '%s' and id in (select id from UserStatus where status = 'offline')",
			username, getSHA256Hash(password));
	queryResult = query(database, sqlCommand);
	if(mysql_num_rows(queryResult))
	{
		dbLock = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_lock(&dbLock);
		successfulLoginProcedure(database, client, clientInfo, mysql_fetch_row(queryResult));
		pthread_mutex_unlock(&dbLock);
		RequestManager::receiveRequests(client, database, clientInfo);
		close(client);
	}
	else loginFailMessage(client);
}


#endif /* SIGNINSERVERPROCEDURE_HPP_ */
