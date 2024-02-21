#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>

typedef unsigned char byte;

int main(int argc, char *argv[]) {
    SHA_CTX shactx;
    byte digest[SHA_DIGEST_LENGTH];
    int i;

    byte* testdata = (byte *)malloc(SHA_DIGEST_LENGTH);
    for (i=0; i<SHA_DIGEST_LENGTH; i++) testdata[i] = 0;

    SHA1_Init(&shactx);
    SHA1_Update(&shactx, testdata, SHA_DIGEST_LENGTH);
    SHA1_Final(digest, &shactx);

    for (i=0; i<SHA_DIGEST_LENGTH; i++)
	printf("%02x", digest[i]);
    putchar('\n');

    return 0;
}
