#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>

#define PORT 8080
#define BACKLOG 20

int main(int argc, char *argv[]){
  int sockfd, newsockfd, n;
  struct sockaddr_in serv_addr;
  socklen_t addr_len = sizeof(serv_addr);
  char buffer[1024];

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("Server: socket");
    exit(0);
  }
  printf("[+]socket created.\n");

  int yes = 1;

  if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes)) < 0){ //allows reuse of local address and port
    perror("setsockopt");
    exit(0);
  }

  memset(&serv_addr, '\0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET; //IPv4
  serv_addr.sin_port = htons(PORT); //convert host byte order to network byte order
  serv_addr.sin_addr.s_addr = inet_ntop("127.0.0.1"); //converts address that is in string to binary
  // addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    perror("Server: bind failed");
    exit(0);
  }
  printf("[+]bind to the port(%d) successful.\n", PORT);

  if(listen(sockfd, BACKLOG) < 0){
    perror("Server: listen");
    exit(0);
  }
  printf("[+]listening...\n");

  if((newsockfd = accept(sockfd, (struct sockaddr *)&serv_addr, &addr_len)) < 0){
    perror("Server: accept");
    exit(0);
  }
  printf("[+]client accepted.\n");

  n = recv(newsockfd, buffer, 1023, 0);
  // printf("%s\n", buffer);

  char *hello = "Server: Hello..!";
  send(newsockfd, hello, strlen(hello), 0);
  printf("[+]message sent.\n");

  close(newsockfd);
  printf("[+]client closed.\n");

  close(sockfd);

  return 0;
}
