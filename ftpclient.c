/**************************************************************
 * *  Filename: otp_enc.c
 * *  Coded by: Kevin To
 * *  Purpose - Acts as the client to send work to the encyption
 * *            daemon.
 * *            Sample command:
 * *              otp_enc plaintext key port
 * *
 * *              plaintext = file holding the plain text to be
 * *                          converted.
 * *              key = file holding they key that will be used in
 * *                    the encyption.
 * *              port = the port to connect to the daemon on.
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

#define LENGTH 512

void error(const char *msg);
void ConnectToServer(char *portString, char *plainTextFileName, char *keyFileName);
void RemoveNewLineAndAddNullTerm(char *fileName);
void CheckKeyLength(long keySize, long plainTextSize, char *keyName);
void ScanInvalidCharacters(char *stringToCheck, int stringLength);
void SendFileToServer(int sockfd, int tempFileDesc);
void ReceiveFileFromServer(int sockfd, char *fileName);
int CombineTwoFiles(char *fileOneName, char *fileTwoName);
int GetTempFD();
void AddNewLineToEndOfFile(FILE *filePointer);
void OutputTempFile(int tempFileDesc);
void BufRemoveNewLineAndAddNullTerm(char *buffer, int bufferSize);

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
int main(int argc, char *argv[])
{
	// If the user did not enter the correct number of parameters,
	//  display the correct message.
	if (argc != 4)
	{
		printf("usage: otp_enc plaintext key port\n");
		exit(1);
	}

	// ------------------------------ Get the plain text ----------------------
	FILE *filePointer = fopen(argv[1], "rb");
	if (filePointer == 0)
	{
		printf("Plaintext file does not exist\n");
		exit(1);
	}

	// Find the size of the file.
	fseek(filePointer, 0, SEEK_END); // Sets the position indicator to the end of the file
	long plainTextSize = ftell(filePointer); // Gets the file size
	fseek(filePointer, 0, SEEK_SET); // Sets the position indicator to the start of the file

	// Get the string from the file. Remember plainTextSize includes the newline
	//  character at the end of the file.
	char *plainTextString = malloc(plainTextSize + 1); // Allocates memory for the string taken from the file
	fread(plainTextString, plainTextSize, 1, filePointer); // Get the string from the file
	fclose(filePointer);

	plainTextString[plainTextSize] = 0; // Null terminate the string
	RemoveNewLineAndAddNullTerm(plainTextString);

	// ------------------------------ Get the key ----------------------
	filePointer = fopen(argv[2], "rb");
	if (filePointer == 0)
	{
		printf("key file does not exist\n");
		exit(1);
	}

	// Find the size of the file.
	fseek(filePointer, 0, SEEK_END); // Sets the position indicator to the end of the file
	long keySize = ftell(filePointer); // Gets the file size
	fseek(filePointer, 0, SEEK_SET); // Sets the position indicator to the start of the file

	// Get the string from the file. Remember keySize includes the newline
	//  character at the end of the file.
	char *keyString = malloc(keySize + 1); // Allocates memory for the string taken from the file
	fread(keyString, keySize, 1, filePointer); // Get the string from the file
	fclose(filePointer);

	keyString[keySize] = 0; // Null terminate the string
	RemoveNewLineAndAddNullTerm(keyString);

	// Check if the key is shorter than the plaintext
	CheckKeyLength(keySize, plainTextSize, argv[2]);

	// Check if key or plain text have any invalid characters
	ScanInvalidCharacters(plainTextString, plainTextSize);
	ScanInvalidCharacters(keyString, keySize);

	// Connect to server
	ConnectToServer(argv[3], argv[1], argv[2]);

	// Free the strings
	free(plainTextString);
	free(keyString);

	return 0;
}

/**************************************************************
 * * Entry:
 * *  N/a
 * *
 * * Exit:
 * *  N/a
 * *
 * * Purpose:
 * *  This method does all the socket communication
 * *
 * ***************************************************************/
void ConnectToServer(char *portString, char *plainTextFileName, char *keyFileName)
{
	int sockfd;
	struct sockaddr_in remote_addr;
	int portNumber = atoi(portString);

	/* Get the Socket file descriptor */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Error: Failed to obtain socket descriptor.\n");
		exit(2);
	}

	/* Fill the socket address struct */
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(portNumber);
	inet_pton(AF_INET, "127.0.0.1", &remote_addr.sin_addr);
	bzero(&(remote_addr.sin_zero), 8);

	/* Try to connect the remote */
	if (connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
	{
		printf("Error: could not contact otp_enc_d on port %s\n", portString);
		exit(2);
	}
	else
	{
		printf("[otp_enc] Connected to server at port %d...ok!\n", portNumber);
	}

	int resultTempFd = CombineTwoFiles(plainTextFileName, keyFileName);
	// FILE *combinedFilePointer = fdopen(resultTempFd, "rb");
	// if (combinedFilePointer == 0)
	// {
	// 	printf("[otp_enc] Could not open combined temp file.\n");
	// }
	// OutputTempFile(resultTempFd);
	// Send file to server -- TODO: convert the second parameter to a file pointer to the file
	SendFileToServer(sockfd, resultTempFd); // Send the combined file


	// Receive file from server
	ReceiveFileFromServer(sockfd, "final");

	close (sockfd);
	printf("[otp_enc] Connection lost.\n");
}

void ReceiveFileFromServer(int sockfd, char *fileName)
{
	printf("[otp_enc] Receiveing file from Server and saving it as final.txt...");

	FILE *filePointer = fopen(fileName, "w");
	if (filePointer == 0)
		printf("[otp_enc] File %s Cannot be opened.\n", fileName);
	else
	{
		char recvBuffer[LENGTH];
		bzero(recvBuffer, LENGTH);

		// Wait for info that is sent from server
		int receiveSize = 0;
		while ((receiveSize = recv(sockfd, recvBuffer, LENGTH, 0)) > 0)
		{
			// Write info sent from server into file
			int writeSize = fwrite(recvBuffer, sizeof(char), receiveSize, filePointer);
			if (writeSize < receiveSize)
			{
				error("[otp_enc] File write failed.\n");
			}
			bzero(recvBuffer, LENGTH);

			// Exit out of receive loop if data chunk size is invalid
			if (receiveSize == 0 || receiveSize != 512)
			{
				break;
			}
		}
		if (receiveSize < 0)
		{
			if (errno == EAGAIN)
			{
				printf("[otp_enc] recv() timed out.\n");
			}
			else
			{
				printf("[otp_enc] recv() failed \n");
			}
		}
		printf("Ok received from server!\n");
		fclose(filePointer);
	}
}

// void SendFileToServer(int sockfd, char *fileName)
void SendFileToServer(int sockfd, int tempFileDesc)
{
	char sendBuffer[LENGTH];
	bzero(sendBuffer, LENGTH);

	// OutputTempFile(tempFileDesc);
	// FILE *filePointer = fopen(fileName, "rb");
	// FILE *filePointer = fdopen(tempFileDesc, "rb");
	// if (filePointer == NULL)
	// {
	// 	printf("[otp_enc] ERROR: File not found to be sent.\n");
	// 	exit(1);
	// }

	printf("[otp_enc] Sending file to the Server... ");
	int sendSize;
	// while ((sendSize = fread(sendBuffer, sizeof(char), LENGTH, filePointer)) > 0)
	while ((sendSize = read(tempFileDesc, sendBuffer, sizeof(sendBuffer))) > 0)
	{
		if (send(sockfd, sendBuffer, sendSize, 0) < 0)
		{
			printf("[otp_enc] Error: Failed to send file.\n");
			break;
		}
		bzero(sendBuffer, LENGTH);
	}
	printf("Ok File from Client was Sent!\n");

	// fclose(filePointer);
}

int CombineTwoFiles(char *fileOneName, char *fileTwoName)
{
	char readBuffer[LENGTH];
	int sizeRead = 0;
	FILE *fileOnePointer = fopen(fileOneName, "rb");
	FILE *fileTwoPointer = fopen(fileTwoName, "rb");
	int tempFD = GetTempFD();

	// Add the contents of the first file to the temp file.
	// Also added a semi-colon delimiter at the end.
	bzero(readBuffer, LENGTH);
	while ((sizeRead = fread(readBuffer, sizeof(char), LENGTH, fileOnePointer)) > 0)
	{
		BufRemoveNewLineAndAddNullTerm(readBuffer, LENGTH); // Removes new line and add null term if needed
		if (write(tempFD, readBuffer, sizeRead - 1) == -1) // Write to the temp file
		{
			printf("[otp_enc] Error in combining plaintext and key\n");
		}
		bzero(readBuffer, LENGTH);
	}

	// Add a semicolon at the end of the first file
	bzero(readBuffer, LENGTH);
	strncpy(readBuffer, ";", 1);
	if (write(tempFD, readBuffer, 1) == -1)
	{
		printf("[otp_enc] Error in combining plaintext and key\n");
	}

	// Add the contents of the second file to the temp file.
	// Also added a semi-colon delimiter at the end.
	bzero(readBuffer, LENGTH);
	while ((sizeRead = fread(readBuffer, sizeof(char), LENGTH, fileTwoPointer)) > 0)
	{
		BufRemoveNewLineAndAddNullTerm(readBuffer, LENGTH); // Removes new line and add null term if needed
		if (write(tempFD, readBuffer, sizeRead - 1) == -1) // Write to the temp file
		{
			printf("[otp_enc] Error in combining plaintext and key\n");
		}
		bzero(readBuffer, LENGTH);
	}

	// Add a semicolon at the end of the second file
	bzero(readBuffer, LENGTH);
	strncpy(readBuffer, ";", 1);
	if (write(tempFD, readBuffer, 1) == -1)
	{
		printf("[otp_enc] Error in combining plaintext and key\n");
	}

	// Reset the file pointer for the temp file	
	if (-1 == lseek(tempFD, 0, SEEK_SET))
	{
		printf("File pointer reset for combined file failed\n");
	}

	// Used for debugging only
	// OutputTempFile(tempFD);
	fclose(fileOnePointer);
	fclose(fileTwoPointer);

	return tempFD;
}

/**************************************************************
 * * Entry:
 * *  msg - the message to print along with the error
 * *
 * * Exit:
 * *  N/a
 * *
 * * Purpose:
 * *  This is the wrapper for the error handler.
 * *
 * ***************************************************************/
void error(const char *msg)
{
	perror(msg);
	exit(1);
}

/**************************************************************
 * * Entry:
 * *  stringValue - the string you want to transform
 * *
 * * Exit:
 * *  n/a
 * *
 * * Purpose:
 * *  Removes the new line character from the string and adds a null
 * *  terminator in its place.
 * *
 * ***************************************************************/
void RemoveNewLineAndAddNullTerm(char *stringValue)
{
	size_t ln = strlen(stringValue) - 1;
	if (stringValue[ln] == '\n')
	{
		stringValue[ln] = '\0';
	}
}

/**************************************************************
 * * Entry:
 * *  keySize - the size of the key
 * *  plainTextSize - the size of the plain text
 * *  keyName - the key file name
 * *
 * * Exit:
 * *  n/a
 * *
 * * Purpose:
 * *  Checks if key is shorter than the plaintext. If it is, then
 * *  display the error message and exit.
 * *
 * ***************************************************************/
void CheckKeyLength(long keySize, long plainTextSize, char *keyName)
{
	if (keySize < plainTextSize)
	{
		printf("Error: key %s is too short\n", keyName);
		exit(1);
	}
}

/**************************************************************
 * * Entry:
 * *  stringValue - the string you want to validate
 * *  stringLength - the length of the string you want to validate
 * *
 * * Exit:
 * *  n/a
 * *
 * * Purpose:
 * *  Checks if the string does not contain uppercase letters or
 * *  space. If it does not, then the program exits with an error.
 * *
 * ***************************************************************/
void ScanInvalidCharacters(char *stringValue, int stringLength)
{
	static const char possibleChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	int i;
	for (i = 0; i < stringLength; i++)
	{
		// If there is an invalid character then exit the program
		if (strchr(possibleChars, stringValue[i]) == 0)
		{
			printf("otp_enc error: input contains bad characters\n");
			exit(1);
		}
	}
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

	return filedes;
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

void OutputTempFile(int tempFileDesc)
{
	// Do not need to close open file because the temp
	// file will auto delete at program exit.
	FILE *filePointer = fdopen(tempFileDesc, "rb");

	// Set file pointer to beginning of the file
	if (fseek(filePointer, 0, SEEK_SET) == -1)
	{
		printf("Received file pointer reset failed\n");
	}

	printf("test write:....\n");
	if (filePointer) {
		int c;
		while ((c = fgetc(filePointer)) != EOF)
		{
			putchar(c);
		}
	}

	// Set file pointer to beginning of the file
	if (fseek(filePointer, 0, SEEK_SET) == -1)
	{
		printf("Received file pointer reset failed\n");
	}
}

// Removes new lines for buffers
void BufRemoveNewLineAndAddNullTerm(char *buffer, int bufferSize)
{
	int i;
	for (i = 0; i < bufferSize; i++)
	{
		// Exit if we reached a null term
		if (buffer[i] == '\0')
		{
			return;
		}

		// Replace new line with null term
		if (buffer[i] == '\n')
		{
			buffer[i] = '\0';
		}
	}
}