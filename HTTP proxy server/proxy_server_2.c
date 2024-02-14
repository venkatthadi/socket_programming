#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<error.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<unistd.h>

#define PROXY_PORT 8080
#define MAX_SIZE 65536
#define BACKLOG 10

// void handle_client(int client_sockfd, char *port){
//     char request[MAX_SIZE], host[MAX_SIZE], response[MAX_SIZE];
//     int server_sockfd, i, j, k = 0, n;
//     size_t r;
//     struct addrinfo *hserv, hints, *p;
//     memset(request, 0, sizeof(request));
//
//     if((r = read(client_sockfd, request, MAX_SIZE)) > 0){
//         printf("[+]request - \n%s\n", request);
//
//         memset(&host, '\0', strlen(host));
//         for(i = 0; i < sizeof(request)-2; i++){
//             if(request[i] == '/' && request[i + 1] == '/'){
//                 k = i + 2;
//                 for(j = 0; request[k] != '/'; j++){
//                     host[j] = request[k];
//                     k++;
//                 }
//                 host[j] = '\0';
//                 break;
//             }
//         }
//
//         char* portchr = strstr(host, ":");
//         if(portchr != NULL){
//             *portchr = '\0';
//             port = portchr + 1;
//         }
//         printf("[+]server name : %s.\n", host);
//         printf("[+]port : %s.\n", port);
//
//         memset(&hints, '\0', sizeof(hints));
//         hints.ai_family = AF_UNSPEC;
//         hints.ai_socktype = SOCK_STREAM;
//         if((n = getaddrinfo(host, port, &hints, &hserv)) != 0){
//             perror("getaddrinfo");
//             exit(0);
//         }
//         for(p = hserv; p != NULL; p = p->ai_next){
//             if((server_sockfd = socket(p->ai_family, p->ai_socktype, 0)) < 0){
//                 perror("server socket");
//                 continue;
//             }
//             if(connect(server_sockfd, p->ai_addr, p->ai_addrlen) < 0){
//                 close(server_sockfd);
//                 perror("server connect");
//                 continue;
//             }
//             break;
//         }
//         if(p == NULL){
//             printf("not connected.\n");
//             exit(0);
//         }
//         printf("[+]connected to remote server.\n");
//
//         if(send(server_sockfd, request, MAX_SIZE, 0) < 0){
//             perror("send error");
//             exit(0);
//         }
//         printf("[+]request sent to remote server.\n");
//
//         int bytes_received = 0;
//         // if((bytes_received = recv(serv_sockfd, response, MAX_SIZE, 0)) < 0){
//         //     perror("server recv");
//         //     exit(0);
//         // }
//         // print("[+]response received from remote server.\n");
//         memset(&response, '\0', sizeof(response));
//         while((bytes_received = recv(server_sockfd, response, MAX_SIZE, 0)) > 0){
//             // printf("%s\n", response);
//             send(client_sockfd, response, bytes_received, 0);
//         }
//
//         close(server_sockfd);
//         printf("[+]server closed.\n");
//     }
//     close(client_sockfd);
//     printf("[+]client closed.\n");
// }

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

        // handle_client(newsockfd, "80");

        int bytes_received;
        char buffer[MAX_SIZE];
        char newbuffer[MAX_SIZE];

        bzero(buffer, MAX_SIZE);
        bytes_received = recv(newsockfd, buffer, MAX_SIZE, 0);
        // printf("%s\n",buffer);
        printf("[+]request received bytes - %d.\n", bytes_received);
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

        printf("%s\n",newbuffer);
        if(send(serv_sockfd, newbuffer, MAX_SIZE, 0) < 0){
            perror("sending request");
            break;
            // exit(0);
        }
        printf("[+]request sent to server.\n");

        char response[MAX_SIZE];
        bzero(response, MAX_SIZE);
        if((bytes_received = recv(serv_sockfd, response, MAX_SIZE, 0)) < 0){
                perror("receiving response");
                break;
                // exit(0);
        }
        // response[bytes_received] = '\0';
        printf("[+]response received from server bytes - %d.\n", bytes_received);
        printf("%s\n",response);

        if(send(newsockfd, response, bytes_received, 0) < 0){
            perror("sending response");
            break;
            // exit(0);
        }
        printf("[+]response forwarded to client.\n");

        // handle_client(newsockfd);
        close(newsockfd);
        printf("[+]closed client.\n");
        close(serv_sockfd);
        printf("[+]closed server.\n");
    }
    close(sockfd);
    printf("[+]proxy closed.\n");

    return 0;
}
