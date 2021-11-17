#include <stdbool.h>
#include <stdint.h>


typedef struct {
    int conn_number;
    int thread_number;
    int connfd;
    bool *locks;
} t_params;


typedef struct {
    int connfd;
    uint8_t hash[32];
    uint64_t start;
    uint64_t end;
    int priority;
    uint64_t key;
} Request;

typedef struct Node Node;

struct Node {
    Request info;
    Node* next;
};


