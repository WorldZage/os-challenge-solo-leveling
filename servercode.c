// C POSIX library includes : https://en.wikipedia.org/wiki/C_POSIX_library

// Standard libraries:
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
// #include <sys/types.h>
#include <string.h>
#include <strings.h>
// not portable, should be removed for final submission.

#include <sys/syscall.h>
// POSIX OS API:
#include <unistd.h>
// header for packet formats:
#include "messages.h"

// Network libraries
#include <arpa/inet.h> // for string IP to int: inet_addr("127.0.0.1")
#include <sys/socket.h>
#include <netinet/in.h>

// my definitions:
#define TCP 6 // comes from sys/socket.h:
#define SA struct sockaddr
#include <hashcode.h>

#ifndef structs
#define structs
#include <auxstructs.h>
#endif // structs

#include <prioritycode.h>
#include <hashmapcode.h>

//threading definitions & inclusions:
#define NUM_THREADS 3 // max number of threads
#include <assert.h>
#include <pthread.h>



Request read_request(int connectionfd) {
    const unsigned int inSize = PACKET_REQUEST_SIZE;
    const size_t hashSize = SHA256_DIGEST_LENGTH;
    unsigned char recBuff[inSize];
    unsigned char hash[hashSize];
    read(connectionfd, recBuff, sizeof(recBuff));

    uint64_t i;
    // calculating the start and end values:
    uint64_t start = 0;
    uint64_t end = 0;
    for (i = 0; i < 8; i++) {
        //printf("ind %d: shift %d. ",i,arr[39-i]);
        start = start | ((uint64_t)recBuff[39-i] << i*8); // casting is important, or else the bitwise shifts would cast to uint32_t ( maybe?)
        // source: https://stackoverflow.com/a/25669375
        end = end | ((uint64_t)recBuff[47-i] << i*8);
    }
    start = le64toh(start);
    end = le64toh(end);
    int priority = recBuff[48];
    memcpy(hash,recBuff,hashSize);
    // create the request obj
    Request request;
    request.connfd = connectionfd;
    memcpy(request.hash,hash,hashSize);
    request.start = start;
    request.end = end;
    request.priority = priority;
    request.key = -1;
    return request;
}


void send_key(Request request) {
    const unsigned int inSize = PACKET_REQUEST_SIZE;
    unsigned char recBuff[inSize];

    const unsigned int outSize = PACKET_RESPONSE_SIZE;
    uint8_t sendBuff[outSize];



    //uint64_t key = htobe64(crackHash(arr,start,end)); // have to send the data back as big endian
    uint64_t key = request.key;
    const int connectionfd = request.connfd;
    // copy the key ínto the buffer for sending.
    memcpy(sendBuff,&key,(size_t)outSize);
    // send that buffer to client
    write(connectionfd, sendBuff, outSize);

    // Source: https://stackoverflow.com/questions/48583574/proper-closure-of-a-socket-other-side-gets-stuck-when-reading
    // and https://man7.org/linux/man-pages/man2/shutdown.2.html for POSIX guidance.


    shutdown(connectionfd, SHUT_WR);         // send an EOF condition
    while (read(connectionfd, recBuff, sizeof(recBuff) > 0));  // wait for the peer to close its side
    close(connectionfd);          // and actually close
    //bzero(sendBuff, outSize); // wipe it (not necessary)
}



void *cracker_thread(void *arguments) {
    // not portable, should be removed for final submission.
    int tid = (int)syscall(SYS_gettid);


    printf("Thread %d created.\n",tid);
    Request request;
    int sleepCounter = 0;
    const int sleepMax = 10;
    int taskCounter = 0;


    while(sleepCounter < sleepMax) {
        request = get_request();
        if(request.priority == -1) {
            //printf("thread sleeping\n");
            //sleepCounter++; // uncomment if threads should exit after certain time has passed without new requests.
            sleep(2);
        }
        else {
            sleepCounter = 0;
            taskCounter++;
            //printf("start:%ld\tend:%ld\n",request.start,request.end);

            // hashmap_key = get(request.hash);
            // if (hashmap_key){ // not NULL
            // request.key = hashmap_key;
            // }

            // else {
            // attempt to find the key in the hashmap
            uint64_t hashmap_key = get(request.hash);
            if (hashmap_key != 0) {
                request.key = hashmap_key;
            }
            else {
                request.key = htobe64(crackHash(request.hash,request.start,request.end));
                put(request.hash, request.key);
            }
            //printf("thrd %d, taskn:%d\n",tid,taskCounter);
            send_key(request);
        }
    }
    printf("Thread %d ending\n",tid);
    printf("%d nodes remain\n",count_nodes());
    return NULL;
}



int main(int argc, char *argcv[]) {
    int port;
    if (argc < 2) {
        printf("Port parameter expected\n");
        exit(0);
    }
    else if (argc > 2) {
        printf("Too many arguments.\n");
        exit(0);
    }
    else {
        port = atoi(argcv[1]);
        printf("port is %d\n",port);
    }
    // RULES ARE:
    // each request is communicated over a TCP connection.
    // upon establishing a TCP connection, the client sends a request packet that is exactly 49 bytes.
    // The server responds to the client with a response packet that is exactly 8 bytes.
    // The TCP connection is then terminated and the TCP socket is closed.
    // Each TCP connection handles a single reverse hashing request.
    // Multiple TCP connection requests may arrive at the same time.

    // INCOMING PACKETS:
    // The request packet has 4 fields
    // The first 32 bytes correspond to the hash that needs to be reversed (the type is an uint8_t array).
    // The following 8 bytes correspond to the start (the type is uint64_t).
    // The following 8 bytes correspond to the end(the type is uint64_t).
    // Finally, the last byte, p, correspond to the priority level of the requests (the type is uint8_t).

    // OUTGOING PACKETS:
    //  response packet has 1 field
    //  The 8-byte field corresponds to the answer, that is the number that generated the hash in the request (the type is uint64_t)

    // PLAN IS:
    // using create(), Create TCP socket.
    // create server_address variable with specified port, normal IP etc.
    // using bind(), Bind the socket to server address.
    // using listen(), put the server socket in a passive mode, where it waits for the client to approach the server to make a connection
    // using accept(), At this point, connection is established between client and server, and they are ready to transfer data
    // when accepted, read the 49-byte request-packet from client
    // answer back with 8-byte response-packet to client
    // close connection and socket , then repeat.

    // initialize variables:
    unsigned int len, nConn;
    int connfd, socketfd;

    // the "fd" suffix for variable names means its a 'file' descriptor
    // create socket
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // create socket_address variable for our server and client
    struct sockaddr_in servaddr, cli;
    bzero(&servaddr, sizeof(servaddr));
    bzero(&cli, sizeof(cli));
    // assign IP, PORT
    servaddr.sin_port = htons(port); // htons = host_to_network_short, PORT =
    servaddr.sin_family = AF_INET; // AF_INET = 2
    // assign flexible IP assignment:
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket
    setsockopt(socketfd,SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); // set socket option to enable reuse of the port.
    // means we can avoid problems of the port still being "used" when we quickly run the program many times, or errors happen while running.
    if (bind(socketfd, (SA*)&servaddr, sizeof(struct sockaddr)) != 0) {
        perror("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // will now listen for a connection, and see if it fails or not
    // number of allowed connections
    // A smaller number of allowed connections seems to work better. 2 instead of 35 resulted in a much later "Connection Timeout" error.
    nConn = 1000;
    if ((listen(socketfd, nConn)) != 0) {
        perror("Listen failed...\n");
        exit(0);
    }
    else {
        printf("Waiting..");
        printf("Server listening..\n");
    }
    len = sizeof(servaddr);

    // Accept the data packet from client and verification
    printf("Waiting..\n");
    connfd = accept(socketfd, (SA*)&cli, &len);
    printf("ACCEPTED\n");

    // current connection is stored at index 0, current thread is stored at index 1, connfd is stored at index 2:
    //int connThread[2] = {0,connfd};
    //int curr_thread = 0;
    int curr_conn = 0; // current connection


    // create the node used as access point for the linked list:
    create_access_node();
    // declare variables:
    Request new_request;
    Node *new_node;

    // create the hashmap:
    create_hashmap();

    // threading:
    pthread_t threads[NUM_THREADS];
    // bool locks[NUM_THREADS] = { false };
    for(int index=0;index < NUM_THREADS; index++) {
        pthread_create(&threads[index], NULL, cracker_thread,NULL);
    }
    while (connfd) {
        if (connfd < 0) {
            printf("server acccept failed...\n");
            break;
        }
        else {
            //printf("nReqs:%d\n",++curr_conn);
            new_request = read_request(connfd);
            //printf("new request: start:%ld\tend:%ld\n",new_request.start,new_request.end);

            new_node = create_node(new_request);
            //printf("new node's start:%ld\tend:%ld\n",new_node->info.start,new_node->info.end);
            sortinsert(new_node);

            //printf("connfd is: %d", connfd);
            //printf("server acccept the client...\n");

            /*connThread[1] = connThread[0] % NUM_THREADS; //curr_thread = curr_conn % NUM_THREADS;
            if (connThread[0] > connThread[1]) { // ONLY if we have already looped through the thread list once, wait for a thread to open up
                result_code = pthread_join(threads[connThread[1]], NULL);
                assert(!result_code);
            }
            pthread_create(&threads[connThread[1]], NULL, communication_thread,connThread);//communicate(connfd);
            *&
            //printf("conn: %d. ",++curr_conn);
            sleep(1);
            ++connThread[0]; // = connThread[0] + 1;
            connThread[2] = accept(socketfd, (SA*)&cli, &len); //connfd = accept(socketfd, (SA*)&cli, &len);
            */
            ++curr_conn;
            if (curr_conn % NUM_THREADS == 0) {
              usleep(15 * 100000); // 1.5 seconds
            }


            /*find_usable_thread(threads, locks, NUM_THREADS, connfd, curr_conn);
            */
            connfd = accept(socketfd, (SA*)&cli, &len); //connfd = accept(socketfd, (SA*)&cli, &len);

        }

    }
    printf("server DONE");



}
