#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>

int main(int argc, char *argv[]){
  //char *ip = "127.0.0.1";

  if(argc < 3){
    fprintf(stderr, "usage- file [hostname] [port]\n");
    exit(0);
  }

  int sockfd, port, n;
  char buffer[1024];
  struct sockaddr_in serv_addr;
  struct hostent *server;
  time_t t;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0){
    perror("client: socket");
    exit(1);
  }
  printf("[+]socket created\n");

  server = gethostbyname(argv[1]);
  if(server == NULL){
    fprintf(stderr, "No such host\n");
    exit(0);
  }

  port = atoi(argv[2]);
  // bzero((char *)&serv_addr, sizeof(serv_addr));
  memset(&serv_addr, '\0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  //bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h.length);
  strncpy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);
  //serv_addr.sin_addr.s_addr = inet_addr(ip);

  if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    perror("connection failed.");
    exit(1);
  }
  printf("[+]server connected\n");

  bzero(buffer, 1024);
  fgets(buffer, 1024, stdin);
  n = send(sockfd, buffer, strlen(buffer), 0);
  if(n < 0){
    perror("client: write");
    exit(1);
  }

  bzero(buffer, 1024);
  n = recv(sockfd, buffer, 1024, 0);
  if(n < 0){
    perror("client: read");
    exit(1);
  }

  time(&t);
  strcat(buffer, ctime(&t));

  printf("Server: %s\n", buffer);


  close(sockfd);

  return 0;
}
