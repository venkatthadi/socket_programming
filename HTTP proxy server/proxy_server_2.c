#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PROXY_PORT 8080
#define MAX_SIZE 4096
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
    while(1){
        if((newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &cli_len)) < 0){ // accept the client connection
            perror("server: client socket");
            exit(0);
        }
        printf("[+]client accepted.\n");

        int bytes_received;
        char buffer[MAX_SIZE];
        char newbuffer[MAX_SIZE];

        bzero(buffer, MAX_SIZE);
        bytes_received = recv(newsockfd, buffer, MAX_SIZE, 0);
        // printf("%s\n",buffer);
        printf("[+]request received.\n");
        strcpy(newbuffer, buffer);

        char *host = strstr(buffer, "Host: ");
        // char *host_site;
        sscanf(host, "Host: %s", host);
        if(!host){
            perror("invalid request");
            // exit(0);
            break;
        }
        printf("request - %s\n", host);

        struct hostent *server;
        struct sockaddr_in serv_addr;
        int serv_sockfd;

        if((serv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("server: socket");
            exit(0);
        }
        printf("[+]server socket created.\n");

        // remote server
        server = gethostbyname(host);
        if(server == NULL){
            perror("no such host");
            // exit(0);
            break;
        }

        // char **addr;
        // for (addr = server->h_addr_list; *addr != NULL; addr++) {
        //     printf("%s ", inet_ntoa(*(struct in_addr *) *addr));
        // }
        // printf("\n");

        memset(&serv_addr, '\0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        // strcpy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr);
        serv_addr.sin_port = htons(80);
        struct in_addr **addr_list = (struct in_addr **)server->h_addr_list;
        if (addr_list[0] != NULL){
            serv_addr.sin_addr = *addr_list[0];
        } else{
            perror("host address not found");
            exit(1);
        }
        char ip_address[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(serv_addr.sin_addr), ip_address, INET_ADDRSTRLEN);
        printf("Server IP Address: %s\n", ip_address);

        if(connect(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
            perror("server: connect");
            exit(0);
        }
        printf("[+]server connected.\n");

        // printf("%s\n",newbuffer);
        send(serv_sockfd, buffer, strlen(buffer), 0);
        printf("[+]request sent to server.\n");

        char response[MAX_SIZE];
        bzero(response, MAX_SIZE);
        if((bytes_received = recv(serv_sockfd, response, MAX_SIZE, 0)) < 0){
                perror("receiving response");
                break;
        }
        response[bytes_received] = '\0';
        printf("[+]response received from server.\n");
        printf("%s\n",response);

        if(send(newsockfd, response, strlen(response), 0) < 0){
            perror("sending response");
            break;
        }
        printf("[+]response forwarded to client.\n");

        // handle_client(newsockfd);
        close(serv_sockfd);
        printf("[+]closed server.\n");
        close(newsockfd);
        printf("[+]closed client.\n");
    }
    close(sockfd);

    return 0;
}
