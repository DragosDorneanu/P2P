/*
 * ConnectionEncryptor.hpp
 *
 *  Created on: Jan 17, 2017
 *      Author: dragos
 */

#ifndef CONNECTIONENCRYPTOR_HPP_
#define CONNECTIONENCRYPTOR_HPP_

#include <openssl/rsa.h>

class ConnectionEncryptor
{
	RSA * rsa;
	int client;
	char encryptedMessage[4096];
	char * publicKey;
	int encryptedMessageLength;

	void readError();
	bool createRSA();
	void writeError();

public:
	ConnectionEncryptor(int client);
	bool receivePublicKey();
	void checkRSA();
	bool encrypt(char * message, int messageSize);
	bool sendEncryptedMessage();
	virtual ~ConnectionEncryptor();
};

#endif /* CONNECTIONENCRYPTOR_HPP_ */
