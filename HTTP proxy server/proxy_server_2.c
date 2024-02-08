#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>

#define PROXY_PORT 8080
#define MAX_SIZE 1024
#define BACKLOG 10

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
    // while(1){
        if((newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &cli_len)) < 0){ // accept the client connection
            perror("server: client socket");
            exit(0);
        }
        printf("[+]client accepted.\n");

        int bytes_received;
        char buffer[MAX_SIZE];

        bzero(buffer, MAX_SIZE);
        bytes_received = recv(newsockfd, buffer, MAX_SIZE, 0);
        printf("%s\n",buffer);
        printf("[+]request received.\n");

        char *host = strstr(buffer, "Host: ");
        if(!host){
            perror("invalid request");
            exit(0);
            // break;
        }
        printf("request - %s\n", host + 6);

        struct hostent *server;
        struct sockaddr_in serv_addr;

        // remote server
        server = gethostbyname(host);
        if(server == NULL){
            perror("no such host");
            exit(0);
            // break;
        }

        memset(&serv_addr, '\0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(server->h_addr);
        serv_addr.sin_port = htons(80);



        // send(newsockfd, msg, strlen(msg), 0);

        // handle_client(newsockfd);
        close(newsockfd);
        printf("[+]closed client/\n");
    // }
    close(sockfd);

    return 0;
}
