/*
 * ConnectionDecryptor.hpp
 *
 *  Created on: Jan 17, 2017
 *      Author: dragos
 */

#ifndef CONNECTIONDECRYPTOR_HPP_
#define CONNECTIONDECRYPTOR_HPP_

#include <openssl/rsa.h>
#include <string>

using namespace std;

class ConnectionDecryptor
{
	RSA * rsa;
	int client;
	string publicKey;
	string privateKey;
	char encryptedMessage[256], decryptedMessage[4096];
	unsigned int encryptedMessageLength;

	bool createRSA();
	void readError();
	void readPublicKey();
	void readPrivateKey();

public:
	ConnectionDecryptor(int client);
	bool sendPublicKey();
	bool receiveEncryptedMessage();
	bool decrypt();
	char * getDecryptedMessage();
	virtual ~ConnectionDecryptor();
};

#endif /* CONNECTIONDECRYPTOR_HPP_ */
