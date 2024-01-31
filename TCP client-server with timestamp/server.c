#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define BACKLOG 10

int main(int argc, char *argv[]){ //argv[0] portnumber
  char *ip = "127.0.0.1";

  if(argc < 2){
    fprintf(stderr, "usage- file [portnumber]\n");
    exit(1);
  }

  int sockfd, newsockfd, port, n;
  char buffer[1024];
  struct sockaddr_in serv_addr, client_addr;
  socklen_t client_len;
  time_t t;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0){
    perror("Server: socket.");
    exit(1);
  }
  printf("[+]Socket created.\n");

  // bzero((char *)&serv_addr, sizeof(serv_addr));
  memset(&serv_addr, '\0', sizeof(serv_addr));
  port = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);

  if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    perror("Server: bind");
    exit(1);
  }
  printf("[+]Bind to port number - %d.\n", port);

  listen(sockfd, BACKLOG);
  client_len = sizeof(client_addr);

  while(1){
    newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if(newsockfd < 0){
      perror("Server: accept");
      exit(1);
    }
    printf("[+]Accepted client.\n");

    bzero(buffer, 1024);
    n = recv(newsockfd, buffer, 1024, 0);
    if(n < 0){
      perror("Server: read");
      exit(1);
    }

    time(&t);
    printf("Message received at - %s\n", ctime(&t));
    strcat(buffer, ctime(&t));

    n = send(newsockfd, buffer, strlen(buffer), 0);
    if(n < 0){
      perror("Server: write");
      exit(1);
    }

    close(newsockfd);
    printf("[+]client closed\n");
  }

  close(sockfd);

  return 0;
}
