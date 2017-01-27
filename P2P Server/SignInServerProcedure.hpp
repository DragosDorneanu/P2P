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
#include "ConnectionDecryptor.hpp"

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

void signinFail(int &client, MYSQL * database)
{
	loginFailMessage(client);
	close(client);
	mysql_close(database);
}

void signInServerProcedure(MYSQL *& database, ClientThreadParameter * parameters)
{
	MYSQL_RES * queryResult;
	char username[50], password[50], sqlCommand[300];
	ConnectionDecryptor decryptor(parameters->client);

	if(!decryptor.sendPublicKey())
	{
		signinFail(parameters->client, database);
		return;
	}

	if(read(parameters->client, &parameters->clientInfo.sin_port, sizeof(parameters->clientInfo.sin_port)) == -1 ||
			!decryptor.receiveEncryptedMessage())
	{
		signinFail(parameters->client, database);
		return;
	}

	if(!decryptor.decrypt())
	{
		signinFail(parameters->client, database);
		return;
	}
	strcpy(username, decryptor.getDecryptedMessage());

	if(!decryptor.receiveEncryptedMessage())
	{
		signinFail(parameters->client, database);
		return;
	}

	if(!decryptor.decrypt())
	{
		signinFail(parameters->client, database);
		return;
	}
	strcpy(password, decryptor.getDecryptedMessage());

	sprintf(sqlCommand, "select id from UserInfo where username = '%s' and password = '%s' and id in (select id from UserStatus where status = 'offline')",
			username, getSHAHash(password));
	queryResult = query(database, sqlCommand);
	if(mysql_num_rows(queryResult))
	{
		successfulLoginProcedure(database, parameters->client, parameters->clientInfo, mysql_fetch_row(queryResult));
		RequestManager::receiveRequests(parameters->client, database, parameters->clientInfo);
	}
	else loginFailMessage(parameters->client);
	mysql_close(database);
}


#endif /* SIGNINSERVERPROCEDURE_HPP_ */
