/*
 * ConnectionDecryptor.cpp
 *
 *  Created on: Jan 17, 2017
 *      Author: dragos
 */

#include "ConnectionDecryptor.hpp"

#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

using namespace std;

ConnectionDecryptor::ConnectionDecryptor(int client)
{
	this->client = client;
	encryptedMessageLength = 0;
	readPublicKey();
	readPrivateKey();
	createRSA();
}

bool ConnectionDecryptor::createRSA()
{
    BIO * publicBio = BIO_new_mem_buf(publicKey.c_str(), publicKey.size());
    BIO * privateBio = BIO_new_mem_buf(privateKey.c_str(), privateKey.size());

    if (publicBio == NULL || privateBio == NULL)
    {
        cout << "Failed to create key BIO" << endl;
        return false;
    }

    rsa = RSA_new();
    rsa = PEM_read_bio_RSAPublicKey(publicBio, NULL, NULL, NULL);
    rsa = PEM_read_bio_RSAPrivateKey(privateBio, NULL, NULL, NULL);
    BIO_free_all(publicBio);
    BIO_free_all(privateBio);

    if(rsa == NULL)
    	return false;
    return true;
}

void ConnectionDecryptor::readPublicKey()
{
	publicKey =
	"-----BEGIN PUBLIC KEY-----\n"\
	"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAu80qDfZoBcLSp/ayu2JH\n"\
	"FcIkFrdGe8nJzrEkPG2ooksHgKl5d8cXbj3SrC0eRjApSwLKoBpzsc7Z5iNQjsWh\n"\
	"xkbDleYGJpm6VIqfst0oama12FkCxFUS6w5cnS5+i69fhEDNCK2/rIlVicZ6WSKK\n"\
	"+n8lcQFNxqabk/9xtGFJOmdt1zMlNpObbG1Wi8sCnl5HJbsE4nddLI5K7D/CvZV1\n"\
	"pbPPuds9/3cjbJ+ATq46qUv5vpUs+kmpVXYAKVNE5ij6JmJ8P0bKS/y5EXQZtEe+\n"\
	"bZ+cQ3sELYGxiRfszbFkxLhMKUIhuAG0m+gPZnwWz1sb5gbwa+3RwUJchrTr30T0\n"\
	"iQIDAQAB\n"\
	"-----END PUBLIC KEY-----";
}

void ConnectionDecryptor::readPrivateKey()
{
	privateKey =
	"-----BEGIN RSA PRIVATE KEY-----\n"\
	"MIIEpAIBAAKCAQEAu80qDfZoBcLSp/ayu2JHFcIkFrdGe8nJzrEkPG2ooksHgKl5\n"\
	"d8cXbj3SrC0eRjApSwLKoBpzsc7Z5iNQjsWhxkbDleYGJpm6VIqfst0oama12FkC\n"\
	"xFUS6w5cnS5+i69fhEDNCK2/rIlVicZ6WSKK+n8lcQFNxqabk/9xtGFJOmdt1zMl\n"\
	"NpObbG1Wi8sCnl5HJbsE4nddLI5K7D/CvZV1pbPPuds9/3cjbJ+ATq46qUv5vpUs\n"\
	"+kmpVXYAKVNE5ij6JmJ8P0bKS/y5EXQZtEe+bZ+cQ3sELYGxiRfszbFkxLhMKUIh\n"\
	"uAG0m+gPZnwWz1sb5gbwa+3RwUJchrTr30T0iQIDAQABAoIBAQCe5u2aes9xcHMF\n"\
	"o3t/iZxUELywa8q7mvWiacgbST3SdGGYv80DR/XPVYgYHuTqxn1p9qaz6S3TWQ6N\n"\
	"53uWEirbtaxv/P1fU9uRBOozUWWNAfMDZ+0rakBjmvdKF7kDQBQ1tw7FNR5lQp6Z\n"\
	"wNJs8QlfAo4Pm+bPGtGw3Aa9dUOcoctb/w0RzQr8KuyTSpr0602z3s/R/P/d7c6b\n"\
	"pogC8anJF2cZF6HULdXQL0hcXa4Yg7kXe3W8KgG+Ccrxy+8trsQByibTLE5kFu8U\n"\
	"SYsI1y357hKhXBQ5t0zKnWyISgPg0ooETkSG/jrOkZYLlyTZSAsiSVhooOFllHCq\n"\
	"W2iW+4AxAoGBAOi9ZNJXSQTkPSLX2t7EOa0R5CQ8cZC21BNwAKXH7R+bXqwIE9l6\n"\
	"cRTkyFQr/urCZw3ovi2CHSgcZOBEyh6drrS9ai7TqSdVqboZbrGFzgJXT1YCtPC4\n"\
	"Vh/2kEJdRXFXBDe5mBCGyVLU7bfoKeta4F8wdQmpBXPwx8NUIZ2tFU8/AoGBAM6S\n"\
	"B5oE+LkFjEpjTIkfybKycfEGSMlJ4WGspErODs2K1b0cdxsK6CHIIrKSmbHuFnWP\n"\
	"wdknQVN7O/ZO6FIrmwqOE9VA1UTIGLz97XDQFVtRZ+iq9ix4rlYVTIWSf7pHwyKk\n"\
	"ZzJHPOqXXqFMXSMDjZugCS6a50k7PkbXxpK2wZI3AoGAO67FugLUIML+wn6kC9g6\n"\
	"Ch26aWhunvOjctCX+etI7YAESQ1ROcXiVb9Nd/hISJ9Nmg2DJ1xkabLOU4yQYEWD\n"\
	"Van1Hsj8aa/X9HctIaz0ZTlKiP3stIYw38V0vO/6LNRpqniXfDV0Tx93Tl/k0Avf\n"\
	"2cktWirT0SV/tKFQdYS7IDUCgYBw+QhTQZjNh608GaaH7+ydYF5UVP2FqHQsuQB5\n"\
	"MMyDuQRAEstI7OQbZZskUmzgFLgPv3liy8c8Ys9eW7VPCy4VzbInFPgOT+jHo5Ax\n"\
	"/HtRn+nWI3nSxIWD6s4Y1xAnzot37LXLl45HNlW4nUzqk0zk6ddbt7f6Iv5iPCwL\n"\
	"nQusbQKBgQDZfSlas39alK5JHc00SIFTgIXuRAy56eU7T6Fu8bwjt6eIWjivnIMC\n"\
	"XfdRxd8K0WjGq5eNH++e4hOQEGmbFTFL7gckITHfa6Ouj/bQwJCd3ktMelsY8Ur6\n"\
	"javsxnDcPIrXyXURiTC04yg4ay2mtiSflcrOxuHMJuHzOni5zOC3dw==\n"\
	"-----END RSA PRIVATE KEY-----";
}

bool ConnectionDecryptor::sendPublicKey()
{
	unsigned int publicKeyLength = publicKey.size();
	if(write(client, &publicKeyLength, 4) == -1 ||
			write(client, publicKey.c_str(), publicKeyLength) == -1)
	{
		perror("Error while sending public key");
		return false;
	}
	return true;
}

void ConnectionDecryptor::readError()
{
	perror("Error while reading encrypted message");
	close(client);
}

bool ConnectionDecryptor::receiveEncryptedMessage()
{
	if(read(client, &encryptedMessageLength, 4) == -1)
	{
		readError();
		return false;
	}
	if(read(client, encryptedMessage, encryptedMessageLength) == -1)
	{
		readError();
		return false;
	}
	encryptedMessage[encryptedMessageLength] = '\0';
	return true;
}

bool ConnectionDecryptor::decrypt()
{
	int decryptedMessageLength;
	if((decryptedMessageLength = RSA_private_decrypt(encryptedMessageLength, (unsigned char *)encryptedMessage, (unsigned char *)decryptedMessage, rsa, RSA_PKCS1_PADDING)) == -1)
	{
		char err[1000];
		ERR_load_crypto_strings();
		ERR_error_string(ERR_get_error(), err);
		cout << err << endl;
		perror("RSA decrypt error");
		close(client);
		return false;
	}
	decryptedMessage[decryptedMessageLength] = '\0';
	return true;
}

char * ConnectionDecryptor::getDecryptedMessage() {
	return decryptedMessage;
}

ConnectionDecryptor::~ConnectionDecryptor()
{
	RSA_free(rsa);
	publicKey.clear();
	privateKey.clear();
}
