
// hashmap should be an N array space, which hashes 32 uint8_t (int) to calculate an index.
// Whenever we decrypt a key in the main loop, we add it to the hashmap.
// Then, before we begin decryption of following keys, we check if it has already been cracked
// If so, we don't need to decrypt it again.
// If not, then we decrypt and add the hash-key pair to the hashmap.
// lastly, when we run out of spaces in the array,

// the hash function we use is referred to as the "folding method";
// take pairs of digits of the value to be hashed, and add them together.
// then, modulo by size of the hashmap.

// reference: https://runestone.academy/runestone/books/published/pythonds/SortSearch/Hashing.html

// MAPSIZE should be a prime number.
#define MAPSIZE 97
#define SENTINEL 0
#define LINSTEP 3

#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include "messages.h"
#include <hashcode.h>




// the naming convention used is to consider the hashmap as a generic hashmap structure, not in context of the server;
// thus, we use 'key-value' pairs;
typedef struct Hashmap {
    int size;
    uint8_t keys[MAPSIZE][SHA256_DIGEST_LENGTH]; //; // array of uint8_t arrays, the type we will use as keys.
    uint64_t values[MAPSIZE];
    double load_factor; // used to determine the max number of times we wanted to probe before "settling".
    // for PUT:
    // The more full the table is, the less times we will attempt to look through,
    // as we expect to not find an empty spot easily.
    // for GET:
    // reverse case; higher load_factor means more attempts. This is because we expect a larger probability of the value being present in the hashmap.
} Hashmap;


pthread_mutex_t map_lock;
Hashmap *map;


void create_hashmap() {
    map = malloc(sizeof(Hashmap));
    map->size = MAPSIZE;
    //map->values = malloc(MAPSIZE*sizeof(uint64_t));
    map->load_factor = 0.0d;
    for(int i=0; i<map->size;i++) {
        map->values[i] = SENTINEL;
        //map->keys[i] = (uint8_t*)malloc(2 * sizeof(uint8_t)); // dynamically allocate a pointer for each hash. SHA256_DIGEST_LENGTH
    }
    //map->keys = malloc(sizeof(uint8_t[MAPSIZE][SHA256_DIGEST_LENGTH]))
    // set the attribute of the mutex.
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutex_init(&map_lock,&mutex_attr);
}

// linear probing:
int lin_rehash(int oldkey) {
    return (oldkey + (LINSTEP))%map->size;
}

// quadratic probing:
int quad_rehash(int oldkey, int attempt_count) {
   return (int)(oldkey + pow(attempt_count,2)) % map->size;
}

//
int folding_hash(uint8_t *input_key) {
    uint64_t index = 0;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        index += input_key[i] * i;
    }
    return (int)(index%MAPSIZE);
}

// this is the helper function used by "put".
void _put_help(int index, uint64_t value, uint8_t *key) {
    if(map->values[index] == SENTINEL) { // increase the load factor if it didn't replace a previous value.
        map->load_factor += (1.0d / map->size);
    } // converse case should (almost) never happen.
    map->values[index] = value;
    /*if (map->keys[index]) {
        free((map->keys[index]));
    }*/
    memcpy(map->keys[index],key,SHA256_DIGEST_LENGTH);
    return;
}


// "input_key" is a pointer to a 32 element hash that we receive from the client for us to crack. "value" is the SHA-256 decryption of "input_key".
void put(uint8_t *input_key, uint64_t value) {
    pthread_mutex_lock(&map_lock);
    int index = folding_hash(input_key);
    if (map->values[index] != SENTINEL) { // first attempt wasn't empty, so probe through the hashmap:
        if (!compareHash(input_key, map->keys[index])) { // case of input_key and key stored in hashmap not matching, i.e. slot already occupied
            for(int attempt = 0; attempt < ceil((1.0d - map->load_factor) * map->size)  ; attempt++) { // iterate through values using quadratic probing
                index = lin_rehash(index);//quad_rehash(index,attempt); // calculate the rehashed value
                // the number of attempts we make depends on the value of our load factor
                if(map->values[index] == SENTINEL) break; // found an empty spot
                if(compareHash(input_key, map->keys[index])) break; // value already exists in the hashmap.
            }
        }
    }
    _put_help(index, value,input_key); // insert value at the last visited index.
    pthread_mutex_unlock(&map_lock);
    return;
}


uint64_t _get_help(int index, bool found_key) {
    if (index >= map->size) {
        perror("index outside of hashmap's scope");
        return 0;
    }
    else if (found_key) { // case of having found the correct value.
        uint64_t value = (map->values[index]);
        //printf("Found a key with hashmap\n");
        return value;
    }
    else { // case of empty spot, or the key at index does not match the input_key.
        return 0;
    }
}

// very similar to the put function, except we want to find our desired value.
// we use 0 as our "sentinel value". This can be excused by the fact that, if the key was 0 and our implementation cannot detect 0 as a value, it is okay
// as 0 will be the first element to iterate through when decrypting the hash, afterwards.
uint64_t get(uint8_t *input_key) {
    pthread_mutex_lock(&map_lock);
    int index = folding_hash(input_key);
    bool found_key = false;
    if (map->values[index] != SENTINEL) { // case of a value present at the first index to check, meaning the value could be at another index
        // (since we do not have a 'delete key-value' function).
        if (compareHash(input_key, map->keys[index])) { // ideal case of finding the desired key on first attempt.
            found_key = true;
        }
        else{ // if slot was full, try new slots, using a probing method
            for(int attempt = 0; attempt < (ceil(map->load_factor / 2.0d) * map->size) ; attempt++) { // iterate through (up to half of all) values using quadratic probing
                index = lin_rehash(index);//quad_rehash(index,attempt); // calculate the rehashed value
                if(map->values[index] == SENTINEL) break; // found an empty spot = value doesn't exist in hashmap.
                else if(compareHash(input_key, map->keys[index])) { // we found the key, meaning we can get the value.
                    found_key = true;
                    break;
                }
            }
        }
    }
    pthread_mutex_unlock(&map_lock);
    return _get_help(index, found_key);
}




