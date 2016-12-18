/*
 * SignIn.hpp
 *
 *  Created on: Dec 15, 2016
 *      Author: dragos
 */

#ifndef SIGNINCLIENTPROCEDURE_HPP_
#define SIGNINCLIENTPROCEDURE_HPP_

#include <iostream>
#include <termios.h>
#include <cstring>
#include <fstream>
#include <cstdlib>
#include "ValidationProcedures.hpp"

using namespace std;

#define SIGN_IN_ERROR 4
#define SIGN_IN_SUCCESS 5


void signInProcedure(int &client)
{
	char username[50], password[50], downloadPath[512];
	unsigned int usernameSize, passwordSize, signinStatus;
	ifstream configFile("path.conf");

	cin.ignore(1, '\n');
	cout << "Username : ";
	cin.getline(username, 50);
	usernameSize = strlen(username);
	readPasswordInHiddenMode(password, passwordSize);
	sendUserInfoToServer(client, usernameSize, username, passwordSize, password);
	if(read(client, &signinStatus, 4) == -1)
		readError();
	configFile.getline(downloadPath, 512);
	if(signinStatus == SIGN_IN_SUCCESS)
	{
		listDirectory(client, downloadPath);
		cout << "You have signed in successful!!!" << endl;
	}
	else
	{
		cout << "Sign in failed!!! Wrong username or password..." << endl;
		exit(EXIT_FAILURE);
	}
}

#endif /* SIGNINCLIENTPROCEDURE_HPP_ */
