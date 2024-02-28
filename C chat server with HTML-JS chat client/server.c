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
#include <pthread.h> 

#define PORT 8080
#define BACKLOG 10
#define MAX_SIZE 65536
#define MAX_CLIENTS 100

char *GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //global unique identifier
//store details of clients
struct socket_info{
    int sockfd;
    int user_ID;
}clients_sockfds[MAX_CLIENTS];

//Encodes a binary safe base 64 string
void Base64Encode(char *client_key, char *accept_key) { 
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

void send_frame_to_client(int client_sockfd, uint8_t response[], int header_size, int payload_length){
    if(send(client_sockfd, response, header_size + payload_length, 0) > 0){
        printf("[+]message sent to client.\n");
    }
}

void encode_frame_and_respond(int client_sockfd, uint8_t *payload, int payload_length){
    int header_size = 2;
    uint8_t frame[10];
    int mask = 0, fin = 1, opcode = 1;

    frame[0] = (fin << 7) | (opcode & 0x0F); //129(1000 0001)
    frame[1] = (mask << 7); //0
    if(payload_length <= 125){
        frame[1] |= payload_length;
    }else if(payload_length <= 65536){
        frame[1] |= 126;
        frame[2] = (payload_length >>  8) & 0xFF;
        frame[3] = (payload_length      ) & 0xFF;
        header_size += 2;
    }else{
        frame[1] |= 127;
        frame[2] = (payload_length >> 56) & 0xFF;
        frame[3] = (payload_length >> 48) & 0xFF;
        frame[4] = (payload_length >> 40) & 0xFF;
        frame[5] = (payload_length >> 32) & 0xFF;
        frame[6] = (payload_length >> 24) & 0xFF;
        frame[7] = (payload_length >> 16) & 0xFF;
        frame[8] = (payload_length >>  8) & 0xFF;
        frame[9] = (payload_length      ) & 0xFF;
        header_size += 8;
    }

    int32_t i, respInd = 0;
    uint8_t response[header_size + payload_length];

    // if(mask){
        //create mask
        //add it to frame
    // }
    
    for(i = 0; i < header_size; i++){
        response[respInd++] = frame[i];
    }
    for(i = 0; i < payload_length; i++){
        response[respInd++] = payload[i];
    }

    send_frame_to_client(client_sockfd, response, header_size, payload_length);
}

void decode_websocket_frame_header(uint8_t buffer[], int buffer_length){
    //decode header for payload length and mask bit
    int payload_length = buffer[1] & 0x7F;
    // printf("payload length - %d\n", payload_length);
    // printf("buffer length - %d\n", buffer_length);
    uint8_t message[payload_length];
    int mask_bit = buffer[1] && (1 << 7); //always 1
    // printf("mask bit - %d\n", mask_bit);

    //move forward until we reach mask
    int indexFirstMask = 2;
    if(payload_length == 126){
        indexFirstMask = 4;
    }else if(payload_length == 127){
        indexFirstMask = 10;
    }

    //get mask
    size_t i;
    uint8_t mask[4];
    for(i = 0; i < 4; i++){
        mask[i] = buffer[indexFirstMask + i];
    }

    //move to the start of payload
    i += indexFirstMask;
    int ind = 0;
    while(i++ < buffer_length){
        message[ind] = buffer[indexFirstMask + 4 + ind] ^ mask[ind % 4];
        ind++;
    }
    // printf("message length - %d\n", strlen(message));
    message[ind] = '\0';
    printf("received message - %s\n", message);
}

void recv_message_from_client(int client_sockfd){
    int bytes_received;
    uint8_t buffer[MAX_SIZE];
    
    memset(buffer, 0, MAX_SIZE);
    if((bytes_received = recv(client_sockfd, buffer, MAX_SIZE, 0)) > 0){
        printf("[+]message received.\n");
    }
    // printf("message received - \n%s\n", buffer);
    decode_websocket_frame_header(buffer, strlen(buffer));
}

void handle_client(struct socket_info client_sockfd){
    char buffer[MAX_SIZE];
    int bytes_received;

    bzero(buffer, MAX_SIZE);
    if((bytes_received = read(client_sockfd.sockfd, buffer, MAX_SIZE)) < 0){
        perror("server: recv");
        return;
    }
    printf("[+]request received.\n");
    printf("request - \n%s\n", buffer);

    //prepare response
    char *key = strstr(buffer, "Sec-WebSocket-Key: ");
    char websoc_key[MAX_SIZE];
    memset(&websoc_key, '\0', MAX_SIZE);
    sscanf(key, "Sec-WebSocket-Key: %s", websoc_key);//extract the key
    // printf("[+]key - %s\n", websoc_key);

    char hash[1024];
    Base64Encode(websoc_key, hash);

    //send the response to upgrade the connection
    char response[MAX_SIZE];
    sprintf(response, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", hash);
    printf("response - \n%s\n", response);
    write(client_sockfd.sockfd, response, strlen(response));
    printf("[+]handshake established\n");

    uint8_t encoded_data[1024];
    char payload[35];
    sprintf(payload, "hello, from server... user - %d!!!", client_sockfd.user_ID);
    // printf("%s\n",payload);
    encode_frame_and_respond(client_sockfd.sockfd, (uint8_t *) payload, strlen(payload));

    recv_message_from_client(client_sockfd.sockfd);
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

    int i;
    for(i = 0; i < MAX_CLIENTS; i++){
        clients_sockfds[i].sockfd = 0;
        clients_sockfds[i].user_ID = i + 1;
    }
    while(1){
        if((cli_sockfd = accept(serv_sockfd, (struct sockaddr *)&serv_addr, &addr_len)) < 0){
            perror(0);
            continue;
        }
        for(i = 0; i < MAX_CLIENTS; i++){
            if(clients_sockfds[i].sockfd == 0){
                clients_sockfds[i].sockfd = cli_sockfd;//check for empty slot in array to store details of client
                break;
            }
        }
        if(i == MAX_CLIENTS){
            perror("[-]max number of clients reached\n");
            break;
        }
        printf("[+]client accepted.\n");

        handle_client(clients_sockfds[i]);

        // close(cli_sockfd);
        // printf("[+]client closed.\n");
    }
    close(serv_sockfd);
    printf("[+]server closed.\n");
}
