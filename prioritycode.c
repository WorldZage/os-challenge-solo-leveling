
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef structs
#define structs
#include <auxstructs.h>
#endif // structs


pthread_mutex_t node_lock;
Node *head;



void create_access_node() {
    head = malloc(sizeof(Node));
    head->next = NULL;
    head->info.start = -1;

    pthread_mutexattr_t mutex_attr;

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_ERRORCHECK);

    pthread_mutex_init(&node_lock,&mutex_attr);
    //return head;
}


Node* create_node(Request request) {
    Node *new_node = malloc(sizeof(Node));
    new_node->info = request;
    new_node->next = NULL;
    return new_node;
}

void insert_node(Node* target, Node* insert) {
    Node *next_node = target->next;
    insert->next = next_node;
    target->next = insert;
}

// Use a linked list to manage requests.
void sortinsert(Node* new_node) { // return head node;
    pthread_mutex_lock(&node_lock);
    Node *current_node = head->next; // our head node is a placeholder, to server as access point for the linked list.
    if(!current_node) { // check if the head node's "next" is NULL
        insert_node(head,new_node);
        pthread_mutex_unlock(&node_lock);
        return;
    }
    while(current_node->next) { // while next node is not NULL
        if (current_node->info.priority <= new_node->info.priority) {
            insert_node(current_node, new_node);
            pthread_mutex_unlock(&node_lock);
            return;
            }
        current_node = current_node->next;
    }
    // if we got to the last node in the linked list:
    insert_node(current_node,new_node);
    pthread_mutex_unlock(&node_lock);
    return;
}

int count_nodes() {
    int counter = 0;
    pthread_mutex_lock(&node_lock);
    Node *curr_node = head -> next;
    while(curr_node){
        counter++;
        curr_node = head -> next;
    }
    pthread_mutex_unlock(&node_lock);
    return counter;
}


Request get_request() {
    pthread_mutex_lock(&node_lock);
    Node *next_node = head->next;
    if(!next_node) { // check if the head node's "next" is NULL, meaning there's no real requests yet.
        //printf("head had no next node\n");
        Request empty;
        empty.priority = -1; // we use this as a flag to wait until new requests arrive.
        pthread_mutex_unlock(&node_lock);
        return empty;
    }
    Request info = next_node->info;
    head->next = next_node->next;
    free(next_node);
    //printf("nextP:%p\n",head->next);
    //printf("get_r start:%ld\tend:%ld\n",info.start,info.end);
    pthread_mutex_unlock(&node_lock);
    return info;
}


