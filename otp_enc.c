/**************************************************************
 * *  Filename: otp_enc.c
 * *  Coded by: Kevin To
 * *  Purpose - Acts as the client to send work to the encyption 
 * *						daemon. 
 * *						Sample command:
 * *							otp_enc plaintext key port
 * *
 * *							plaintext = file holding the plain text to be 
 * *													converted.
 * *							key = file holding they key that will be used in 
 * *										the encyption.
 * *							port = the port to connect to the daemon on.
 * *
 * ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/types.h>

void ScanInvalidCharacters(char *stringToCheck, int stringLength);
void RemoveNewLineAndAddNullTerm(char *fileName);
void CheckKeyLength(long keySize, long plainTextSize, char *keyName);
void error(const char *msg);
void ConnectToServer(char *portString, char *plainTextString, int plainTextSize, char *keyString, int keySize);

/**************************************************************
 * * Entry:
 * *  N/a
 * *
 * * Exit:
 * *  N/a
 * *
 * * Purpose:
 * *	This is the entry point into the program.
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
	ConnectToServer(argv[3], plainTextString, plainTextSize, keyString, keySize);

	// Free the strings
	free(plainTextString);
	free(keyString);
	return 0;
}

void ConnectToServer(char *portString, char *plainTextString, int plainTextSize, char *keyString, int keySize)
{
	int sockfd, portNumber, n;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  char buffer[256];
  portNumber = atoi(portString);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
      error("ERROR opening socket");
  server = gethostbyname("localhost");
  if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
  }
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
       (char *)&serv_addr.sin_addr.s_addr,
       server->h_length);
  serv_addr.sin_port = htons(portNumber);
  if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
  {
  	printf("Error: could not contact otp_enc_d on port %d\n", portNumber);
  	exit(2);
  }
  // printf("Please enter the message: ");
  // bzero(buffer,256);
  // fgets(buffer,255,stdin);
  // n = write(sockfd,buffer,strlen(buffer));
  // if (n < 0) 
  //      error("ERROR writing to socket");
  // bzero(buffer,256);
  // n = read(sockfd,buffer,255);
  // if (n < 0) 
  //      error("ERROR reading from socket");
  // printf("%s\n",buffer);
  close(sockfd);
}

void error(const char *msg)
{
    perror(msg);
    exit(0);
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
 * *	display the error message and exit.
 * *
 * ***************************************************************/
void CheckKeyLength(long keySize, long plainTextSize, char *keyName)
{
	if (keySize < plainTextSize)
	{
		printf("Error: key ‘%s’ is too short\n", keyName);
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
 * *	space. If it does not, then the program exits with an error.
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
