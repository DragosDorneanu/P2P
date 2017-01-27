/*
 * ConnectionEncryptor.cpp
 *
 *  Created on: Jan 17, 2017
 *      Author: dragos
 */

#include "ConnectionEncryptor.hpp"

#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <openssl/rsa.h>
#include <openssl/pem.h>

using namespace std;

ConnectionEncryptor::ConnectionEncryptor(int client)
{
	this->client = client;
	publicKey = NULL;
	rsa = NULL;
	encryptedMessageLength = 0;
}

void ConnectionEncryptor::readError()
{
	perror("Read public key error");
	close(client);
	exit(EXIT_FAILURE);
}

bool ConnectionEncryptor::receivePublicKey()
{
	unsigned int publicKeyLength;
	if(read(client, &publicKeyLength, 4) == -1)
		readError();
	publicKey = new char[publicKeyLength + 1];
	if(read(client, publicKey, publicKeyLength) == -1)
		readError();
	publicKey[publicKeyLength] = '\0';
	if(!createRSA())
		return false;
	return true;
}

void ConnectionEncryptor::checkRSA()
{
	if(rsa == NULL)
		cout << "RSA IS NULL" << endl;
	else
		cout << "RSA IS INITIALIZED" << endl;
}

void ConnectionEncryptor::writeError()
{
	perror("Send encrypted message error");
	close(client);
	exit(EXIT_FAILURE);
}

bool ConnectionEncryptor::createRSA()
{
    BIO * publicBio = BIO_new_mem_buf(publicKey, strlen(publicKey));

    if (publicBio == NULL)
    {
        cout << "Failed to create public key BIO" << endl;
        return false;
    }

    rsa = RSA_new();
    rsa = PEM_read_bio_RSA_PUBKEY(publicBio, NULL, NULL, NULL);
    BIO_free_all(publicBio);

    if(rsa == NULL)
    	return false;
    return true;
}

bool ConnectionEncryptor::encrypt(char * message, int messageSize)
{
	if((encryptedMessageLength = RSA_public_encrypt(messageSize, (unsigned char *)message, (unsigned char *)encryptedMessage, rsa, RSA_PKCS1_PADDING)) == -1)
	{
		perror("RSA encrypt error");
		close(client);
		return false;
	}
	return true;
}

bool ConnectionEncryptor::sendEncryptedMessage()
{
	if(write(client, &encryptedMessageLength, 4) == -1 ||
			write(client, encryptedMessage, encryptedMessageLength) == -1)
		writeError();
	return true;
}
ConnectionEncryptor::~ConnectionEncryptor()
{
	RSA_free(rsa);
	delete publicKey;
}

