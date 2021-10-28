// C POSIX library includes : https://en.wikipedia.org/wiki/C_POSIX_library

// Standard libraries:
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>
#include <sys/types.h>
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

//threading definitions & inclusions:
#define NUM_THREADS 5   // max number of threads
#include <assert.h>
#include <pthread.h>




uint64_t communicate(int connectionfd) {
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
    memcpy(sendBuff,&key,(size_t)outSize);
    // send that buffer to client
    write(connectionfd, sendBuff, outSize);

    // Source: https://stackoverflow.com/questions/48583574/proper-closure-of-a-socket-other-side-gets-stuck-when-reading
    // and https://man7.org/linux/man-pages/man2/shutdown.2.html for POSIX guidance.
    shutdown(connectionfd, SHUT_WR);         // send an EOF condition
    while (read(connectionfd, recBuff, sizeof(recBuff) > 0));  // wait for the peer to close its side
    close(connectionfd);          // and actually close
    //bzero(sendBuff, outSize); // wipe it (not necessary)
    return key;
}

// inspiration : https://en.wikipedia.org/wiki/Pthreads
void *communication_thread(void *arguments) {
    int conn_number = *((int *)arguments);
    int thread_number = *((int *)arguments + 1);
    int connfd = *((int *)arguments + 2);
    //printf("connID:%d",connfd);
    printf("Thread: %d created.\n",thread_number);
    uint64_t key = communicate(connfd);
    printf("Thread:%d completed. connN:%d, connID: %d, key: %ld\n",thread_number,conn_number,connfd,be64toh(key));
    return NULL;
}

void find_usable_thread(pthread_t threads[], int num_threads) {
    //
    // wait until a thread is available,
    // then answer the highest priority request.

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
    bzero(&cli, sizeof(cli));
    // assign IP, PORT
    servaddr.sin_port = htons(port); // htons = host_to_network_short, PORT =
    servaddr.sin_family = AF_INET; // AF_INET = 2
    //servaddr.sin_addr.s_addr = inet_aton("192.168.101.10"); //htonl(INADDR_ANY); // INADDR_ANY = 0x000000
    // old IP assignment:
    //inet_aton("192.168.101.10", (struct in_addr*)&servaddr.sin_addr.s_addr);
    // new, 'flexible' IP assignment:
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // bind socket
    // struct sadd = struct sockaddr;
    //printf("ip is: %c", (servaddr.sin_addr.s_addr));
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
    nConn = 100;//35;
    if ((listen(socketfd, nConn)) != 0) {
        printf("Listen failed...\n");
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
    //int curr_conn = 0; // current connection
    //int curr_thread = 0;
    int connThread[3] = {0,0,connfd};
    int result_code = 0;
    // threading:
    pthread_t threads[NUM_THREADS];
    //pthread_mutex_t locks[NUM_THREADS];

    while (connfd) {
        if (connfd < 0) {
            printf("server acccept failed...\n");
            break;
        }
        else {
            //printf("connfd is: %d", connfd);
            //printf("server acccept the client...\n");
            connThread[1] = connThread[0] % NUM_THREADS; //curr_thread = curr_conn % NUM_THREADS;
            if (connThread[0] > connThread[1]) { //(curr_conn > curr_thread) {
                result_code = pthread_join(threads[connThread[1]], NULL);
                assert(!result_code);
            }
            pthread_create(&threads[connThread[1]], NULL, communication_thread,connThread);//communicate(connfd);
            //printf("conn: %d. ",++curr_conn);
            sleep(1);
            ++connThread[0]; // = connThread[0] + 1;
            connThread[2] = accept(socketfd, (SA*)&cli, &len); //connfd = accept(socketfd, (SA*)&cli, &len);


        }

    }
    close(connfd);



}
