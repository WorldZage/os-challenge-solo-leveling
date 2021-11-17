#ifndef structs
#define structs
#include <auxstructs.h>
#endif // structs




Node* create_access_node();

Node* create_node(Request request);

void insert_node(Node* insert);

void sortinsert(Node* new_node);

Request get_request();

int count_nodes();
