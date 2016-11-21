/*
 * cipher.c
 *
 *  Created on: Nov 20, 2016
 *      Author: adigrabow
 */
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#define BYTE 1
#define SIZE 2048
#define DIRECTORY "."
#define PARENTDIR ".."


int main(int argc, char** argv) {

	if (argc != 4) {
		printf("Wrong number of arguments. expected 3, received %d\n", argc);
		return -1; //TODO add appropriate error msg
	}

	ssize_t numOfFileBytesRead;
	ssize_t numOfKeyBytesRead;
	ssize_t numOfBytesWrite;
	char inBuffer[1];
	char keyBuffer[1];
	DIR* inDirectory; //input directory stream
	DIR* outDirectory; //input directory stream
	struct dirent* directoryEntry; //a directory is consists by directory entries. (this is the type readdear() returns)
	int keyFileDescriptor; //a pointer to the key file

	char* inputDirectoryPath = argv[1];
	char* keyFilePath = argv[2];
	char* outputDirectoryPath = argv[3];

	printf("inputDirectoryPath = %s\nkeyFilePath = %s\noutputDirectoryPath = %s\n",inputDirectoryPath,keyFilePath,outputDirectoryPath);


	/*try to open the input directory stream*/
	if ((inDirectory = opendir(inputDirectoryPath)) == NULL) {
		printf("Error opening directory: %s\n", inputDirectoryPath);
		return -1; //TODO add appropriate error msg
	}

	/*try to open the output directory stream*/
	if ((outDirectory = opendir(outputDirectoryPath)) == NULL) {
		printf("Error opening directory: %s\n", outputDirectoryPath);
		closedir(inDirectory);
		return -1; //TODO add appropriate error msg
	}

	/*try to open the key file*/
	keyFileDescriptor = open(keyFilePath, O_RDONLY);
	if( keyFileDescriptor < 0 ) {
		printf( "Error opening file: %s\n", keyFilePath);
		closedir(inDirectory);
		closedir(outDirectory);
		return 2;//TODO add appropriate error msg
	}

	/****************************************************************
	 * inDirectory loop - go through all the files in the inDirectory
	 * **************************************************************/
	while ( (directoryEntry = readdir(inDirectory)) != NULL) {

		/*make sure we are not reading a directory*/

		if ((strcmp(directoryEntry->d_name, DIRECTORY) == 0) || (strcmp(directoryEntry->d_name, PARENTDIR) == 0) ) {
			continue;
		}

		printf("currently working on the file %s\n",directoryEntry->d_name);

		/*open key file*/
		keyFileDescriptor = open(keyFilePath, O_RDONLY);
		if( keyFileDescriptor < 0 ) {
			printf( "Error opening file: %s\n", keyFilePath);
			closedir(inDirectory);
			closedir(outDirectory);
			return 2;//TODO add appropriate error msg
		}

		int inFileDescriptor;
		int outFileDescriptor;


		/*prepare output and input file path*/
		char outFilePath[SIZE];
		sprintf(outFilePath,"%s%s" ,outputDirectoryPath,directoryEntry->d_name );
		char inFilePath[SIZE];
		sprintf(inFilePath,"%s%s" ,inputDirectoryPath,directoryEntry->d_name );



		/* open input and output files*/
		inFileDescriptor = open(inFilePath, O_RDONLY);
		if (inFileDescriptor < 0) {
			printf( "Error opening file: %s\n",directoryEntry->d_name) ;
			close(keyFileDescriptor);
			closedir(inDirectory);
			closedir(outDirectory);
			return -1;
		}

		outFileDescriptor = open(outFilePath, O_WRONLY | O_CREAT, 0644);
		if (outFileDescriptor < 0) {
			printf( "Error opening file: %s\n",directoryEntry->d_name) ;
			close(inFileDescriptor);
			close(keyFileDescriptor);
			closedir(inDirectory);
			closedir(outDirectory);
			return -1;
		}

		/**********************
		 * read file byte-byte
		 * ********************/
		while((numOfFileBytesRead = read (inFileDescriptor, &inBuffer, 1)) > 0) {
			/*get key's next byte*/
			numOfKeyBytesRead = read (keyFileDescriptor, &keyBuffer, 1);

			/*if we reached the end of key file before we finished input file, re-open key file*/
			if (numOfKeyBytesRead == 0) {
				keyFileDescriptor = open(keyFilePath, O_RDONLY);
				if( keyFileDescriptor < 0 ) {
					printf( "Error opening file: %s\n", inputDirectoryPath);
					close(inFileDescriptor);
					closedir(inDirectory);
					closedir(outDirectory);
					return 2;//TODO add appropriate error msg

				}
			}
			numOfKeyBytesRead = read (keyFileDescriptor, &keyBuffer, 1);

			/*make XOR calc between the 2 bytes and write it to output file*/
			char xor[BYTE];
			xor[0] = inBuffer[0] ^ keyBuffer[0];
			numOfBytesWrite = write (outFileDescriptor, xor, (ssize_t) numOfFileBytesRead);
			if(numOfBytesWrite != numOfFileBytesRead){
				perror("write"); //TODO add appropriate error msg
				return 4;
			}

		}

		close(outFileDescriptor);
		close(inFileDescriptor);

	}

	closedir(inDirectory);
	closedir(outDirectory);
	printf("Done!\n");
	return 0;
}
