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

void succesfulSignUpProcedure(MYSQL * database, int &client, struct sockaddr_in clientInfo, char * username, char * password)
{
	char sqlCommand[200];
	MYSQL_RES * queryResult;
	MYSQL_ROW id;

	signUpSuccessMessage(client);
	insertInUserInfo(database, username, password);
	insertInUserStatus(database, clientInfo);
	sprintf(sqlCommand, "select id from UserInfo where username = '%s'", username);
	queryResult = query(database, sqlCommand);
	id = mysql_fetch_row(queryResult);
	insertUserAvailableFiles(database, client, id[0]);
}

void signUpServerProcedure(DatabaseQueryParameters * parameters)
{
	MYSQL * database = parameters->getDatabase();
	int client = *(parameters->getClient());
	MYSQL_RES * queryResult;
	char * username = new char[50], * password = new char[50], sqlCommand[200];
	unsigned int sizeOfUsername, sizeOfPassword;
	pthread_mutex_t dbLock;

	if(read(client, &sizeOfUsername, 4) == -1 || read(client, username, sizeOfUsername) == -1 ||
			read(client, &sizeOfPassword, 4) == -1 || read(client, password, sizeOfPassword) == -1)
	{
		signUpFailMessage(client);
		readError();
	}
	printf("USERNAME : %s\n", username);
	printf("PASSWORD : %s\n", password);
	sprintf(sqlCommand, "select username from UserInfo where username = '%s'", username);
	queryResult = query(database, sqlCommand);
	if(mysql_num_rows(queryResult) == 0)
	{
		pthread_mutex_lock(&dbLock);
		succesfulSignUpProcedure(database, client, parameters->getClientInfo(), username, password);
		pthread_mutex_unlock(&dbLock);
	}
	else signUpFailMessage(client);
}

#endif /* SIGNUPSERVERPROCEDURE_HPP_ */
