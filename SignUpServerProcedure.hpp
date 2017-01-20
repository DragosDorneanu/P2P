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
#include "ConnectionDecryptor.hpp"

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

void signupFail(int &client, MYSQL * database)
{
	signUpFailMessage(client);
	close(client);
	mysql_close(database);
}

void signUpServerProcedure(MYSQL *& database, ClientThreadParameter * parameters)
{
	MYSQL_RES * queryResult;
	char username[50], password[50], sqlCommand[200];
	ConnectionDecryptor decryptor(parameters->client);

	if(!decryptor.sendPublicKey())
	{
		signupFail(parameters->client, database);
		return;
	}

	if(!decryptor.receiveEncryptedMessage())
	{
		signupFail(parameters->client, database);
		return;
	}
	if(!decryptor.decrypt())
	{
		signupFail(parameters->client, database);
		return;
	}
	strcpy(username, decryptor.getDecryptedMessage());

	if(!decryptor.receiveEncryptedMessage())
	{
		signupFail(parameters->client, database);
		return;
	}

	if(!decryptor.decrypt())
	{
		signupFail(parameters->client, database);
		return;
	}
	strcpy(password, decryptor.getDecryptedMessage());

	sprintf(sqlCommand, "select username from UserInfo where username = '%s'", username);
	queryResult = query(database, sqlCommand);
	if(mysql_num_rows(queryResult) == 0)
		succesfulSignUpProcedure(database, parameters->client, parameters->clientInfo, username, password);
	else
		signUpFailMessage(parameters->client);
	mysql_close(database);
}

#endif /* SIGNUPSERVERPROCEDURE_HPP_ */
