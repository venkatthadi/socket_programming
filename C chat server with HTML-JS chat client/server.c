#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/sha.h>

#define PORT 8080
#define BACKLOG 10
#define MAX_SIZE 65536

char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //global unique identifier

void handle_client(int cli_sockfd){
    char buffer[MAX_SIZE];
    int bytes_received;

    bzero(buffer, MAX_SIZE);
    if((bytes_received = recv(cli_sockfd, buffer, MAX_SIZE, 0)) < 0){
        perror("server: recv");
        continue;
    }
    printf("[+]request received.\n");

    char *key = strstr(buffer, "Sec-websocket-key: ");
    sscanf(key, "Sec-websocket-key: ", key);
    strcat(key, GUID); //concat guid to the websocket key

    sha1_hash(key);



}

int main(){
    int serv_sockfd, cli_sockfd;
    struct sockaddr_in serv_addr;
    size_t addr_len = sizeof(serv_addr);

    if((serv_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("server: socket");
        exit(0);
    }
    printf("[+]server socket created.\n");

    int yes = 1;
    if(setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes)) == -1){
        perror("setsockopt");
        exit(0);
    }

    memset(&serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        perror("server: bind");
        exit(0);
    }
    printf("[+]bind to port - %d.\n", PORT);

    if(listen(serv_sockfd, BACKLOG) < 0){
        perror("server: listen");
        exit(0);
    }
    printf("[+]listening...\n");

    while(1){
        if((cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&serv_addr, &addr_len)) < 0){
            perror(0);
            continue;
        }
        printf("[+]client accepted.\n");

        handle_client(cli_sockfd);
    }
}
