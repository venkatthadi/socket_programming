#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>

#define PROXY_PORT 8080
#define MAX_SIZE 1024
#define BACKLOG 10

void handle_client(int client_sock){
    char buffer[MAX_SIZE];
    int n;

    bzero(buffer, MAX_SIZE);
    if((n = recv(client_sock, buffer, MAX_SIZE, 0)) < 0){ // receive the HTTP request from client
        perror("server: recv");
        close(client_sock);
        return;
    }
    printf("[+]request received.\n");

    // destination server : 8888
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock < 0){
        perror("server_sock");
        close(client_sock);
        return;
    }
    printf("[+]server socket created.\n");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888); // port of destination server
    serv_addr.sin_addr.s_addr = INADDR_ANY; // address of destination

    if(connect(server_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        perror("connection failed");
        close(server_sock);
        close(client_sock);
        return;
    }
    printf("[+]connected to destination server.\n");

    if(send(server_sock, buffer, n, 0) < 0){ // forward HTTP request to server
        perror("failed to forward client's request");
        close(client_sock);
        return;
    }
    printf("[+]HTTP request sent to server.\n");

    bzero(buffer, MAX_SIZE);
    if((n = recv(server_sock, buffer, MAX_SIZE, 0)) >= 0){ // receive response from destination server
        buffer[n] = '\0';
        if(send(client_sock, buffer, n, 0) < 0){ // send HTTP response to client
            perror("failed to send to client");
        }
    }
    printf("[+]HTTP response sent to client.\n");

    close(client_sock);
    close(server_sock);
}

int main(){
    int sockfd, newsockfd;
    struct sockaddr_in prox_addr, client_addr;
    socklen_t cli_len;

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

    // socket for proxy server
    memset(&prox_addr, '\0', sizeof(prox_addr));
    prox_addr.sin_family = AF_INET;
    prox_addr.sin_port = htons(PROXY_PORT);
    prox_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (struct sockaddr *)&prox_addr, sizeof(prox_addr)) < 0){
        perror("server: bind");
        exit(0);
    }
    printf("[+]binded to port - %d.\n", PROXY_PORT);

    if(listen(sockfd, BACKLOG) < 0){
        perror("server: listen");
        exit(0);
    }
    printf("[+]listening...\n");

    cli_len = sizeof(client_addr);
    while(1){
        if((newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &cli_len)) < 0){ // accept the client connection
            perror("server: client socket");
            break;
        }
        printf("[+]client accepted.\n");

        handle_client(newsockfd);
    }

    return 0;
}
