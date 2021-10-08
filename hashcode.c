#include <stdio.h>
#include <stdbool.h>

#include <strings.h>
// POSIX OS API:
#include <unistd.h>
// header for packet formats:
#include "messages.h"


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
    unsigned char testmessage[32];
    for(uint64_t i = start; i < end; i++) {
        i = htole64(i); // ensure the same endianness for all machines.
        //memcpy(testdata,&i,len); // copy the i into a uint8_t array (which is an alias for a char array)
        SHA256((unsigned char*)&i, len, (unsigned char*)testmessage);//SHA256(testdata, len, (unsigned char*)testmessage);
        //printf("i:%ld,1st: %d,2nd: %d\t",i,(unsigned char)testmessage[0],(unsigned char)testmessage[1]);
        if(compareHash(truth,testmessage)){
            //printf("RESULT IS : %ld\n",i);
            return i;
        }
        bzero(testdata,8); // deletes
        bzero(testmessage,32); // deletes
    }
   printf("FAILURE to find key\n");
   return 0;
}

