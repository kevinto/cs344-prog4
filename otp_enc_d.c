/**************************************************************
 * *  Filename: otp_enc_d.c
 * *  Coded by: Kevin To
 * *  Purpose -
 * *
 * ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

// stdin fix 
#include <termios.h>

#define MAX_ARGS 513 
#define MAX_COMMAND_LENGTH 513 
#define MAX_ERR_MSG_LENGTH 80 

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
int main()
{
	return 0;
}