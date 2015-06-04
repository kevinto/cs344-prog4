/**************************************************************
 * *  Filename: otp_enc_d.c
 * *  Coded by: Kevin To
 * *  Purpose -
 * *
 * ***************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>

#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LISTEN_QUEUE 5
#define LENGTH 512

void error(const char *msg);
void ProcessConnection(int socket);
int GetTempFD();
void ReceiveClientFile(int socket, FILE *tempFilePointer);
void SendFileToClient(int socket, int tempFilePointer);
void AddNewLineToEndOfFile(FILE *filePointer);
int GetSizeOfPlaintext(FILE *filePointer);
int GetSizeOfKeyText(FILE *filePointer);
void SavePlainTextToString(char *plainTextString, int plainTextSize, FILE *filePointer);
void SaveKeyTextToString(char *keyTextString, int keyTextSize, FILE *filePointer);
void EncyptText(char *plainTextString, int plainTextSize, char *keyTextString, int keyTextSize, char *cipherText);

// Signal handler to clean up zombie processes
static void wait_for_child(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

/**************************************************************
 * * Entry:
 * *  argc - the number of arguments passed into this program
 * *  argv - a pointer to the char array of all the arguments
 * *         passed into this program
 * *
 * * Exit:
 * *  N/a
 * *
 * * Purpose:
 * *  This is the entry point into the program.
 * *
 * ***************************************************************/
int main (int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("usage: otp_enc_d port\n");
		exit(1);
	}

	int sockfd, newsockfd, sin_size, pid;
	struct sockaddr_in addr_local; // client addr
	struct sockaddr_in addr_remote; // server addr
	int portNumber = atoi(argv[1]);
	struct sigaction sa;

	// Get the socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor. (errno = %d)\n", errno);
		exit(1);
	}
	else
	{
		printf("[Server] Obtaining socket descriptor successfully.\n");
	}

	// Fill the client socket address struct
	addr_local.sin_family = AF_INET; // Protocol Family
	addr_local.sin_port = htons(portNumber); // Port number
	addr_local.sin_addr.s_addr = INADDR_ANY; // AutoFill local address
	bzero(&(addr_local.sin_zero), 8); // Flush the rest of struct

	// Bind a port
	if ( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to bind Port. (errno = %d)\n", errno);
		exit(1);
	}
	else
	{
		printf("[Server] Binded tcp port %d in addr 127.0.0.1 sucessfully.\n", portNumber);
	}

	// Listen to port
	if (listen(sockfd, LISTEN_QUEUE) == -1)
	{
		fprintf(stderr, "ERROR: Failed to listen Port. (errno = %d)\n", errno);
		exit(1);
	}
	else
	{
		printf ("[Server] Listening the port %d successfully.\n", portNumber);
	}

	// Set up the signal handler to clean up zombie children
	sa.sa_handler = wait_for_child;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}

	// Main loop that accepts and processes client connections
	while (1)
	{
		sin_size = sizeof(struct sockaddr_in);

		// Wait for any connections from the client
		if ((newsockfd = accept(sockfd, (struct sockaddr *)&addr_remote, &sin_size)) == -1)
		{
			fprintf(stderr, "ERROR: Obtaining new socket descriptor. (errno = %d)\n", errno);
			exit(1);
		}
		else
		{
			printf("[Server] Server has got connected from %s.\n", inet_ntoa(addr_remote.sin_addr));
		}

		// Create child process to handle processing
		// TODO: need to limit to only 5 forks
		pid = fork();
		if (pid < 0)
		{
			perror("ERROR on fork");
			exit(1);
		}

		if (pid == 0)
		{
			/* This is the client process */
			close(sockfd);
			ProcessConnection(newsockfd);
			exit(0);
		}
		else
		{
			close(newsockfd);
		}
	}
}

/**************************************************************
 * * Entry:
 * *  socket - the socket file descriptor
 * *
 * * Exit:
 * *  N/a
 * *
 * * Purpose:
 * *  This code gets run in the forked process to handle the actual
 * *  server processing of the client request.
 * *
 * ***************************************************************/
void ProcessConnection(int socket)
{
	// char receiveBuffer[LENGTH]; // Receiver buffer

	// Receive the plaintext and key file from the client
	// Both plaintext and key are in one file
	int receiveTempFilePointer = GetTempFD();
	FILE *filePointer = fdopen(receiveTempFilePointer, "w+");
	if (filePointer == 0)
	{
		printf("File temp receive cannot be opened file on server.\n");
	}
	else
	{
		ReceiveClientFile(socket, filePointer);
	}
	AddNewLineToEndOfFile(filePointer);

	// TODO - KEEP track of threads

	// Get the plain text from the file and save to a string
	int plainTextSize = GetSizeOfPlaintext(filePointer);
	char *plainTextString = malloc(plainTextSize + 1); // Allocates memory for the string taken from the file
	bzero(plainTextString, plainTextSize + 1);
	SavePlainTextToString(plainTextString, plainTextSize, filePointer);
	// printf("plainTextString: %s\n", plainTextString); // For debug only

	// Get the plain text from the file and save to a string
	int keyTextSize = GetSizeOfKeyText(filePointer);
	char *keyTextString = malloc(keyTextSize + 1); // Allocates memory for the string taken from the file
	bzero(keyTextString, keyTextSize + 1);
	SaveKeyTextToString(keyTextString, keyTextSize, filePointer);
	// printf("keyTextString: %s\n", keyTextString);

	// TODO - do processing on the recieved file
	// calculate size of the encypted text
	char *cipherText = malloc(plainTextSize + 1); // Allocates memory for the cipherText
	bzero(cipherText, plainTextSize + 1);
	EncyptText(plainTextString, plainTextSize, keyTextString, keyTextSize, cipherText);

	int resultTempFD = 	GetTempFD();
	FILE *resultFilePointer = fdopen(resultTempFD, "w+");
	if (resultFilePointer != 0)
	{
		printf("putting to file: %s\n", cipherText);
		fputs(cipherText, resultFilePointer);
		AddNewLineToEndOfFile(resultFilePointer);
	}

	// Send File to Client 
	// // Set file pointer to the start of the file
	// SendFileToClient(socket, receiveTempFilePointer);
	SendFileToClient(socket, resultTempFD);

	free(plainTextString);
	free(keyTextString);
	free(cipherText);
	fclose(filePointer);
	close(receiveTempFilePointer);
	close(socket);

	printf("[Server] Connection with Client closed. Server will wait now...\n");
}

void EncyptText(char *plainTextString, int plainTextSize, char *keyTextString, int keyTextSize, char *cipherText)
{
	strncpy(cipherText, "blah", plainTextSize);
}

void SaveKeyTextToString(char *keyTextString, int keyTextSize, FILE *filePointer)
{
	char readBuffer[LENGTH];
	int i;
	int fileTracker = 0;
	int stringTracker = 0;
	int foundFirstSemiColon = 0;

	// Set file pointer to the start of the file
	if (fseek(filePointer, 0, SEEK_SET) == -1)
	{
		printf("Received file pointer reset failed\n");
	}

	// Count the number of characters
	bzero(readBuffer, LENGTH);
	while (fread(readBuffer, sizeof(char), LENGTH, filePointer) > 0)
	{
		// Loop through the buffer to count characters
		for (i = 0; i < LENGTH; i++)
		{
			// Check if we found the first semicolon
			if (!foundFirstSemiColon && readBuffer[i] == ';')
			{
				foundFirstSemiColon = 1;
				continue;
			}

			if (foundFirstSemiColon)
			{
				keyTextString[stringTracker] = readBuffer[i]; // Copy the file contents to the string
				stringTracker++;
				fileTracker++;
			}

			// Exit loop if we reached the end of the key length
			if (fileTracker == (keyTextSize))
			{
				break;
			}

		}
		bzero(readBuffer, LENGTH);
	}
}

void SavePlainTextToString(char *plainTextString, int plainTextSize, FILE *filePointer)
{
	char readBuffer[LENGTH];
	int i;
	int fileTracker = 0;
	int stringTracker = 0;

	// Set file pointer to the start of the file
	if (fseek(filePointer, 0, SEEK_SET) == -1)
	{
		printf("Received file pointer reset failed\n");
	}

	// Count the number of characters
	bzero(readBuffer, LENGTH);
	while (fread(readBuffer, sizeof(char), LENGTH, filePointer) > 0)
	{
		// Loop through the buffer to count characters
		for (i = 0; i < LENGTH; i++)
		{
			// Exit loop if we reached the end of the plain text portion of the file
			if (fileTracker == (plainTextSize))
			{
				break;
			}

			plainTextString[stringTracker] = readBuffer[i]; // Copy the file contents to the string
			stringTracker++;
			fileTracker++;
		}
		bzero(readBuffer, LENGTH);
	}
}

int GetSizeOfKeyText(FILE *filePointer)
{
	char readBuffer[LENGTH];
	int i;
	int keyTextSize = 0;
	int foundFirstSemiColon = 0;
	int foundLastSemiColon = 0;

	// Set file pointer to the start of the file
	if (fseek(filePointer, 0, SEEK_SET) == -1)
	{
		printf("Received file pointer reset failed\n");
	}

	// Count the number of characters
	bzero(readBuffer, LENGTH);
	while (fread(readBuffer, sizeof(char), LENGTH, filePointer) > 0)
	{
		// Loop through the buffer to count characters
		for (i = 0; i < LENGTH; i++)
		{
			// Found the first semi-colon delimiter.
			if ((foundFirstSemiColon == 0) && readBuffer[i] == ';')
			{
				foundFirstSemiColon = 1;
				continue;
			}

			// Count the characters after the first semi-colon
			if (foundFirstSemiColon)
			{
				// Found the file end semicolon. Exit.
				if (readBuffer[i] == ';')
				{
					foundLastSemiColon = 1;
					break;
				}

				keyTextSize++;
			}
		}
		bzero(readBuffer, LENGTH);

		if (foundLastSemiColon)
		{
			// Found the last semi-colon, exit out of the loop
			break;
		}
	}

	return keyTextSize;
}

int GetSizeOfPlaintext(FILE *filePointer)
{
	char readBuffer[LENGTH];
	int i;
	int fileSize = 0;
	int foundFirstSemiColon = 0;

	// Set file pointer to the start of the file
	if (fseek(filePointer, 0, SEEK_SET) == -1)
	{
		printf("Received file pointer reset failed\n");
	}

	// Count the number of characters
	bzero(readBuffer, LENGTH);
	while (fread(readBuffer, sizeof(char), LENGTH, filePointer) > 0)
	{
		// Loop through the buffer to count characters
		for (i = 0; i < LENGTH; i++)
		{
			// Found the semi-colon delimiter. Break from loop
			if (readBuffer[i] == ';')
			{
				foundFirstSemiColon = 1;
				break;
			}

			fileSize++; // Keep track of the file size
		}
		bzero(readBuffer, LENGTH);

		if (foundFirstSemiColon)
		{
			// Found the delimiter. Break out of the loop
			break;
		}
	}

	return fileSize;
}

void SendFileToClient(int socket, int tempFilePointer)
{
	// char* fs_name = "receive";
	char sdbuf[LENGTH]; // Send buffer
	// printf("[Server] Sending %s to the Client...", fs_name);
	printf("[Server] Sending received file to the Client...");
	// FILE *fs = fopen(fs_name, "r");
	// if (fs == NULL)
	if (tempFilePointer == 0)
	{
		// fprintf(stderr, "ERROR: File %s not found on server. (errno = %d)\n", fs_name, errno);
		fprintf(stderr, "ERROR: File temp received not found on server.");
		exit(1);
	}

	bzero(sdbuf, LENGTH);
	int fs_block_sz;
	while ((fs_block_sz = read(tempFilePointer, sdbuf, LENGTH)) > 0)
	{
		if (send(socket, sdbuf, fs_block_sz, 0) < 0)
		{
			// fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
			fprintf(stderr, "ERROR: Failed to send file temp received.");
			exit(1);
		}
		bzero(sdbuf, LENGTH);
	}
	printf("Ok sent to client!\n");
	// fclose(fr);
}

void ReceiveClientFile(int socket, FILE *tempFilePointer)
{
	char receiveBuffer[LENGTH]; // Receiver buffer
	bzero(receiveBuffer, LENGTH); // Clear out the buffer

	// Loop the receiver until all file data is received
	int bytesReceived = 0;
	while ((bytesReceived = recv(socket, receiveBuffer, LENGTH, 0)) > 0)
	{
		int bytesWrittenToFile = fwrite(receiveBuffer, sizeof(char), bytesReceived, tempFilePointer);
		if (bytesWrittenToFile < bytesReceived)
		{
			error("File write failed on server.\n");
		}
		bzero(receiveBuffer, LENGTH);
		if (bytesReceived == 0 || bytesReceived != 512)
		{
			break;
		}
	}
	if (bytesReceived < 0)
	{
		if (errno == EAGAIN)
		{
			printf("recv() timed out.\n");
		}
		else
		{
			fprintf(stderr, "recv() failed due to errno = %d\n", errno);
			exit(1);
		}
	}
	printf("Ok received from client!\n");
}

// found here: http://www.thegeekstuff.com/2012/06/c-temporary-files/
// how to use the temp file with fwrite - http://www.it.uc3m.es/abel/as/PRJ/M5/inclass_en.html
int GetTempFD()
{
	char tempFileNameBuffer[32];
	char buffer[24];
	int filedes;

	// Zero out the buffers
	bzero(tempFileNameBuffer, sizeof(tempFileNameBuffer));
	bzero(buffer, sizeof(buffer));

	// Set up temp template
	strncpy(tempFileNameBuffer, "/tmp/myTmpFile-XXXXXX", 21);
	// strncpy(buffer, "Hello World", 11); // Need for test only

	errno = 0;
	// Create the temporary file, this function will replace the 'X's
	filedes = mkstemp(tempFileNameBuffer);

	// Call unlink so that whenever the file is closed or the program exits
	// the temporary file is deleted
	unlink(tempFileNameBuffer);

	if (filedes < 1)
	{
		printf("\n Creation of temp file failed with error [%s]\n", strerror(errno));
		return 1;
	}

	// // Write some data to the temporary file
	// if (-1 == write(filedes, buffer, sizeof(buffer)))
	// {
	//   printf("\n write failed with error [%s]\n", strerror(errno));
	//   return 1;
	// }

	// int count;
	// if ( (count = read(filedes, buffer, 11)) < 11 )
	// {
	//   printf("\n read failed with error [%s]\n", strerror(errno));
	//   return 1;
	// }

	// Show whatever is read
	// printf("\n Data read back from temporary file is [%s]\n", buffer);
	return filedes;

	// errno = 0;
	// // rewind the stream pointer to the start of temporary file
	// if (-1 == lseek(filedes, 0, SEEK_SET)) // Need this function for later
	// {
	//   printf("\n lseek failed with error [%s]\n", strerror(errno));
	//   return 1;
	// }
}

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

void AddNewLineToEndOfFile(FILE *filePointer)
{
	char newlineBuffer[1] = "\n";

	// Set the file pointer to the end of the file
	if (fseek(filePointer, 0, SEEK_END) == -1)
	{
		printf("Received file pointer reset failed\n");
	}

	// Write the newline char to the end of the file
	fwrite(newlineBuffer, sizeof(char), 1, filePointer);

	// Set file pointer to the start of the temp file
	if (fseek(filePointer, 0, SEEK_SET) == -1)
	{
		printf("Received file pointer reset failed\n");
	}
}