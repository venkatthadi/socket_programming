#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>

#define PORT 8080
#define BACKLOG 20
#define MAX_SIZE 1024

void send_file(int sockfd, const char *file_path){
  FILE *fp = fopen(file_path, "r");
  char *err = "404 - file not found...";

  if(!fp){
    send(sockfd, err, strlen(err), 0);
    fclose(fp);
    return;
  }

  char buffer[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";;
  size_t bytes;

  // sprintf(buffer, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n");
  send(sockfd, buffer, strlen(buffer), 0);

  while((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0){
    if(send(sockfd, buffer, bytes, 0) < 0){
      perror("send failed");
      break;
    }
  }

  fclose(fp);
}

void handle_client(int sockfd){
  char buffer[MAX_SIZE];
  int n;

  if((n = recv(sockfd, buffer, MAX_SIZE - 1, 0)) < 0){
    perror("server: recv");
    exit(0);
  }
  buffer[MAX_SIZE] = '\0';

  char *request_line = strtok(buffer, "\r\n"); // take the first line of the request
  if (!request_line) {
      perror("Malformed request");
      close(sockfd);
      return;
  }

  char *method = strtok(request_line, " "); // method stores the first word of request (GET)
  if (!method) {
      perror("Malformed request (method)");
      close(sockfd);
      return;
  }

  char *file_path = strtok(NULL, " "); // move one word to get the file path
  if (!file_path) {
      perror("Malformed request (file path)");
      close(sockfd);
      return;
  }

  if (strcmp(method, "GET") != 0) {
      perror("Invalid request (method)");
      close(sockfd);
      return;
  }

  if (file_path[0] == '/') {
      file_path++;
  }

  send_file(sockfd, file_path);

  close(sockfd);
}

int main(){
  int sockfd, newsockfd;
  struct sockaddr_in serv_addr;
  socklen_t addr_len = sizeof(serv_addr);

  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("server: socket");
    exit(0);
  }
  printf("[+]server socket created.\n");

  int yes = 1;
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
  printf("[+]bind successful.\n");

  if(listen(sockfd, BACKLOG) < 0){
    perror("server: listen");
    exit(0);
  }
  printf("[+]listening...\n");

  while(1){
    if((newsockfd = accept(sockfd, (struct sockaddr *)&serv_addr, &addr_len)) < 0){
      perror("server: accept");
      exit(0);
    }

    handle_client(newsockfd);
  }

  return 0;
}
