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
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <netdb.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

void ProcessConnection(int socket);
void error(const char *msg);
int GetTempFD();
void RemoveNewLineAndAddNullTerm(char *stringValue);
int ReceiveClientFile(int socket, int tempFd);
void PutFileIntoString(char *plaintextString, int plaintextFileSize, int plaintextTempFd);

// Signal handler to clean up zombie processes
static void wait_for_child(int sig)
{
  while (waitpid(-1, NULL, WNOHANG) > 0);
}

/**************************************************************
 * * Entry:
 * *  N/a
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
  int sockfd, newsockfd, portno, pid;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  struct sigaction sa;

  if (argc < 2)
  {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }

  // Creates a new socket. It returns a file desc that
  // refers to the socket
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    error("ERROR opening socket");
  }

  // Sets all values in the struct to 0
  bzero((char *) &serv_addr, sizeof(serv_addr));

  // Gets the port number from the param sent in
  portno = atoi(argv[1]);

  // Set the socket address stuct
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  // Bind the socket to an address
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0)
  {
    error("ERROR on binding");
  }

  // Listens to the socket for connections. Max of 5 connections
  // queued
  listen(sockfd, 5);

  // Set up the signal handler
  sa.sa_handler = wait_for_child;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    return 1;
  }

  // Get client connection
  while (1)
  {
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd,
                       (struct sockaddr *) &cli_addr,
                       &clilen);
    if (newsockfd < 0)
    {
      error("ERROR on accept");
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

  close(sockfd);
  return 0;
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
  int plaintextTempFd, plaintextFileSize, keyTempFd, keyFileSize;

  // ------ Get plaintext data ----------------
  plaintextTempFd = GetTempFD(); // Create temp file to hold the recieved data
  plaintextFileSize = ReceiveClientFile(socket, plaintextTempFd);

  char *plaintextString = malloc(plaintextFileSize + 1);
  PutFileIntoString(plaintextString, plaintextFileSize, plaintextTempFd);
  printf("plaintext: %s\n", plaintextString); // For debug only
  
  // ------ Get key data ----------------
  keyTempFd = GetTempFD(); // Create temp file to hold the recieved data
  keyFileSize = ReceiveClientFile(socket, keyTempFd);

  char *keyString = malloc(keyFileSize + 1);
  PutFileIntoString(keyString, keyFileSize, keyTempFd);
  printf("key: %s\n", keyString); // For debug only

  // Think about how big the encypted string is...
  // DoEncryption(plaintextString, plaintextFileSize, keyString, keyStringSize, encryptedString);

  free(plaintextString); 
  close(socket);
}

void PutFileIntoString(char *stringHolder, int stringSize, int fileFd)
{
  read(fileFd, stringHolder, stringSize);
  stringHolder[stringSize + 1] = 0;
}

int ReceiveClientFile(int socket, int tempFd)
{
  int n, file_size, remain_data;
  char buffer[256];
  bzero(buffer, 256);

  // Get the file size
  recv(socket, buffer, 255, 0);
  // read(socket, buffer, 255); // alternative to the above

  file_size = atoi(buffer);
  // printf("otp_enc_d: get file size: %d bytes\n", file_size); // For debug

  // Get the file data and save into a file 1 character at a time
  bzero(buffer, 256);
  remain_data = file_size;
  while (((n = recv(socket, buffer, 1, 0)) > 0) && (remain_data > 0))
  {
    // Do not write any null characters to the file
    if (buffer[0] == '\0')
    {
      continue;
    }

    errno = 0;
    if (-1 == write(tempFd, buffer, 1))
    {
      printf("write failed with error [%s]\n", strerror(errno));
    }

    remain_data -= n;

    // For debugging
    // printf("Receive %d bytes and we hope :- %d bytes\n", n, remain_data);
  }

  errno = 0;
  // rewind the stream pointer to the start of temporary file
  if (-1 == lseek(tempFd, 0, SEEK_SET))
  {
    printf("\n lseek failed with error [%s]\n", strerror(errno));
  }

  // Debug code to read the temp file
  // while (read(tempFd, buffer, 1) > 0)
  // {
  //   printf("%s", buffer);
  // }
  // printf("\n");

  bzero(buffer, 256);
  return file_size;
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

// found here: http://www.thegeekstuff.com/2012/06/c-temporary-files/
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