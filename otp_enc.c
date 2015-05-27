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
	}

	return 0;
}
