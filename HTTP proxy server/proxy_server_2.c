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
#include <sys/poll.h>

#define PROXY_PORT 8080
#define MAX_SIZE 65536
#define BACKLOG 10

void message_handler(int client_socket,int destination_socket){
    struct pollfd pollfds[2];
    pollfds[0].fd = client_socket;
    pollfds[0].events = POLLIN;
    pollfds[0].revents = 0;
    pollfds[1].fd = destination_socket;
    pollfds[1].events = POLLIN;
    pollfds[1].revents = 0;
    char data_buffer[2048];
    ssize_t n;
    while(1){
        if(poll(pollfds,2,-1) == -1){
            exit(1);
        }
    	for(int fd = 0; fd < 2;fd++){
    		if((pollfds[fd].revents & POLLIN) == POLLIN && fd == 0){
    			n = read(client_socket, data_buffer, sizeof(data_buffer));

			if (n <= 0) {
	    			return;
			}
			data_buffer[n]='\0';
			n = write(destination_socket, data_buffer, n);
            // printf("%d\n", n);
    		}
    		if((pollfds[fd].revents & POLLIN) == POLLIN && fd == 1){
    			n = read(destination_socket, data_buffer, sizeof(data_buffer));
			    if (n <= 0) {
	    			return;
			    }
    			data_buffer[n]='\0';
    			n = write(client_socket, data_buffer, n);
    		}
        }
    }
}

void handle_client(int client_sockfd){
    int bytes_received;
    char buffer[MAX_SIZE];
    char newbuffer[MAX_SIZE];

    bzero(buffer, MAX_SIZE);
    bytes_received = recv(client_sockfd, buffer, MAX_SIZE, 0);
    // printf("%s\n",buffer);
    // printf("[+]request received bytes - %d.\n", bytes_received);
    strcpy(newbuffer, buffer);

    if(strncmp(buffer, "CONNECT", 7) == 0){
        printf("[+]request (connect) - \n%s\n",buffer);
        char *host = strtok(buffer + 8, ":");
        int port = atoi(strtok(NULL, " "));
        printf("[+]connect request for host - %s, port - %d.\n", host, port);

        int server_sockfd;
        if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
            perror("server socket");
            close(server_sockfd);
            exit(0);
        }
        printf("[+]server socket created.\n");

        struct hostent *host_info;
        struct sockaddr_in serv_addr;
        memset(&serv_addr, '\0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        host_info = gethostbyname(host);
        if(!host_info){
            perror("host not found.\n");
            close(server_sockfd);
            close(client_sockfd);
            return;
        }
        // memcpy(&serv_addr.sin_addr, host_info->h_addr_list[0], host_info->h_length);
        struct in_addr **addr_list = (struct in_addr **)host_info->h_addr_list;
        if (addr_list[0] != NULL){
            serv_addr.sin_addr = *addr_list[0];
        } else{
            perror("host address not found");
            exit(1);
        }
        // char ip_address[INET_ADDRSTRLEN];
        // inet_ntop(AF_INET, &(serv_addr.sin_addr), ip_address, INET_ADDRSTRLEN);
        // printf("Server IP Address: %s\n", ip_address);

        if(connect(server_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
            perror("server: connect");
            exit(0);
        }
        printf("[+]server connected.\n");

        char *connect_response = "HTTP/1.1 200 Connection established\r\n\r\n";
        int n = send(client_sockfd, connect_response, strlen(connect_response), 0);
        printf("%d\n", n);
        printf("[+]response sent.\n");

        message_handler(client_sockfd, server_sockfd);
    }
    else{
        char *host = strstr(buffer, "Host: ");
        sscanf(host, "Host: %s", host);
        if(!host){
            perror("invalid request");
            exit(0);
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
            exit(0);
        }

        memset(&serv_addr, '\0', sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        // strcpy((char *)server->h_addr,(char *)&serv_addr.sin_addr.s_addr);
        serv_addr.sin_port = htons(80);
        struct in_addr **addr_list = (struct in_addr **)server->h_addr_list;
        if (addr_list[0] != NULL){
            serv_addr.sin_addr = *addr_list[0];
        } else{
            perror("host address not found");
            exit(0);
        }
        // char ip_address[INET_ADDRSTRLEN];
        // inet_ntop(AF_INET, &(serv_addr.sin_addr), ip_address, INET_ADDRSTRLEN);
        // printf("Server IP Address: %s\n", ip_address);

        if(connect(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
            perror("server: connect");
            exit(0);
        }
        printf("[+]server connected.\n");

        printf("%s\n",newbuffer);
        if(send(serv_sockfd, newbuffer, MAX_SIZE, 0) < 0){
            perror("sending request");
            exit(0);
        }
        printf("[+]request sent to server.\n");

        char response[MAX_SIZE];
        bzero(response, MAX_SIZE);
        if((bytes_received = recv(serv_sockfd, response, MAX_SIZE, 0)) < 0){
            perror("receiving response");
            exit(0);
        }
        printf("[+]response received from server bytes - %d.\n", bytes_received);
        printf("%s\n",response);

        if(send(client_sockfd, response, bytes_received, 0) < 0){
            perror("sending response");
            exit(0);
        }
        printf("[+]response forwarded to client.\n");

        close(client_sockfd);
        printf("[+]closed client.\n");
        close(serv_sockfd);
        printf("[+]closed server.\n");
    }
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
            continue;
        }
        printf("[+]client accepted.\n");
        int f;
        if((f = fork()) == 0){
            close(sockfd);
            handle_client(newsockfd);
            exit(0);
        }
        close(newsockfd);
    }
    close(sockfd);
    printf("[+]proxy closed.\n");

    return 0;
}
