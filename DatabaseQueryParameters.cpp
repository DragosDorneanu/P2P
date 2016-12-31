/*
 * DatabaseQueryParameters.cpp
 *
 *  Created on: Dec 30, 2016
 *      Author: dragos
 */

#include "DatabaseQueryParameters.hpp"

DatabaseQueryParameters::DatabaseQueryParameters(MYSQL * database, int * client, struct sockaddr_in clientInfo)
{
	this->database = database;
	this->client = client;
	this->clientInfo = clientInfo;
}

MYSQL * DatabaseQueryParameters::getDatabase() {
	return this->database;
}

int * DatabaseQueryParameters::getClient() {
	return this->client;
}

sockaddr_in DatabaseQueryParameters::getClientInfo() {
	return this->clientInfo;
}

DatabaseQueryParameters::~DatabaseQueryParameters() { }

