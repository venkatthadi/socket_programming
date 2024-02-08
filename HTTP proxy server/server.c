#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define PORT 8888
#define BACKLOG 10
#define MAX_SIZE 1024

int main(){
    int sockfd, newsockfd, n;
    struct sockaddr_in serv_addr, client_addr;
    socklen_t cli_len;
    char buffer[MAX_SIZE];

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("server: socket");
        exit(0);
    }
    printf("[+]socket created.\n");

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
    printf("[+]binded to port - %d.\n", PORT);

    if(listen(sockfd, BACKLOG) < 0){
        perror("server: listen");
        exit(0);
    }
    printf("[+]listening...\n");

    char *msg = "hello, from destination server!";
    cli_len = sizeof(client_addr);
    while(1){
        if((newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &cli_len)) < 0){ // accept connection from proxy
            perror("newsockfd");
            break;
        }
        printf("[+]client accepted.\n");

        bzero(buffer, MAX_SIZE);
        if((n = recv(newsockfd, buffer, MAX_SIZE, 0)) < 0){ // receive HTTP request of client through proxy
            perror("server: recv");
            break;
        }
        printf("[+]request received.\n");

        if(send(newsockfd, msg, strlen(msg), 0) < 0){ // return HTTP response
            perror("server: send");
            break;
        }
        printf("[+]response sent!\n");
    }

    return 0;
}
