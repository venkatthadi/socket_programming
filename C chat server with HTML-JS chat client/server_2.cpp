#include <bits/stdc++.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>

using namespace std;

#define SA struct sockaddr
#define BACKLOG 10
#define MAX_CLIENTS 10
#define PORT "8080"
#define MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

class Server{
        int sockfd;
        void *get_inaddr(struct sockaddr *sa){
		if(sa -> sa_family == AF_INET){
			return &(((struct sockaddr_in *)sa) -> sin_addr);
		}
		else{
			return &(((struct sockaddr_in6 *)sa) -> sin6_addr);
		}
	}

	int serverCreation ()
	{
		struct addrinfo hints, *servinfo, *p;
		int yes = 1;
		int rv;
		memset (&hints, 0, sizeof (hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;// my ip
		
		// set the address of the server with the port info.
		if ((rv = getaddrinfo (NULL, PORT, &hints, &servinfo)) != 0)
		{
		fprintf (stderr, "getaddrinfo: %s\n",gai_strerror (rv));	
		return 1;
		}
		
		// loop through all the results and bind to the socket in the first we can
		for (p = servinfo; p != NULL; p = p -> ai_next)
		{
		sockfd = socket (p -> ai_family, p -> ai_socktype, p -> ai_protocol);
		if (sockfd == -1)
		{ 
			perror ("server: socket\n"); 
			continue; 
		} 
		
		//Reuse port
		if (setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int)) == -1)
		{
			perror ("setsockopt");
			exit (1);	
		}
			
		// it will help us to bind to the port.
		if (bind (sockfd, p -> ai_addr, p -> ai_addrlen) == -1) 
		{
			close (sockfd);
			perror ("server: bind");
			continue;
		}
		break;
		}
		
		// server will be listening with maximum simultaneos connections of BACKLOG
		if (listen (sockfd, BACKLOG) == -1)
		{ 
		perror ("listen");
		exit (1); 
		} 
		return sockfd;
	}

        public:
        Server(){
                serverCreation();
        }

        int getServSockfd(){
		return sockfd;
	}

        int connection_accepting(){
		int client_sockfd;
		struct sockaddr_storage their_addr;
		char s[INET6_ADDRSTRLEN];
		socklen_t sin_size = sizeof(their_addr);

		if((client_sockfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1){
				perror("[-]accept");
				return 1;
		}
		cout << "client accepted..." << endl;

		return client_sockfd;
	}

        int sendRequest(int client_sockfd, uint8_t *buff, int len){
		return send(client_sockfd, buff, len, 0);
	}

	int recvResponse(int client_sockfd, uint8_t *buff, int len){
		return recv(client_sockfd, buff, len, 0);
	}

	int sendRequest(int client_sockfd, char *buff, int len){
		return send(client_sockfd, buff, len, 0);
	}

	int recvResponse(int client_sockfd, char *buff, int len){
		return recv(client_sockfd, buff, len, 0);
	}

};


class WebSocketServer{
        Server tcp;

        void generate_random_mask(uint8_t *mask){
		srand (time (NULL));
		// Generate a random 32-bit mask
		for (size_t i = 0; i < 4; ++i)
			mask [i] = rand () & 0xFF;
	}

        void mask_payload (uint8_t *payload, size_t payload_length, uint8_t *mask) {
                for (size_t i = 0; i < payload_length; ++i)
                payload [i] ^= mask [i % 4];
        }

        void send_frame (const uint8_t *frame, size_t length, int connfd) {
                ssize_t bytes_sent = send (connfd, frame, length, 0);
                if (bytes_sent == -1)
                perror("Send failed");
                else
                printf("Pong Frame sent to client\n");
        }

        void send_pong(const char *payload, size_t payload_length, int connfd) {
                uint8_t pong_frame [128];
                pong_frame [0] = 0xA;
                pong_frame [1] = (uint8_t) payload_length;
                memcpy (pong_frame + 2, payload, payload_length);
                send_frame (pong_frame, payload_length + 2, connfd);
        }

        void handle_ping (const uint8_t *data, size_t length, int connfd) {
                char ping_payload [126];
                memcpy (ping_payload, data + 2, length - 2);
                ping_payload [length - 2] = '\0';
                send_pong (ping_payload, length - 2, connfd);
        }

        // Function to decode the header of a WebSocket frame
        int decode_websocket_frame_header(
                uint8_t *frame_buffer,
                uint8_t *fin,
                uint8_t *opcode,
                uint8_t *mask,
                uint64_t *payload_length
        ) {
                // Extract header bytes and mask
                *fin = (frame_buffer [0] >> 7) & 1;
		*opcode = frame_buffer [0] & 0x0F;
		*mask = (frame_buffer [1] >> 7) & 1;
		int n = 0;
		
		// Calculate payload length based on header type
		*payload_length = frame_buffer [1] & 0x7F;
		if (*payload_length == 126) 
		{
			n = 1;
			*payload_length = *(frame_buffer + 2);
			*payload_length <<= 8;
			*payload_length |= *(frame_buffer + 3);
		} 
		else if (*payload_length == 127) 
		{
			n = 2;
			*payload_length = 0;
			for (int i = 2; i < 10; ++i)
				*payload_length = (*payload_length << 8) | *(frame_buffer + i);
		}

		return  (2 + (n == 1 ? 2 : (n == 2 ? 8 : 0)));
        }

        int process_websocket_frame (uint8_t *data, size_t length, char **decoded_data, int connfd) {
                uint8_t fin, opcode, mask;
                uint64_t payload_length;
                uint8_t* masking_key;

                int header_size = decode_websocket_frame_header (data, &fin, &opcode, &mask, &payload_length);
                if (header_size == -1) {
                        printf ("Error decoding WebSocket frame header\n");
                        return -1;
                }
                
                if (mask){
                        masking_key = header_size + data;
                        header_size += 2;
                }
                header_size += 2;
                
                size_t payload_offset = header_size; 
                if (opcode == 0x9) {
                        handle_ping (data, length, connfd);
                        *decoded_data = NULL;
                        return 1;
                } 
                else if (opcode == 0x8) 
                        return 2;

                *decoded_data = (char *)malloc (payload_length + 1);
                
                if (mask)
                        for (size_t i = 0; i < payload_length; ++i)
                                (*decoded_data) [i] = data [payload_offset + i] ^ masking_key [i % 4];
                        (*decoded_data) [payload_length] = '\0';
                
                return 0;
        }

        // Function to encode a complete WebSocket frame
        int encode_websocket_frame (
                uint8_t fin,
                uint8_t opcode,
                uint8_t mask,
                uint64_t payload_length,
                uint8_t *payload,
                uint8_t *frame
        ) {
                int header_size = 2;
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
		// Mask payload if requested
		if (mask) 
		{
			generate_random_mask (frame + header_size - 4);
			mask_payload (payload, payload_length, frame + header_size - 4);
		}

		// Copy payload after header
		memcpy (frame + header_size, payload, payload_length);
		return header_size + payload_length; // Total frame size
        }  

        void calculate_websocket_accept(const char *clientKey, char *acceptKey) {
                char combinedKey [1024] = "";
                strcpy(combinedKey, clientKey);
                strcat (combinedKey, MAGIC_STRING);

                memset (acceptKey, '\0', 50);
                unsigned char sha1Hash [SHA_DIGEST_LENGTH];
                SHA1 (reinterpret_cast <const unsigned char*>(combinedKey), strlen (combinedKey), sha1Hash);

                BIO* b64 = BIO_new (BIO_f_base64 ());
                BIO_set_flags (b64, BIO_FLAGS_BASE64_NO_NL);

                BIO* bio = BIO_new (BIO_s_mem ());
                BIO_push (b64, bio);

                BIO_write (b64, sha1Hash, SHA_DIGEST_LENGTH);
                BIO_flush (b64);

                BUF_MEM* bptr;
                BIO_get_mem_ptr (b64, &bptr);

                strcpy (acceptKey, bptr -> data);

                size_t len = strlen (acceptKey);
                
                if (len > 0 && acceptKey [len - 1] == '\n') 
                acceptKey [len - 1] = '\0';
                
                acceptKey [28] = '\0';

                BIO_free_all (b64);
        }

        void handle_websocket_upgrade (int client_socket, char *request) {
                // Check if it's a WebSocket upgrade request
                if (strstr (request, "Upgrade: websocket") == NULL) {
                        fprintf (stderr, "Not a WebSocket upgrade request\n");
                        return;
                }

                // Extract the value of Sec-WebSocket-Key header
                char *key_start = strstr (request, "Sec-WebSocket-Key: ") + 19;
                char *key_end = strstr (key_start, "\r\n");
                
                if (!key_start || !key_end) {
                        fprintf (stderr, "Invalid Sec-WebSocket-Key header\n");
                        return;
                }
                *key_end = '\0';

                // Calculate Sec-WebSocket-Accept header
                char accept_key [1024];
                calculate_websocket_accept (key_start, accept_key);

                // Send WebSocket handshake response
                char *upgrade_response_format = strdup (
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: %s\r\n\r\n");

                char response [2048];
                sprintf (response, upgrade_response_format, accept_key);
                send (client_socket, response, strlen (response), 0);

                printf ("WebSocket handshake complete\n");
        }

        public:
        int webSocketCreate (){
                char buffer [2048];
                int client_socket = tcp.connection_accepting ();
                if (client_socket == -1)
                return -1;

                int len = tcp.recvResponse (client_socket, buffer, 2048);

                buffer [len] = '\0';

                handle_websocket_upgrade (client_socket, buffer);
                return client_socket;
        }

        // Function to send WebSocket frame to the client
        int send_websocket_frame (int client_socket, uint8_t fin, uint8_t opcode, char *payload) {
                // Encode the WebSocket frame
                uint8_t encoded_data [1024];

                int encoded_size = encode_websocket_frame (fin, opcode, 0, strlen (payload), (uint8_t *)payload, encoded_data);

                // Send the encoded message back to the client
                ssize_t bytes_sent = tcp.sendRequest (client_socket, encoded_data, encoded_size);

                if (bytes_sent == -1) {
                        perror ("Send failed");
                        return -1;
                }
                return 0;
        }

        void sendCloseFrame (int client_socket){
                uint8_t close_frame [] = {0x88, 0x00};
                tcp.sendRequest (client_socket, close_frame, sizeof (close_frame));

                cout << "Close frame sent to the client" << endl;
        }

        int recv_websocket_frame (char **decodedData, int client_socket){
                uint8_t data [2048]; 
                size_t length = 2048;
                int len = 0;
        
                if ((len = tcp.recvResponse (client_socket, data, length)) == -1)
                        return -1;

                return process_websocket_frame (data, length, decodedData, client_socket);
        }
};


class Client{
        int sockfd;
        char name[20];

        public:
        void setSockFd(int sockfd){
                this->sockfd = sockfd;
        }
        void setName(char *name){
                strcpy(this->name, name);
        }
        int getSockFd(){
                return sockfd; 
        }
        char *getName(){
                return name;
        }
};

class ChatServer{
        unordered_map <int, Client> clients;
        WebSocketServer websocket = WebSocketServer();
        mutex clients_mutex;

        public:
        void startServer(){
                cout << "server waiting on port " << PORT << endl;
                while(1){
                        int client_sockfd = addClient(websocket);
                        if(client_sockfd == -1)
                                continue;

                        clients_mutex.lock();
                        thread clients_thread(&ChatServer::handleClient, this, client_sockfd);
                        clients_thread.detach();
                }
        }

        int addClient(WebSocketServer &websocket){
                Client new_client;
                new_client.setSockFd(websocket.webSocketCreate());
                if(new_client.getSockFd() == -1)
                        return -1;

                clients_mutex.lock();
                clients[new_client.getSockFd()] = new_client;
                clients_mutex.unlock();

                return new_client.getSockFd();
        }

        void handleClose(int client_sockfd){
                clients_mutex.lock();
                close(clients[client_sockfd].getSockFd());
                clients.erase(client_sockfd);
                clients_mutex.unlock();
                pthread_exit(NULL);
        }

        void sendMessage(char *response, int sender_sockfd, char *username){
		int sockfd;
		int flag = 0;

		for(auto i: clients){
			if(strstr(username, i.second.getName())){
				flag = 1;
				sockfd = i.first;
				break;
			}
		}

		if(flag){
			websocket.send_websocket_frame(sockfd, 1, 1, response);
			char success[14] = "message sent.";
			websocket.send_websocket_frame(sender_sockfd, 1, 1, success);
		} else {
			char message[17] = "user not found!!";
			websocket.send_websocket_frame(sockfd, 1, 1, message);
		}
	}

        void broadcastMessage(char *response, int sender_sockfd){
                for(auto i: clients){
                        websocket.send_websocket_frame(i.first, 1, 1, response);
                }
        }

        void *handleClient(int sockfd){
                char name[30], *decoded_name = NULL;
                Client *new_client = &clients [sockfd];
                clients_mutex.unlock ();

                int flag = websocket.recv_websocket_frame(&decoded_name, sockfd);
                new_client->setName(decoded_name);
                char join [128];
                sprintf (join, "%s has joined the chat.", new_client->getName());
                printf ("%s\n", join);
                broadcastMessage (join, sockfd);


                while(1){
                        char buffer[1024];
                        char msg[100];
                        int flag;
                        // char reciever_name[100];
                        char full_message[1136];
                        char *decoded_data = NULL;

                        if ((flag = websocket.recv_websocket_frame(&decoded_data, sockfd)) == -1)
                                break;
                        
                        // cout << decoded_data <<endl;

                        //Ping frame
                        if (flag == 1)
                                continue;
                        
                        //Close frame
                        if (flag == 2)
                        {
                                websocket.sendCloseFrame(sockfd);
                                // broadcastMessage (join, sockfd);
                                break;
                        }

                        sprintf (full_message, "%s: %s", new_client->getName(), decoded_data);
                        // Broadcast the message to all clients
                        broadcastMessage (full_message, sockfd);
                }

                // Notify all clients about the user leaving
                char message [1000];
                sprintf (message, "%s has left the chat.", new_client->getName());
                printf ("%s\n", message);
                broadcastMessage (message, sockfd);
                bzero (message, sizeof (message));

                // Remove the disconnected client from the list
                handleClose (sockfd);
                pthread_exit (NULL);

        }
};

int main(){
	ChatServer server;
	server.startServer();

	return 0;
}