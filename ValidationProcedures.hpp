/*
 * ValidationProcedures.hpp
 *
 *  Created on: Dec 18, 2016
 *      Author: dragos
 */

#ifndef VALIDATIONPROCEDURES_HPP_
#define VALIDATIONPROCEDURES_HPP_

#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdio>
#include <openssl/sha.h>
#include <cstring>
#include <cstdlib>
#include <sys/file.h>

using namespace std;

void readError()
{
	perror("Read Error");
	exit(EXIT_FAILURE);
}

void writeError()
{
	perror("Write Error");
	exit(EXIT_FAILURE);
}

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
	unsigned char shaData[SHA256_DIGEST_LENGTH];
	char data[5121];
	int bytes, toHashFile;
	SHA256_CTX sha;

	if((toHashFile = open(file, O_RDONLY, 0600)) == -1)
		return false;
	flock(toHashFile, LOCK_EX);
	SHA256_Init(&sha);
	while((bytes = read(toHashFile, data, 5120)) > 0)
		SHA256_Update(&sha, data, bytes);
	if(bytes == -1)
		readError();
	flock(toHashFile, LOCK_UN);
	SHA256_Final(shaData, &sha);
	for(bytes = 0; bytes < SHA256_DIGEST_LENGTH; ++bytes)
		sprintf(fileHash + 2 * bytes, "%02x", shaData[bytes]);
	return true;
}

bool isHiddenFile(struct dirent * file) {
	return file->d_name[0] == '.';
}

bool isDirectory(struct dirent * file) {
	return file->d_type == DT_DIR;
}

bool isFifo(struct dirent * file) {
	return file->d_type == DT_FIFO;
}

bool listDirectory(int &client, char * currentDirectory)
{
	DIR * directory;
	dirent * file;
	unsigned int size, hashSize;
	struct stat fileStatus;
	char newPath[512], fileHash[65];

	if((directory = opendir(currentDirectory)) == NULL)
		return false;
	while((file = readdir(directory)) != NULL)
	{
		if(!isHiddenFile(file))
		{
			sprintf(newPath, "%s/%s", currentDirectory, file->d_name);
			if(isDirectory(file))
			{
				if(!listDirectory(client, newPath))
				{
					closedir(directory);
					return false;
				}
			}
			else if(!isFifo(file))
			{
				size = strlen(file->d_name);
				if(write(client, &size, 4) == -1 || write(client, file->d_name, size) == -1)
					writeError();
				if(!statFile(fileStatus, newPath))
					return false;
				if(write(client, &fileStatus.st_size, 4) == -1)
					writeError();
				if(!hashFile(newPath, fileHash))
					return false;
				hashSize = strlen(fileHash);
				if(write(client, &hashSize, 4) == -1 || write(client, fileHash, hashSize) == -1)
					writeError();
			}
		}
	}
	closedir(directory);
	return true;
}

void markEndOfFileSharing(int &client)
{
	int endOfFileSharing = -1;
	if(write(client, &endOfFileSharing, 4) == -1)
		writeError();
}

bool sendAvailableFilesToServer(int &client, char downloadPath[512])
{
	if(!listDirectory(client, downloadPath))
		return false;
	return true;
}


#endif /* VALIDATIONPROCEDURES_HPP_ */
