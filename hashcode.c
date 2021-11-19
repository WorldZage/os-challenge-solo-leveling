#include <stdio.h>
#include <stdbool.h>

#include <string.h>
// POSIX OS API:
#include <unistd.h>
// header for packet formats:
#include "messages.h"


bool compareHash(unsigned char *truth,unsigned char *test) {
    // this function compares a hash value "truth" (type: character array) with a hash value "test", to find if they are equal.
    // will exit upon first inequality.
    const int size = 32;
    int res = memcmp(truth, test, (size_t)size);
    if(res != 0) {
        return false;
    }
    else {
        return true;
    }
}

uint64_t crackHash(unsigned char *truth,uint64_t start, uint64_t end) {
    // this function will iterate through all possible hash results to find the key corresponding "truth" (type: character array).
    size_t len = 8;
    uint8_t testdata[len];
    unsigned char testmessage[32];
    for(uint64_t i = start; i < end; i++) {
        i = htole64(i); // ensure the same endianness for all machines.
        SHA256((unsigned char*)&i, len, (unsigned char*)testmessage);
        if(compareHash(truth,testmessage)){
            return i;
        }
        bzero(testdata,8); // deletes buffer
        bzero(testmessage,32); // deletes buffer
    }
   printf("FAILURE to find key,start:%ld,end:%ld\n",start,end);
   return 0;
}

