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
#include "FunctionArray.hpp"

#define SIGN_IN_ERROR 4
#define SIGN_IN_SUCCESS 5

using namespace std;

void commandPrompt(FunctionArray commandArray)
{
	string commandName;
	char command[MAX_COMMAND_SIZE];
	int functionIndex;

	cout << endl << "p2p> ";
	while(cin >> commandName)
	{
		if((functionIndex = commandArray.exists(commandName, 0, commandArray.size() - 1)) != -1)
		{
			cin.getline(command, MAX_COMMAND_SIZE);
			commandArray.execute(functionIndex, command);
		}
		else cout << "Command does not exist..." << endl;
		cout << "p2p> ";
	}
}

void signInProcedure(int &client)
{
	char username[50], password[50], downloadPath[512];
	unsigned int usernameSize, passwordSize, signinStatus;
	ifstream configFile("path.conf");
	FunctionArray commandArray(client);

	commandArray.setClient(client);
	commandArray.setSignalHandler();
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
		markEndOfFileSharing(client);
		cout << "You have signed in successful!!!" << endl;
		commandPrompt(commandArray);
	}
	else
	{
		cout << "Sign in failed!!! Wrong username or password..." << endl;
		exit(EXIT_FAILURE);
	}
}

#endif /* SIGNINCLIENTPROCEDURE_HPP_ */
