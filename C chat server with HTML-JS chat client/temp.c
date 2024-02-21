#include <stdio.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/evp.h>

void sha1_hash_string(const char *str, unsigned char *outputBuffer) {
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    int i;
    unsigned int outputLength;

    // Create a new message digest context
    mdctx = EVP_MD_CTX_new();
    if (mdctx == NULL) {
        printf("Error creating EVP_MD_CTX\n");
        exit(EXIT_FAILURE);
    }

    // Initialize the context for SHA-1
    md = EVP_sha1();
    EVP_DigestInit_ex(mdctx, md, NULL);

    // Update the digest context with the string data
    EVP_DigestUpdate(mdctx, str, strlen(str));

    // Finalize the digest to obtain the hash value
    EVP_DigestFinal_ex(mdctx, outputBuffer, &outputLength);

    // Clean up
    EVP_MD_CTX_free(mdctx);
}

int main()
{
    char buffer[] = "GET /chat HTTP/1.1\r\nHost: example.com:8000\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13";
    char *key = strstr(buffer, "Sec-WebSocket-Key: ");
    sscanf(key, "Sec-WebSocket-Key: %s", key);
    strcat(key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

    unsigned char hashValue[EVP_MAX_MD_SIZE]; // Allocate enough space for the hash
    int i;

    // Perform SHA-1 hashing on the input string
    sha1_hash_string(key, hashValue);

    // Print the hash value
    printf("SHA-1 hash of \"%s\":\n", key);
    for (i = 0; i < EVP_MD_size(EVP_sha1()); i++) {
        printf("%02x", hashValue[i]);
    }
    printf("\n");

    return 0;
}
