// From servercode.c:

// inspiration : https://en.wikipedia.org/wiki/Pthreads
void *communication_thread(void *arguments) {
    t_params *pParam = arguments;
    /*parameters.conn_number =
    int conn_number = *((int *)arguments);
    int thread_number = *((int *)arguments + 1);
    int connfd = *((int *)arguments + 2);
    bool *locks = *((bool *)arguments + 3);*/
    //printf("connID:%d",connfd);
    printf("Thread: %d created.\n",(pParam->thread_number));
    uint64_t key = communicate(pParam->connfd);
    printf("Thread:%d completed. connN:%d, connID: %d, key: %ld\n",pParam->thread_number,pParam->conn_number,pParam->connfd,be64toh(key));
    pParam->locks[pParam->thread_number] = false;
    free(pParam);
    pParam = NULL;
    return NULL;
}



uint64_t communicate(int connectionfd) {
    const unsigned int inSize = PACKET_REQUEST_SIZE;
    const unsigned int outSize = PACKET_RESPONSE_SIZE;
    unsigned char recBuff[inSize];
    uint8_t sendBuff[outSize];

    // clean the buffers
    /*bzero(sendBuff, outSize);
    bzero(recBuff, inSize);
    */
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




// replace all four arguments with a custom struct
void find_usable_thread(pthread_t *threads, bool *locks, int num_threads, int connfd, int curr_conn) {
    //
    // wait until a thread is available,
    // then answer the highest priority request.
    bool occupied = true;
    int index = 0;
    while(occupied) {
        if(!locks[index]) {
            if (index > curr_conn) {
                int join_code = pthread_join(threads[index], NULL); // ensure that the thread is finished
                assert(!join_code); // double-ensure the thread is finished without problems
            }
            locks[index] = true; // "lock" the thread
            // create communication_thread parameter:
            t_params *params = malloc(sizeof(t_params));

            params->conn_number = curr_conn;
            params->thread_number = index;
            params->connfd = connfd;
            params->locks = locks;
            //printf("index is:%d, currcon:%d,connfd:%d\n",params->thread_number,params->conn_number,params->connfd);
            pthread_create(&threads[index], NULL, communication_thread,params); // create the thread with the communcation_thread function.
            occupied = false;
        }
        index = (index + 1) % num_threads;
        //sleep(1);
    }

    return;
}
