#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdint.h>

#define PORT 8080
#define BACKLOG 10
#define MAX_SIZE 65536

char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //global unique identifier

void Base64Encode(char *client_key, char *accept_key) { //Encodes a binary safe base 64 string
    char combined_key[1024];
    strcpy(combined_key, client_key);
    strcat(combined_key, GUID);

    unsigned char sha1_hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)combined_key, strlen(combined_key), sha1_hash);

    BIO *b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    BIO *bio = BIO_new(BIO_s_mem());
    BIO_push(b64, bio);

    BIO_write(b64, sha1_hash, SHA_DIGEST_LENGTH);
    BIO_flush(b64);

    BUF_MEM *bptr;
    BIO_get_mem_ptr(b64, &bptr);

    strcpy(accept_key, bptr->data);


    size_t len = strlen(accept_key);
    if (len > 0 && accept_key[len - 1] == '\n') {
        accept_key[len - 1] = '\0';
    }

    BIO_free_all(b64);
}

void handle_client(int cli_sockfd){
    char buffer[MAX_SIZE];
    int bytes_received;

    bzero(buffer, MAX_SIZE);
    if((bytes_received = recv(cli_sockfd, buffer, MAX_SIZE, 0)) < 0){
        perror("server: recv");
        return;
    }
    printf("[+]request received.\n");
    printf("request - \n%s\n", buffer);

    //prepare response
    char *key = strstr(buffer, "Sec-WebSocket-Key: ");
    char websoc_key[MAX_SIZE];
    memset(&websoc_key, '\0', MAX_SIZE);
    sscanf(key, "Sec-WebSocket-Key: %s", websoc_key);
    printf("[+]key - %s\n", websoc_key);

    char hash[1024];
    Base64Encode(websoc_key, hash);

    char response[MAX_SIZE];
    sprintf(response, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n", hash);
    send(cli_sockfd, response, strlen(response), 0);
    printf("[+]handshake established\n");

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

        char message[MAX_SIZE];
        bzero(message, MAX_SIZE);
        recv(cli_sockfd, message, MAX_SIZE, 0);
        printf("[+]received message - %s\n", message);

        close(cli_sockfd);
        printf("[+]client closed.\n");
    }
    close(serv_sockfd);
    printf("[+]server closed.\n");
}
