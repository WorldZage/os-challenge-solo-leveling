#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <openssl/sha.h>
#include "messages.h"

bool compareHash(unsigned char *truth,unsigned char *test) {
    int size = 32;
    for(int i = 0; i < size; i++) {
        if(truth[i] != test[i]) {
            return false;
        }
    }
    return true;
}

int main(int argc, char *argcv[]) {
    char message[32];
    //SHA256_CTX c;
    size_t len = 8;
    uint64_t inp = htole64(5); // htole64 isn't strictly necessary as the VM runs in LE.
    uint8_t data[8];
    memcpy(data,&inp,8);
    // prints the little endian of our input
    /*for (int i = 0; i < 8; i++) {
        printf("res:%d .",data[i]);
    }*/

    SHA256(data, len, (unsigned char*)message);
    for (int i = 0; i < 32; i++) {
        //printf("hashed: %d\t",(unsigned char)message[i]);
    }


    uint8_t testdata[8];
    char testmessage[32];
    for(uint64_t i = 0; i < 10; i++) {
        memcpy(testdata,&i,8);
        SHA256(testdata, len, (unsigned char*)testmessage);
        //printf("i:%ld,1st: %d,2nd: %d\t",i,(unsigned char)testmessage[0],(unsigned char)testmessage[1]);
        if(compareHash(message,testmessage)){
            printf("RESULT IS : %ld",i);
            break;
        }
        bzero(testdata,8); // deletes
        bzero(testmessage,32); // deletes

    }


    /*
    // test conversion of uint64 to char[]
    uint64_t data = htole64(5);
    unsigned char str[8];
    memcpy(str,&data,8);
    printf("\n");
    for (int i = 0; i < 8; i++) {
        printf("res:%d .",str[i]);
    }
    */
    //int SHA256_Init(SHA256_CTX *c);
    //int SHA256_Update(SHA256_CTX *c, const void *data, size_t len);
    //int SHA256_Final(unsigned char *message, SHA256_CTX *c);
    bool t = true;
}


