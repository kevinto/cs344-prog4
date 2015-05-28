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

void ScanFileForInvalidCharacters(char *stringToCheck);
void RemoveNewLineAndAddNullTerm(char *fileName);

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
		printf("otp_enc plaintext key port\n");
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

	// For debugging -----------------------------------
	// printf("plainTextSize: %ld\n", plainTextSize);
	// printf("keySize: %ld\n", keySize);
	// printf("string contents: %s\n", plainTextString);
	// printf("string contents: %s\n", keyString);


	// TODO: do the character validation. change the function name
	// Check if key or plain text have any invalid characters
	// ScanFileForInvalidCharacters(argv[1]);
	// ScanFileForInvalidCharacters(argv[2]);

	// Free the strings
	free(plainTextString);
	free(keyString);
	return 0;
}

void ScanFileForInvalidCharacters(char *fileName)
{
	// static const char possibleChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	// FILE *filePointer;

	// filePointer = fopen(fileName, "r");
 //  while(fgets(readString, 199, filePointer))
 //  {
 //     // Load the connections
 //     if (strstr(readString, "CONNECTION") != NULL) {
 //        strncpy(saveString, readString + 14, 79);
 //        RemoveNewLineAndAddNullTerm(saveString);
 //        AddConnectionToRoom(rooms, i, saveString);
 //     }

 //     // Load the room type
 //     if (strstr(readString, "ROOM TYPE:") != NULL) {
 //        strncpy(saveString, readString + 11, 79);
 //        RemoveNewLineAndAddNullTerm(saveString);
 //        strncpy(rooms[i].roomType, saveString, 79);
 //     }
 //  }

 //  fclose(filePointer);

	FILE *filePointer = fopen(fileName, "rb");

	// Find the size of the file
	fseek(filePointer, 0, SEEK_END); // Sets the position indicator to the end of the file
	long fsize = ftell(filePointer); // Gets the file size
	fseek(filePointer, 0, SEEK_SET); // Sets the position indicator to the start of the file

	// Get the string from the file
	char *stringFromFile = malloc(fsize + 1); // Allocates memory for the string taken from the file
	fread(stringFromFile, fsize, 1, filePointer); // Get the string from the file
	fclose(filePointer);

	stringFromFile[fsize] = 0; // Null terminate the string
	RemoveNewLineAndAddNullTerm(stringFromFile);

	printf("string contents: %s\n", stringFromFile);
	free(stringFromFile);
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