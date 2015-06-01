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
    // n = read(newsockfd,buffer,9);
    // if (n < 0)
    // {
    //     error("otp_enc_d: ERROR reading from socket");
    // }

    // This does not work for large file sizes
    // while (1) // http://geeky.altervista.org/blog/receive-tcp-packets-from-a-socket-c-language/?doing_wp_cron=1432966436.4647989273071289062500
    // {
    //   n = read(newsockfd, buffer, 1);
    //   if (n < 0) error("ERROR reading from socket");
    //   fflush(stdout);

    //   if (n == 0 || buffer[0] == ';')
    //   {
    //     break;
    //   }

    //   // This causes the flow not to work anymore
    //   // printf("%c", buffer[0]);
    // }

    // TODO figure out how to save what is sent through

    // This is the block to write back messages to the client
    // printf("Here is the message: %s\n", buffer);
    // bzero(buffer, 256);
    // n = write(newsockfd, "I got your message", 18);
    // if (n < 0)
    // {
    //   error("ERROR writing to socket");
    // }

    // Get the file size
    recv(newsockfd, buffer, 255, 0);
    file_size = atoi(buffer);
    printf("otp_enc_d: get file size: %d bytes\n", file_size);

    close(newsockfd);
  }

  close(sockfd);
  return 0;
}
