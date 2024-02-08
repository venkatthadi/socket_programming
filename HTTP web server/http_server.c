#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define FILE_PATH "."

void handle_get_request(int client_socket, const char *url) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s%s", FILE_PATH, url);

    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        // File not found, send 404 response
        const char *response = "HTTP/1.1 404 Not Found\r\nContent-Length: 26\r\n\r\n<h1>404 Not Found</h1>";
        send(client_socket, response, strlen(response), 0);
    } else {
        // File found, send its contents as response
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        char *file_contents = (char *)malloc(file_size + 1);
        fread(file_contents, 1, file_size, file);
        fclose(file);

        char response_header[256];
        snprintf(response_header, sizeof(response_header), "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n", file_size);
        send(client_socket, response_header, strlen(response_header), 0);
        send(client_socket, file_contents, file_size, 0);

        free(file_contents);
    }
}

void handle_post_request(int client_socket, char *body) {
    // Here you can process the body of the POST request as needed
    const char *response = "HTTP/1.1 200 OK\r\nContent-Length: 26\r\n\r\n<h1>POST Request Received</h1>";
    send(client_socket, response, strlen(response), 0);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        // Accept incoming connection
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        // Read the HTTP request
        read(client_socket, buffer, BUFFER_SIZE);

        // Check if it's a GET request
        if (strstr(buffer, "GET") != NULL) {
            // Extract the URL from the request
            char *url_start = strstr(buffer, " ");
            if (url_start != NULL) {
                url_start++;
                char *url_end = strstr(url_start, " ");
                if (url_end != NULL) {
                    *url_end = '\0';
                    handle_get_request(client_socket, url_start);
                }
            }
        }
        // Check if it's a POST request
        else if (strstr(buffer, "POST") != NULL) {
            // Find the start of the request body
            char *body_start = strstr(buffer, "\r\n\r\n");
            if (body_start != NULL) {
                // Extract the body from the request
                char *body = body_start + 4; // Skip "\r\n\r\n"
                handle_post_request(client_socket, body);
            }
        }

        // Close the client socket
        close(client_socket);
    }

    return 0;
}
