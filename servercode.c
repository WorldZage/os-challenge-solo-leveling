
#include <stdio.h>
#include <stdlib.h>
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

void communicate(int connectionfd) {
    int inSize = PACKET_REQUEST_SIZE;
    int outSize = PACKET_RESPONSE_SIZE;
    char recBuff[inSize];
    char sendBuff[outSize];

    int n;
    // clean the buffer
    bzero(recBuff, inSize);

    // read the message from client and copy it in buffer
    read(connectionfd, recBuff, sizeof(recBuff));
    // print buffer which contains the client contents
    int i;
    uint8_t *arr = (uint8_t *)recBuff;
    // printf("From client: %d",(arr[1]));

    // printing the received hash:
    for (int i = 31; i >= 0; i--) {
        printf("hashnumber: number %d is %d\t",i,arr[i]);
    }
    // calculating the hash as a uint256
    printf("\n\n");
    //


    // print the start value as bytes:
    /*
    for (int i = 32; i <= 39; i++) {
        printf("start value is: number %d is %d\t",i,arr[i]);
    }
    */
    // calculating the start value:
    uint64_t i64 = 0; // = arr[39] | (arr[38] << 8) | (arr[37] << 16) | (arr[36] << 24) | (arr[35] << 32); // | (arr[34] << 40) | (arr[33] << 48) | (arr[32] << 56);
    for (int i = 0; i < 8; i++) {
        i64 = i64 | (arr[39-i] << i*8);
    }
    printf("start is: %ld\t",i64);

    // calculating end value:
    uint64_t end = 0; // = arr[39] | (arr[38] << 8) | (arr[37] << 16) | (arr[36] << 24) | (arr[35] << 32); // | (arr[34] << 40) | (arr[33] << 48) | (arr[32] << 56);
    for (int i = 0; i < 8; i++) {
        end = end | (arr[47-i] << i*8);
    }
    printf("end is: %ld\t",end);

    /* n = 0;
    for (n = 0; n < inSize; n++ ) {
        // copy server message in the buffer
        //recBuff[n] = getchar();
        printf("value is : %d\t",recBuff[n]);
    }*/
    //printf("is : %s",recBuff);
    bzero(sendBuff, outSize);
    // and send that buffer to client
    write(connectionfd, sendBuff, outSize);

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
    int len, nConn, connfd, socketfd;

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
    setsockopt(socketfd,SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
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
    printf("Waiting..");
    connfd = accept(socketfd, (SA*)&cli, &len);
    printf("ACCEPTED");
    if (connfd < 0) {
        printf("server acccept failed...\n");
        exit(0);
    }
    else
        printf("server acccept the client...\n");
    communicate(connfd);
    connfd = accept(socketfd, (SA*)&cli, &len);
    communicate(connfd);
    close(connfd);



}
