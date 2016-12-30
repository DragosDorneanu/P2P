/*
 * DatabaseQueryParameters.h
 *
 *  Created on: Dec 30, 2016
 *      Author: dragos
 */

#ifndef DATABASEQUERYPARAMETERS_HPP_
#define DATABASEQUERYPARAMETERS_HPP_

#include <mysql/my_global.h>
#include <mysql/mysql.h>

class DatabaseQueryParameters
{
private:
	MYSQL * database;
	int * client;
	struct sockaddr_in clientInfo;

public:
	DatabaseQueryParameters(MYSQL * database, int * client, struct sockaddr_in clientInfo);
	virtual ~DatabaseQueryParameters();
	MYSQL * getDatabase();
	int * getClient();
	struct sockaddr_in getClientInfo();
};

#endif /* DATABASEQUERYPARAMETERS_HPP_ */
