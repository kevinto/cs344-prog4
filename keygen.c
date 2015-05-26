/**************************************************************
 * *  Filename: keygen.c
 * *  Coded by: Kevin To
 * *  Purpose - To generate a random string of specified length
 * *
 * ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

void GetRandomizedString(char *stringHolder, int stringLength);

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
	// Seed the random number generator
	srandom(time(NULL));

	int randomStringLength;
	char *randomizedString;

	// If there are more than 2 parameters, then get the randomized string.
	if (argc >= 2)
	{
		randomStringLength = atoi(argv[1]);

		randomizedString = (char*)malloc(sizeof(char)*(randomStringLength+1));
		GetRandomizedString(randomizedString, randomStringLength);
	}

	// If there are exactly 2 parameters, then output to stdout
	if (argc == 2)
	{
		printf("%s\n\n", randomizedString);
	}

	free(randomizedString);

	return 0;
}

/**************************************************************
 * * Entry:
 * *  stringHolder - pointer to the string to hold the result
 * *  stringLength - the length of the string holder
 * *
 * * Exit:
 * *  N/a
 * *
 * * Purpose:
 * *	Gets a randomized string of a specified length. Randomized
 * *	string will contain upper case letters and a space character.
 * *
 * ***************************************************************/
void GetRandomizedString(char *stringHolder, int stringLength)
{
	static const char possibleChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

	int i;
	for (i = 0; i < stringLength; i++) {
		// For each element in the string array put a random character
		// from the possible characters into the string holder
	    stringHolder[i] = possibleChars[rand() % (sizeof(possibleChars) - 1)];
	}

	// Make the last element null terminated. 
	// There is an extra element here because we allocated an additional 
	// element for the null terminator.
	stringHolder[stringLength] = 0;
}
