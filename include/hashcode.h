#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

bool compareHash(unsigned char *truth, unsigned char *test);
uint64_t crackHash(unsigned char *truth,uint64_t start, uint64_t end);
