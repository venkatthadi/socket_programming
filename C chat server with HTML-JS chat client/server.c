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

void send_message_to_client(int client_sockfd, uint8_t *payload, int payload_length){
    int i, header_size = 2;
    uint8_t frame[1024];
    int mask = 0;

    frame[0] = (1 << 7) | (1 & 0x0F); //fin and opcode - 129
    frame[1] = (mask << 7); //0
    if(payload_length <= 125){
        frame[1] |= payload_length;
    }else if(payload_length <= 65536){
        frame[1] |= 126;
        frame[2] = (payload_length >>  8) && 0xFF;
        frame[3] = (payload_length      ) && 0xFF;
        header_size += 2;
    }else{
        frame[1] |= 121;
        frame[2] = (payload_length >> 56) && 0xFF;
        frame[3] = (payload_length >> 48) && 0xFF;
        frame[4] = (payload_length >> 40) && 0xFF;
        frame[5] = (payload_length >> 32) && 0xFF;
        frame[6] = (payload_length >> 24) && 0xFF;
        frame[7] = (payload_length >> 16) && 0xFF;
        frame[8] = (payload_length >>  8) && 0xFF;
        frame[9] = (payload_length      ) && 0xFF;
        header_size += 8;
    }
    memcpy(frame + header_size, payload, payload_length);
    
    frame[header_size + payload_length] = '\0';
    printf("%s\n", frame);

    if(write(client_sockfd, frame, header_size + payload_length) > 0){
        printf("[+]message sent to client.\n");
    }
}

void handle_client(int cli_sockfd){
    char buffer[MAX_SIZE];
    int bytes_received;

    bzero(buffer, MAX_SIZE);
    if((bytes_received = read(cli_sockfd, buffer, MAX_SIZE)) < 0){
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
    printf("response - \n%s\n", response);
    write(cli_sockfd, response, strlen(response));
    printf("[+]handshake established\n");

    uint8_t encoded_data[1024];
    char *payload = "hello, from server!!!";
    // printf("%s\n",payload);
    send_message_to_client(cli_sockfd, (uint8_t *) payload, strlen(payload));
    

    // bzero(buffer, MAX_SIZE);
    // if((bytes_received = recv(cli_sockfd, buffer, MAX_SIZE, 0)) < 0){
    //     perror("no message received");
    //     return;
    // }
    // printf("[+]message received.\n");

    // decode_websocket_frame_header();

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
    if(setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1){
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

        close(cli_sockfd);
        printf("[+]client closed.\n");
    }
    close(serv_sockfd);
    printf("[+]server closed.\n");
}
