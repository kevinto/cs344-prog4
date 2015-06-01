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

void ScanBufferForInvalidChars(char *buffer, int bufferSize);

void error(const char *msg)
{
  perror(msg);
  exit(1);
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
  int sockfd, newsockfd, portno, n, file_size, remain_data;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  FILE *received_file;

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

    bzero(buffer, 256);

    // Get the file size
    recv(newsockfd, buffer, 255, 0);
    file_size = atoi(buffer);
    printf("otp_enc_d: get file size: %d bytes\n", file_size);

    // Open the recieving file for writing
    received_file = fopen("filetorecieve.txt", "w");
    if (received_file == NULL)
    {
      printf("client: Failed to open file \n");

      exit(1);
    }

    // Get the file data and save into a file
    bzero(buffer, 256);
    remain_data = file_size;
    while (((n = recv(newsockfd, buffer, 1, 0)) > 0) && (remain_data > 0))
    {
      if (buffer[0] == '\0')
      {
        continue;
      }

      fwrite(buffer, sizeof(char), n, received_file);
      remain_data -= n;
      printf("Receive %d bytes and we hope :- %d bytes\n", n, remain_data);
    }

    fclose(received_file);

    // Open file and try to print it
    // Open the recieving file for writing
    // received_file = fopen("filetorecieve.txt", O_RDONLY);
    // if (received_file == NULL)
    // {
    //   printf("client: Failed to open file \n");

    //   exit(1);
    // }

    // printf("trying to print\n");
    // int c;
    // if (received_file) {
    //   while ((c = getc(received_file)) != EOF)
    //     putchar(c);
    //   fclose(received_file);
    // }

    close(newsockfd);
  }

  close(sockfd);
  return 0;
}

void ScanBufferForInvalidChars(char *buffer, int bufferSize)
{
  int i;
  for (i = 0; i < bufferSize; i++)
  {
    if (buffer[i] == '\0')
    {
      buffer[i] = ';';
    }
  }
}