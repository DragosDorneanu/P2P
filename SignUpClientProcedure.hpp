/*
 * SignUp.hpp
 *
 *  Created on: Dec 15, 2016
 *      Author: dragos
 */

#ifndef SIGNUPCLIENTPROCEDURE_HPP_
#define SIGNUPCLIENTPROCEDURE_HPP_

#include <iostream>
#include <fstream>
#include <cstring>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstdio>
#include <openssl/sha.h>
#include <cstdlib>
#include <fcntl.h>
#include "ValidationProcedures.hpp"

#define SIGN_UP_ERROR 3
#define SIGN_UP_SUCCESS 6

using namespace std;

void readDownloadPath(char downloadPath[512])
{
	ofstream configFile("path.conf");
	cout << "Valid path to a download/upload directory : ";
	cin.getline(downloadPath, 512);
	configFile << downloadPath;
	configFile.close();
}

void signUpProcedure(int &client)
{
	char username[50], password[50], downloadPath[512];
	unsigned int sizeOfUsername, sizeOfPassword, signUpStatus;

	cin.ignore(1, '\n');
	cout << "Enter username : ";
	cin.getline(username, 50);
	sizeOfUsername = strlen(username);
	readPasswordInHiddenMode(password, sizeOfPassword);
	sendUserInfoToServer(client, sizeOfUsername, username, sizeOfPassword, password);
	readDownloadPath(downloadPath);

	if(read(client, &signUpStatus, 4) == -1)
		readError();
	if(signUpStatus == SIGN_UP_SUCCESS)
	{
		cout << "You have signed up successful!!!" << endl;
		exit(EXIT_SUCCESS);
	}
	else
	{
		cout << "Sign up failed!!! User already exists..." << endl;
		exit(EXIT_FAILURE);
	}
}

#endif /* SIGNUPCLIENTPROCEDURE_HPP_ */
