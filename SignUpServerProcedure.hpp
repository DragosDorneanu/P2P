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

char * getSHA256Hash(char * str);

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

void succesfulSignUpProcedure(MYSQL * database, int &client, struct sockaddr_in clientInfo, char username[50], char password[50], char downloadPath[512], char * id)
{
	signUpSuccessMessage(client);
	insertInUserInfo(database, username, getSHA256Hash(password), downloadPath);
	insertInUserStatus(database, clientInfo);
	insertUserAvailableFiles(database, client, id);
}

void signUpServerProcedure(DatabaseQueryParameters * parameters)
{
	MYSQL * database = parameters->getDatabase();
	int client = *(parameters->getClient());
	MYSQL_RES * queryResult;
	MYSQL_ROW outputRow;
	char username[50], password[50], sqlCommand[200], downloadPath[512];
	unsigned int sizeOfUsername, sizeOfPassword, sizeOfPath;

	if(read(client, &sizeOfUsername, 4) == -1 || read(client, username, sizeOfUsername) == -1 || read(client, &sizeOfPassword, 4) == -1 || read(client, password, sizeOfPassword) == -1 || read(client, &sizeOfPath, 4) == -1 || read(client, downloadPath, sizeOfPath) == -1)
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
		outputRow = mysql_fetch_row(queryResult);
		succesfulSignUpProcedure(database, client, parameters->getClientInfo(), username, password, downloadPath, outputRow[0]);
	}
	else signUpFailMessage(client);
}

#endif /* SIGNUPSERVERPROCEDURE_HPP_ */
