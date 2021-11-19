
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "messages.h"
#include <time.h> // used to benchmark
#include <openssl/sha.h>
#include <openssl/evp.h>

int main(int argc, char* argcv[]) {

    // generate 1.000.000 random numbers
    // random numbers are used to ensure we don't have much human bias involved in choosing the numbers.
    // we might still get blindsided by bias in the implementation of the generator, though.
    // for example, the randomly generated numbers seem to have a range of (0...pow(2,32)).
    int random_length = 1000000;
    uint64_t randNums[random_length];
    // set the seed for generation of random numbers to the same for every iteration - ensures we get same result every time we run the code.
    int seed = 2;
    srandom(seed);
    int i;
    for (i = 0; i < random_length; i++) {
        randNums[i] = htole64(random()); // convert the number to little-endian, to match our server's use-case.
    }

    // The variables needed to be initialized for either function:
    // A message buffer and message length
    const int msglength = 32;
    unsigned char message[msglength];

    // number of bytes in the input (8 times 8 bytes = 64 bits).
    const size_t len = 8;
    // array for converting a uint64_t to an array of 8 bytes.
    uint8_t testdata[len];
    // ------------------------------------------------
    // 1: time the one-function SHA256() method:
    float startTime = (float)clock()/CLOCKS_PER_SEC;
    for (i = 0; i < random_length; i++) {
        memcpy(testdata,&randNums[i],len);
        SHA256(testdata, len, message);
        //bzero(testdata,len); // fills the buffers with 0s
        //bzero(message,msglength);
    }
    float endTime = (float)clock()/CLOCKS_PER_SEC;
    printf("METHOD 1: TIME = %f\n",endTime - startTime);
    // ------------------------------------------------
    // 2: time the EVP digest routine method:
    // create the context:
    EVP_MD_CTX *hashctx = EVP_MD_CTX_create();
    const EVP_MD *hashptr = EVP_get_digestbyname("SHA256");
    unsigned int sz = 32;

    // hash all the numbers:
    startTime = (float)clock()/CLOCKS_PER_SEC;
    for (i = 0; i < random_length; i++) {
        //memcpy(testdata,&i,len);
        EVP_DigestInit_ex(hashctx, hashptr, NULL);
        EVP_DigestUpdate(hashctx, &randNums[i], len);
        EVP_DigestFinal_ex(hashctx, message, &sz);
        //bzero(testdata,len); // fills the buffers with 0s
        //bzero(message,msglength);
    }
    endTime = (float)clock()/CLOCKS_PER_SEC;
    printf("METHOD 2: TIME = %f\n",endTime - startTime);

    EVP_MD_CTX_free(hashctx); // clean up the context afterwards
    // ------------------------------------------------
    // 3: one-line EVP digest method:
    startTime = (float)clock()/CLOCKS_PER_SEC;
    for (i = 0; i < random_length; i++) {
        //memcpy(testdata,&i,len);
        EVP_Digest (&randNums[i], len, message, NULL, EVP_sha256(), NULL);
    }
    endTime = (float)clock()/CLOCKS_PER_SEC;
    printf("METHOD 3: TIME = %f\n",endTime - startTime);
    // ------------------------------------------------
    // 4: One line SHA256, without memcpy.
    startTime = (float)clock()/CLOCKS_PER_SEC;
    for (i = 0; i < random_length; i++) {
        SHA256((unsigned char*)&randNums[i], len, message);
        //bzero(testdata,len); // fills the buffers with 0s
        //bzero(message,msglength);
    }
    endTime = (float)clock()/CLOCKS_PER_SEC;
    printf("METHOD 4: TIME = %f\n",endTime - startTime);



}

