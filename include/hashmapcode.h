#include <stdint.h>

void create_hashmap();
int lin_rehash(int oldkey);
int quad_rehash(int oldkey, int attempt_count);
int folding_hash(uint8_t *input_key);
void _put_help(int index, uint64_t value, uint8_t *key);
void put(uint8_t *input_key, uint64_t value);
uint64_t _get_help(int index, bool found_key);
uint64_t get(uint8_t *input_key);
