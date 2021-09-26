#include <stdio.h>
#include <openssl/sha.h>
#include "messages.h"
int main(int argc, char *argcv[]) {
    unsigned char message[32];
    //SHA256_CTX c;
    size_t len = 4;
    char data[] = "5";
    //int SHA256_Init(SHA256_CTX *c);
    //int SHA256_Update(SHA256_CTX *c, const void *data, size_t len);
    //int SHA256_Final(unsigned char *message, SHA256_CTX *c);
    SHA256(data, len, (unsigned char*)message);
    for (int i = 0; i < 32; i++) {
        printf("hashed: %ld\t",htole64(message[i]));
    }

}
