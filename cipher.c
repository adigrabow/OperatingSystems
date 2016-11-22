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
		printf("Wrong number of arguments. expected 3, received %s\n", strerror(errno));
		return 1;
	}


	ssize_t numOfFileBytesRead;
	ssize_t numOfKeyBytesRead;
	ssize_t numOfBytesWrite;
	char inBuffer[1];
	char keyBuffer[1];
	DIR* inDirectory; //input directory stream
	DIR* outDirectory; //output directory stream
	struct dirent* directoryEntry; //this is the type readdear() returns
	int keyFileDescriptor; //a pointer to the key file
	int numOfTimesKeyFileWasOpen = 0;
	int inFileDescriptor;
	int outFileDescriptor;
	char outFilePath[SIZE];
	char inFilePath[SIZE];



	char* inputDirectoryPath = argv[1];
	char* keyFilePath = argv[2];
	char* outputDirectoryPath = argv[3];

	//printf("inputDirectoryPath = %s\nkeyFilePath = %s\noutputDirectoryPath = %s\n",inputDirectoryPath,keyFilePath,outputDirectoryPath);

	/*try to open the input directory stream*/
	if ((inDirectory = opendir(inputDirectoryPath)) == NULL) {
		printf("Error opening directory: %s. %s\n", inputDirectoryPath,strerror(errno) );
		return errno; //TODO add appropriate error msg
	}

	/*try to open the output directory stream*/
	if ((outDirectory = opendir(outputDirectoryPath)) == NULL) {

		/*create a directory with the requested name*/
		int makedirRes = mkdir(outputDirectoryPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		if (makedirRes < 0) {
			printf("Error creating new directory directory: %s. %s\n", outputDirectoryPath,strerror(errno));
			closedir(inDirectory);
			return errno;
		}

		/*try to open the newly created directory*/
		if ((outDirectory = opendir(outputDirectoryPath)) == NULL) {
			printf("Error opening directory: %s. %s\n", outputDirectoryPath,strerror(errno));
			closedir(inDirectory);
			return errno;
		}

	}


	/****************************************************************
	 * inDirectory loop - go through all the files of inDirectory
	 * **************************************************************/
	while ( (directoryEntry = readdir(inDirectory)) != NULL) {

		/*prepare output and input file path*/
		sprintf(outFilePath,"%s/%s" ,outputDirectoryPath,directoryEntry->d_name );
		sprintf(inFilePath,"%s/%s" ,inputDirectoryPath,directoryEntry->d_name );

		/*make sure we are not reading a directory*/
		if ((strcmp(directoryEntry->d_name, DIRECTORY) == 0) || (strcmp(directoryEntry->d_name, PARENTDIR) == 0) || (strcmp(directoryEntry->d_name, ".DS_Store") == 0) ) {
			continue;
		}

		//printf("currently working on the file %s\n",directoryEntry->d_name);

		/* open input file*/
		inFileDescriptor = open(inFilePath, O_RDONLY);
		if (inFileDescriptor < 0) {
			printf("Error opening file: %s. %s\n",inFilePath, strerror(errno));
			closedir(inDirectory);
			closedir(outDirectory);
			return errno;
		}

		/* open output file*/
		outFileDescriptor = open(outFilePath, O_RDWR | O_CREAT| O_TRUNC, 0777);
		if (outFileDescriptor < 0) {
			printf("Error opening file: %s. %s\n",outFilePath, strerror(errno)) ;
			close(inFileDescriptor);
			closedir(inDirectory);
			closedir(outDirectory);
			return errno;
		}

		/*try to open the key file*/
		keyFileDescriptor = open(keyFilePath, O_RDONLY);
		if( keyFileDescriptor < 0 ) {
			printf( "Error opening file: %s. %s\n", keyFilePath, strerror(errno));
			close(inFileDescriptor);
			close(outFileDescriptor);
			closedir(inDirectory);
			closedir(outDirectory);
			return errno;
		}

		/**********************
		 * read file byte-byte
		 * ********************/
		while((numOfFileBytesRead = read(inFileDescriptor, &inBuffer, 1)) > 0) {
			/*get key's next byte*/
			numOfKeyBytesRead = read(keyFileDescriptor, &keyBuffer, 1);

			/*if we reached the end of key file before we finished input file, re-open key file*/
			if (numOfKeyBytesRead == 0) {
				numOfTimesKeyFileWasOpen ++;
				close(keyFileDescriptor);
				keyFileDescriptor = open(keyFilePath, O_RDONLY);
				if( keyFileDescriptor < 0 ) {
					printf( "Error opening file: %s. %s\n", keyFilePath, strerror(errno));
					close(outFileDescriptor);
					close(inFileDescriptor);
					closedir(inDirectory);
					closedir(outDirectory);
					return errno;

				}
				numOfKeyBytesRead = read (keyFileDescriptor, &keyBuffer, 1);
			}


			/*make XOR calc between the 2 bytes and write it to output file*/
			char xor[BYTE];
			xor[0] = inBuffer[0] ^ keyBuffer[0];
			numOfBytesWrite = write (outFileDescriptor, xor, (ssize_t) numOfFileBytesRead);

			if (numOfBytesWrite != numOfFileBytesRead){
				printf( "Error writing to file: %s. %s\n", outFilePath, strerror(errno));
				close(outFileDescriptor);
				close(inFileDescriptor);
				close(keyFileDescriptor);
				closedir(inDirectory);
				closedir(outDirectory);
				return errno;
			}
		}

		//printf("key file was opened %d times\n", numOfTimesKeyFileWasOpen);
		close(outFileDescriptor);
		close(inFileDescriptor);

	}

	close(outFileDescriptor);
	close(inFileDescriptor);
	close(keyFileDescriptor);
	closedir(inDirectory);
	closedir(outDirectory);
	printf("Done!\n");
	return 0;
}
