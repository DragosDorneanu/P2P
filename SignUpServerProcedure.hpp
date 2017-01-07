/*
 * SignInServerProcedure.hpp
 *
 *  Created on: Dec 15, 2016
 *      Author: dragos
 */

#ifndef SIGNUPSERVERPROCEDURE_HPP_
#define SIGNUPSERVERPROCEDURE_HPP_

#include <cstdio>
#include <cstdlib>
#include "database_operation.hpp"

#define SIGN_UP_ERROR 3
#define SIGN_UP_SUCCESS 6

void signUpFailMessage(int &client)
{
	unsigned int signUpError = SIGN_UP_ERROR;
	if(write(client, &signUpError, 4) == -1)
		writeError();
}

void signUpSuccessMessage(int &client)
{
	unsigned int signUpSuccess = SIGN_UP_SUCCESS;
	if(write(client, &signUpSuccess, 4) == -1)
		writeError();
}

void succesfulSignUpProcedure(MYSQL *& database, int &client, sockaddr_in clientInfo, char * username, char * password)
{
	signUpSuccessMessage(client);
	insertInUserInfo(database, username, password);
	insertInUserStatus(database, clientInfo);
}

void signUpServerProcedure(MYSQL *& database, ClientThreadParameter * parameters)
{
	MYSQL_RES * queryResult;
	char username[50], password[50], sqlCommand[200];
	unsigned int sizeOfUsername, sizeOfPassword;
	//pthread_mutex_t dbLock = PTHREAD_MUTEX_INITIALIZER;

	if(read(parameters->client, &sizeOfUsername, 4) == -1 || read(parameters->client, username, sizeOfUsername) == -1 ||
			read(parameters->client, &sizeOfPassword, 4) == -1 || read(parameters->client, password, sizeOfPassword) == -1)
	{
		signUpFailMessage(parameters->client);
		readError();
	}
	username[sizeOfUsername] = '\0';
	password[sizeOfPassword] = '\0';
	sprintf(sqlCommand, "select username from UserInfo where username = '%s'", username);
	//pthread_mutex_lock(&dbLock);
	queryResult = query(database, sqlCommand);
	//pthread_mutex_unlock(&dbLock);
	if(mysql_num_rows(queryResult) == 0)
	{
		//pthread_mutex_lock(&dbLock);
		succesfulSignUpProcedure(database, parameters->client, parameters->clientInfo, username, password);
		//pthread_mutex_unlock(&dbLock);
	}
	else signUpFailMessage(parameters->client);
	mysql_close(database);
}

#endif /* SIGNUPSERVERPROCEDURE_HPP_ */
