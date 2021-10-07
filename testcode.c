#include <stdio.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <stdlib.h>
#include "messages.h"
typedef unsigned char byte;

bool compareHash(unsigned char *truth,unsigned char *test) {
    // this function compares a hash value "truth" (type: character array) with a hash value "test", to find if they are equal.
    // will short-circuit upon inequality.
    int size = 32;
    for(int i = 0; i < size; i++) {
        if(truth[i] != test[i]) {
            return false;
        }
    }
    return true;
}

uint64_t crackHash(unsigned char *truth,uint64_t start, uint64_t end) {
    // this function will iterate through all possible hash results to find the key corresponding "truth" (type: character array).
    size_t len = 8;
    uint8_t testdata[len];
    char testmessage[32];
    bool success = false;
    for(uint64_t i = start; i < end; i++) {
        memcpy(testdata,&i,len);
        SHA256(testdata, len, (unsigned char*)testmessage);
        //printf("i:%ld,1st: %d,2nd: %d\t",i,(unsigned char)testmessage[0],(unsigned char)testmessage[1]);
        if(compareHash(truth,testmessage)){
            printf("RESULT IS : %ld\n",i);
            success = true;
            break;
        }
        bzero(testdata,8); // deletes
        bzero(testmessage,32); // deletes
    }
    if(!success) {
       printf("FAILURE to find key\n");
        }
}

int main(int argc, char *argcv[]) {
    const int msglength = 32;
    unsigned char message[msglength];
    //SHA256_CTX c;
    const size_t len = 8;
    uint64_t inp = htole64(pow(2,48)-5);//htole64(5); // htole64 isn't strictly necessary as the VM runs in LE.
    uint8_t data[8];
    memcpy(data,&inp,len);
    // prints the little endian of our input
    for (int i = 0; i < 8; i++) {
        printf("res:%x .",data[i]);
    }
    // OUTPUTS: 4a c6 a ... 4d b7 8e
    /*
    SHA256(data, len, (unsigned char*)message);
    for (int i = 0; i < 32; i++) {
        printf("hashed: %d\t",(unsigned char)message[i]);
    }
    */
    //crackHash(message,1,6);
    /*const int DataLen = 30;
    SHA_CTX shactx;
    byte digest[SHA_DIGEST_LENGTH];
    int testvalue = 5;

    int i;
    byte* testdata = (byte *)malloc(DataLen);
    //for (i=0; i<DataLen; i++) testdata[i] = 0;
    testdata[0] = testvalue;

    SHA256_Init(&shactx);
    SHA256_Update(&shactx, testdata, DataLen);
    SHA256_Final(digest, &shactx);

    for (i=0; i<SHA_DIGEST_LENGTH; i++)
	printf("%d", digest[i]);
    putchar('\n');

    return 0;*/
    //EVP_MD_CTX *hashctx;
    EVP_MD_CTX *hashctx = EVP_MD_CTX_create();
    const EVP_MD *hashptr = EVP_get_digestbyname("SHA256");
    //EVP_MD *hashptr = EVP_get_digestbyname("SHA1");

    //EVP_MD_CTX_init(hashctx);
    // first hashed message
    EVP_DigestInit_ex(hashctx, hashptr, NULL);
    EVP_DigestUpdate(hashctx, data, len);


    unsigned int sz = EVP_MD_CTX_size(hashctx);
    //printf("size is:%d\n",sz);
    EVP_DigestFinal_ex(hashctx, message, &sz);
    printf("First message hash:\n");
    int i;
    for (i = 0; i < msglength; ++i) {
        printf("%x ", message[i]);
    }

    //printf("tst:%ld\n",tst);
    // Hashing again, with same value:
    uint64_t inp2 = htole64(pow(2,48)-5);
    memcpy(data,&inp2,len);
    // reseting the hash context after using DigestFinal
    EVP_DigestInit_ex(hashctx, hashptr, NULL);
    bzero(message,msglength);
    // hashing the data
    EVP_DigestUpdate(hashctx, data, len);
    EVP_DigestFinal_ex(hashctx, message, &sz);
    printf("\nSecond message hash:\n");
    for (i = 0; i < msglength; ++i) {
        printf("%x ", message[i]);
    }
    // OUTPUTS: 4a c6 a ... 4d b7 8e
    // same result as first hashing!

    EVP_MD_CTX_free(hashctx);


    return 0;



    /*uint8_t testdata[8];
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

    }*7


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

}


