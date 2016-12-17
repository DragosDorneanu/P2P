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

#define SIGN_UP_ERROR 3
#define SIGN_UP_SUCCESS 6

using namespace std;

void readError();

void writeError();

void readPasswordInHiddenMode(char password[50], unsigned int &size)
{
	struct termios oldTerminal, newTerminal;
	tcgetattr(fileno(stdin), &oldTerminal);
	newTerminal = oldTerminal;
	newTerminal.c_lflag &= ~(ECHO);
	cout << "Password : ";
	tcsetattr(fileno(stdin), TCSANOW, &newTerminal);
	cin.getline(password, 50);
	tcsetattr(fileno(stdin), TCSANOW, &oldTerminal);
	cout << endl;
	size = strlen(password);
}

void readDownloadPath(char downloadPath[512])
{
	ofstream configFile("path.conf");
	cout << "Valid path to a download/upload directory : ";
	cin.getline(downloadPath, 512);
	configFile << downloadPath;
	configFile.close();
}

void sendUserInfoToServer(int &client, unsigned int userSize, char username[50], unsigned int passwordSize, char password[50])
{
	if(write(client, &userSize, 4) == -1 || write(client, username, userSize) == -1)
		writeError();
	if(write(client, &passwordSize, 4) == -1 || write(client, password, passwordSize) == -1)
		writeError();
}

bool statFile(struct stat &fileStatus, char path[512])
{
	if(stat(path, &fileStatus) == -1)
	{
		perror("Stat error");
		return false;
	}
	return true;
}

bool hashFile(char * file, char fileHash[65])
{
	unsigned char shaData[65];
	char data[1025];
	unsigned int bytes;
	SHA256_CTX sha;
	FILE * toHashFile = fopen(file, "rb");

	if(toHashFile == NULL)
		return false;
	SHA256_Init(&sha);
	while((bytes = fread(data, 1, 1024, toHashFile)) != 0)
		SHA256_Update(&sha, data, bytes);
	SHA256_Final(shaData, &sha);
	for(bytes = 0; bytes < sizeof(shaData); ++bytes)
		sprintf(fileHash + 2 * bytes, "%02x", shaData[bytes]);
	return true;
}

bool isHiddenFile(struct dirent * file) {
	return file->d_name[0] == '.';
}

bool isDirectory(struct dirent * file) {
	return file->d_type == DT_DIR;
}

bool listDirectory(int &client, char * currentDirectory)
{
	DIR * directory;
	dirent * file;
	unsigned int size;
	struct stat fileStatus;
	char newPath[512], fileHash[65];

	if((directory = opendir(currentDirectory)) != NULL)
	{
		while((file = readdir(directory)) != NULL)
		{
			if(!isHiddenFile(file))
			{
				size = strlen(file->d_name);
				if(write(client, &size, 4) == -1 || write(client, file->d_name, size) == -1)
					writeError();
				sprintf(newPath, currentDirectory, "%s/%s", file->d_name);
				if(!statFile(fileStatus, newPath))
					return false;
				if(write(client, &fileStatus.st_size, 4) == -1)
					writeError();
				if(!hashFile(newPath, fileHash))
					return false;
				if(write(client, fileHash, 64) == -1)
					writeError();
				if(isDirectory(file))
				{
					if(!listDirectory(client, newPath))
					{
						closedir(directory);
						return false;
					}
				}
			}
		}
		closedir(directory);
		return true;
	}
	return false;
}

bool sendAvailableFilesToServer(int &client, char downloadPath[512])
{
	if(!listDirectory(client, downloadPath))
		return false;
	return true;
}

void signUpProcedure(int &client)
{
	char * username = new char[50], * password = new char[50], * downloadPath = new char[512];
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
		if(!sendAvailableFilesToServer(client, downloadPath))
		{
			cout << "Invalid download path or error while sharing available files. Other information were successful saved." << endl;
			exit(EXIT_FAILURE);
		}
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
