/*
 * RequestManager.hpp
 *
 *  Created on: Dec 30, 2016
 *      Author: dragos
 */

#ifndef REQUESTMANAGER_HPP_
#define REQUESTMANAGER_HPP_

#include <mysql/mysql.h>
#include <mysql/my_global.h>

class RequestManager
{
private:
	static void downloadProcedure(int &client, MYSQL * database, sockaddr_in clientInfo);
	static void findProcedure(int &client, MYSQL * database);
	static void quitProcedure(MYSQL * database, char * clientIP, unsigned int port);

public:
	RequestManager();
	static void receiveRequests(int &client, MYSQL * database, sockaddr_in clientInfo);
	virtual ~RequestManager();
};

#endif /* REQUESTMANAGER_HPP_ */
