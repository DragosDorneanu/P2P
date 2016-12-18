/*
 * SignInServerProcedure.hpp
 *
 *  Created on: Dec 15, 2016
 *      Author: dragos
 */

#ifndef SIGNINSERVERPROCEDURE_HPP_
#define SIGNINSERVERPROCEDURE_HPP_

#define SIGN_IN_ERROR 4
#define SIGN_IN_SUCCESS 5

#include <cstdio>
#include <cstdlib>
#include "database_operation.hpp"

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
		successfulLoginProcedure(database, client, parameters->getClientInfo(), mysql_fetch_row(queryResult));
	else
		loginFailMessage(client);
}


#endif /* SIGNINSERVERPROCEDURE_HPP_ */
