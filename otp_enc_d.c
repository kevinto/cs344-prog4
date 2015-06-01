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

void ProcessConnection(int socket);
void error(const char *msg);

// Signal handler to reap zombie processes
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
  int n, file_size, remain_data;
  char buffer[256];
  FILE *received_file;

  bzero(buffer, 256);

  // Get the file size
  recv(socket, buffer, 255, 0);
  file_size = atoi(buffer);
  printf("otp_enc_d: get file size: %d bytes\n", file_size);

  // Open the recieving file for writing
  received_file = fopen("filetorecieve.txt", "w");
  if (received_file == NULL)
  {
    printf("client: Failed to open file \n");

    exit(1);
  }

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

    fwrite(buffer, sizeof(char), n, received_file);
    remain_data -= n;
    // printf("Receive %d bytes and we hope :- %d bytes\n", n, remain_data);
  }

  fclose(received_file);
  close(socket);
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
