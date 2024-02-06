#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <poll.h>

#define MAX_CLIENTS 10
#define BACKLOG 5
#define PORT 8080

int main(){
  int sockfd, client_fds[MAX_CLIENTS], activity, n, newsockfd, i;
  struct sockaddr_in serv_addr;
  char buffer[1024];
  int yes = 1;
  socklen_t addr_len = sizeof(serv_addr);

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("server: socket");
    exit(0);
  }
  printf("[+]server socket created.\n");

  if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes))){
    perror("setsockopt");
    exit(0);
  }

  memset(&serv_addr, '\0', sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
    perror("server: bind");
    exit(0);
  }
  printf("[+]bind to port-8080 successful.\n");

  if(listen(sockfd, BACKLOG) < 0){
    perror("server: listen");
    exit(0);
  }
  printf("[+]listening...\n");

  for(i = 0; i < MAX_CLIENTS; i++){ //initialize client sockets
    client_fds[i] = 0;
  }

  while(1){
    struct pollfd fds[MAX_CLIENTS + 1];
    fds[0].fd = sockfd;
    fds[0].events = POLLIN;

    for(i = 0; i < MAX_CLIENTS; i++){ //add clients to pollfd structure
      if(client_fds[i] > 0){
        fds[i + 1].fd = client_fds[i];
        fds[i + 1].events = POLLIN;
      }
    }

    //poll to monitor different sockets (-1 for timeout will block until some event occurs)
    if((activity = poll(fds, MAX_CLIENTS + 1, -1)) < 0){
      perror("server: poll");
      exit(0);
    }

    //handle server socket
    if(fds[0].revents & POLLIN){
      if((newsockfd = accept(sockfd, (struct sockaddr *)&serv_addr, &addr_len)) < 0){
        perror("server: accept");
        exit(0);
      }
      // //add new client socket to client array
      // for(i = 0; i < MAX_CLIENTS; i++){
      //   if(client_fds[i] == 0){
      //     client_fds[i] = newsockfd;
      //     break;
      //   }
      // }
      const char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body>Hello, World!</body></html>"; //http request
      send(newsockfd, msg, strlen(msg),0);
      close(newsockfd);
      printf("[+]client closed.\n");
    }

    // //hanfle other clients
    // for(i = 0; i < MAX_CLIENTS; i++){
    //   if(client_fds[i] > 0 && fds[i + 1].revents & POLLIN){
    //     if((n = recv(client_fds[i], buffer, 1023, 0)) == 0){
    //       //client disconnected
    //       close(client_fds[i]);
    //       client_fds[i] = 0;
    //     } else {
    //         //echo back to the client
    //         send(client_fds[i], buffer, n, 0);
    //     }
    //   }
    // }
  }
  close(sockfd);

  return 0;
}
