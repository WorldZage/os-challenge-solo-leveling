
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
// POSIX OS API:
#include <unistd.h>
// header for packet formats:
#include "messages.h"

// for string IP to int: inet_addr("127.0.0.1")
#include <arpa/inet.h>


// my definitions:
// comes from sys/socket.h:
#define TCP 6;
#define SA struct sockaddr


//threading definitions & inclusions:
#define NUM_THREADS 1 // max number of threads
#include <assert.h>
#include <pthread.h>


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
        memcpy(testdata,&i,len); // copy the i into a uint8_t array (which is an alias for a char array)
        SHA256(testdata, len, (unsigned char*)testmessage);
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


void communicate(int connectionfd) {
    const unsigned int inSize = PACKET_REQUEST_SIZE;
    const unsigned int outSize = PACKET_RESPONSE_SIZE;
    unsigned char recBuff[inSize];
    uint8_t sendBuff[outSize];
    bzero(sendBuff, outSize);

    // clean the buffer
    bzero(recBuff, inSize);

    // read the message from client and copy it in buffer
    read(connectionfd, recBuff, sizeof(recBuff));
    // print buffer which contains the client contents
    unsigned char *arr = recBuff;
    // printf("From client: %d",(arr[1]));


    // print start/end part of received message:
    /*for (i = 32; i < inSize; i++) {
        printf("arr[%d] is: %d.  ",i, arr[i]);
    }*/

    // printing the received hash:
    /*for (int i = 31; i >= 0; i--) {
        printf("hash: arr[%d] is %d\t",i,arr[i]);
    }*/

    uint64_t i;
    // calculating the start and end values:
    uint64_t start = 0;
    uint64_t end = 0;
    for (i = 0; i < 8; i++) {
        //printf("ind %d: shift %d. ",i,arr[39-i]);
        start = start | ((uint64_t)arr[39-i] << i*8); // casting is important, or else the bitwise shifts would cast to uint32_t ( maybe?)
        // source: https://stackoverflow.com/a/25669375
        end = end | ((uint64_t)arr[47-i] << i*8);
    }
    start = le64toh(start);
    end = le64toh(end);
    //printf("start is: %ld... end is: %ld.\t",start,end);


    uint64_t key = htobe64(crackHash(arr,start,end)); // have to send the data back as big endian
    // copy the key ínto the buffer for sending.
    printf("key is: %ld\n",be64toh(key));
    memcpy(sendBuff,&key,(size_t)outSize);
    // send that buffer to client
    write(connectionfd, sendBuff, outSize);
    // wipe it (not necessary)
    //bzero(sendBuff, outSize);

}

// inspiration : https://en.wikipedia.org/wiki/Pthreads
void *communication_thread(void *arguments) {
    int connfd = *((int *)arguments);
    printf("connID:%d",connfd);
    communicate(connfd);
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

    // the -fd suffix for variable names means its a descriptor
    // create socket
    socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // create socket_address variable for our server and client
    struct sockaddr_in servaddr, cli;
    bzero(&servaddr, sizeof(servaddr));
    // assign IP, PORT
    servaddr.sin_port = htons(port); // htons = host_to_network_short, PORT =
    servaddr.sin_family = AF_INET; // AF_INET = 2
    //servaddr.sin_addr.s_addr = inet_aton("192.168.101.10"); //htonl(INADDR_ANY); // INADDR_ANY = 0x000000
    inet_aton("192.168.101.10", (struct in_addr*)&servaddr.sin_addr.s_addr);
    // bind socket
    // struct sadd = struct sockaddr;
    printf("ip is: %d", (servaddr.sin_addr.s_addr));
    setsockopt(socketfd,SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)); // set socket option to enable reuse of the port.
    // means we can avoid problems of the port still being "used" when we quickly run the program many times, or errors happen while running.
    if (bind(socketfd, (SA*)&servaddr, sizeof(struct sockaddr)) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // will now listen for a connection, and see if it fails or not
    // number of allowed connections
    nConn = 15;
    if ((listen(socketfd, nConn)) != 0) {
        printf("Listen failed...\n");
        exit(0);
    }
    else {
        printf("Waiting..");
        printf("Server listening..\n");
    }
    len = sizeof(cli);

    // Accept the data packet from client and verification
    printf("Waiting..\n");
    connfd = accept(socketfd, (SA*)&cli, &len);
    printf("ACCEPTED\n");

    int curr_conn = 0; // current connection
    int curr_thread = 0;
    int result_code = 0;
    // threading:
    pthread_t threads[NUM_THREADS];

    while (connfd) {
        if (connfd < 0) {
            printf("server acccept failed...\n");
            break;
        }
        else {
            //printf("connfd is: %d", connfd);
            //printf("server acccept the client...\n");
            curr_thread = curr_conn % NUM_THREADS;
            if (curr_conn > curr_thread) {
                result_code = pthread_join(threads[curr_thread], NULL);
                assert(!result_code);
            }
            pthread_create(&threads[curr_thread], NULL, communication_thread,&connfd);//communicate(connfd);
            printf("conn: %d. ",++curr_conn);
            connfd = accept(socketfd, (SA*)&cli, &len);


        }

    }
    close(connfd);



}
