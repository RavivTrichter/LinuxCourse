//
// Created by Raviv Trichter on 12/29/18.
//



#include <sys/stat.h>
#include "p2p.h"


/**  Global Variables  **/
bool isShutDown;
typedef enum {act_seed,act_leech,act_shutDown} Action;

/**********************************************************************************************************/

in_port_t seed(int sockFD, char* argv[], int argc, struct sockaddr_in* myaddr) {
    int                 i;
    msg_notify_t        msg_notify;
    msg_ack_t           msg_ack;

    msg_notify.m_addr = myaddr->sin_addr.s_addr;
    msg_notify.m_port = 0; // initialized to 0
    msg_notify.m_type = MSG_NOTIFY;
    for (i = 0; i < argc; ++i) {
        strcpy(msg_notify.m_name, argv[i]);
        printf("Client - share: sending MSG_NOTIFY for <%s> @ 127.0.0.1:%d\n",msg_notify.m_name,msg_notify.m_port);
        if(send(sockFD,(char*)&msg_notify, sizeof(msg_notify),0) < 0 ) {
            perror("Client - send()");
            exit(EXIT_FAILURE);
        }
        printf("Client - share: recieving MSG_ACK\n");
        if(recv(sockFD,(char*)&msg_ack, sizeof(msg_ack),0) < 0) {
            perror("Cleint - recv()");
            exit(EXIT_FAILURE);
        }
        if(i == 0) {
            msg_notify.m_port = msg_ack.m_port; // updating new port number
            printf("Client - share: set port to : %d\n", msg_notify.m_port);
        }
    }
    return msg_ack.m_port;
}


/* Given a filename returns true if it appears in the array of strings argv */
/**********************************************************************************************************/

bool isRelevantFile(char* checkMe, char* argv[], int argc) {
    int i;

    for (i = 0; i < argc; ++i)
        if( argv[i] && strcmp(checkMe,argv[i]) == 0 )
            return true;
    return false;
}

/**********************************************************************************************************/

void sendFileToPeer(int connectedFD) {
    msg_filereq_t   msg_filereq;
    msg_filesrv_t   msg_filesrv;
    ssize_t         nr = 0;
    int             fileFD, rand_int, num_of_iterations, i, last_size;
    char            buff[P_BUFF_SIZE], *last_buff;
    struct stat     file_stat;


    srand(time(NULL));

    printf("Peer - start_server : starting peer server\n");
    if(recv(connectedFD,(char*)&msg_filereq, sizeof(msg_filereq),0) < 0) { // recieving name of file from peer
        perror("Peer - recv()");
        close(connectedFD);
        exit(EXIT_FAILURE);
    }
    printf("Peer - start_server : received MSG_FILEREQ for file : < %s >\n", msg_filereq.m_name);
    printf("Peer - start_server : listening on socket\n");
    if(((fileFD = open( msg_filereq.m_name , O_RDONLY )) == -1)) { // opening the file that is wanted by another peer
        perror("Peer - open()");
        close(connectedFD);
        exit(EXIT_FAILURE);
    }
    if(fstat(fileFD, &file_stat) < 0) { // finding the size of the file
        perror("Peer fstat()");
        close(connectedFD);
        close(fileFD);
        exit(EXIT_FAILURE);
    }
    msg_filesrv.m_file_size = (int)file_stat.st_size;
    msg_filesrv.m_type = MSG_FILESRV;
    rand_int = rand() % 100;
    if(rand_int == 0 ) { // in a small probability of 1 to 100, the client will refuse to send the file or file empty
        printf("Peer - refusing to send file %s \n",msg_filereq.m_name);
        msg_filesrv.m_file_size = -1; // sending with negative size of file
        if((send(connectedFD,(char*)&msg_filesrv, sizeof(msg_filesrv),0)) < 0 ) {
            perror("Peer - send()");
            close(connectedFD);
            exit(EXIT_FAILURE);
        }
        close(fileFD);
        return;
    }
    printf("Peer - sending_file: sending MSG_FILESRV with file <%s>, file size : %d\n",msg_filereq.m_name, msg_filesrv.m_file_size );
    if((send(connectedFD,(char*)&msg_filesrv, sizeof(msg_filesrv),0)) < 0 ) { // sending the size of the file
        perror("Peer - send()");
        close(connectedFD);
        exit(EXIT_FAILURE);
    }
    printf("Peer - sending_file: sending content of file\n");



    /* When given read() a buffer larger then the content inside oit might wait, so I allocated the exact size */
    num_of_iterations = (msg_filesrv.m_file_size / P_BUFF_SIZE) + 1;
    last_size = msg_filesrv.m_file_size % P_BUFF_SIZE;
    last_buff = (char*)malloc(sizeof(char) * last_size);
    if(!last_buff){
        perror("Peer - malloc()");
        close(connectedFD);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < num_of_iterations; ++i) {
        if( i == num_of_iterations - 1 ) { // last iteration, reads and sends the exact size
            nr = read(fileFD, last_buff, (size_t)last_size);
            if (nr == -1) {
                perror("Peer - read()");
                close(connectedFD);
                exit(EXIT_FAILURE);
            }
            if ((send(connectedFD, last_buff,(size_t)last_size, 0)) < 0) { // sending the size of the file
                perror("Peer - send()");
                close(connectedFD);
                exit(EXIT_FAILURE);
            }
            printf("Peer - sending_file: file has been sent\n");
            continue;
        }
        nr = read(fileFD,buff,P_BUFF_SIZE);
        if(nr == -1){
            perror("Peer - read()");
            close(connectedFD);
            exit(EXIT_FAILURE);
        }
        printf("Peer - sending_file: sending a full buffer to client\n");
        if(( send(connectedFD,&buff, sizeof(buff),0)) < 0 ) { // sending the size of the file
            perror("Peer - send()");
            close(connectedFD);
            exit(EXIT_FAILURE);
        }
    }

    printf("Peer- sending_file: finished sending file\n");
    free(last_buff);
    close(fileFD);

}

/**********************************************************************************************************/

bool getFileFromClient(int serverFD, msg_dirent_t* msg_dirent, struct sockaddr_in* server_address, in_port_t* my_port) {
    msg_filereq_t       msg_filereq;
    msg_filesrv_t       msg_filesrv;
    msg_notify_t        msg_notify;
    msg_ack_t           msg_ack;
    msg_finish_input_t  msg_finish_input;
    int                 peerFD, fd,i, num_of_iterations, last_size, j;
    struct sockaddr_in  clientAddress;
    char                buff[P_BUFF_SIZE], *last_buff;


    clientAddress.sin_family = AF_INET;
    clientAddress.sin_port = htons(msg_dirent->m_port); // the peer's port
    clientAddress.sin_addr.s_addr = INADDR_ANY;
    inet_aton("127.0.0.1", &clientAddress.sin_addr);

    printf("Client - get_file_from_client: getting file <%s> from client on port %d: \n",msg_dirent->m_name,msg_dirent->m_port );
    if((peerFD = socket(AF_INET,SOCK_STREAM,0)) == -1){ // opening new socket
        perror("Client - socket()");
        exit(EXIT_FAILURE);
    }

    if(connect(peerFD,(struct sockaddr*)&clientAddress,sizeof(clientAddress)) < 0 ) {
        perror("Client - getFileFromClient() - connect()");
        close(peerFD);
        exit(EXIT_FAILURE);
    }
    printf("Client - get_file_from_client: established connection\n");

    msg_filereq.m_type = MSG_FILEREQ;
    strcpy(msg_filereq.m_name,msg_dirent->m_name);
    if(send(peerFD,(char*)&msg_filereq, sizeof(msg_filereq),0) < 0 ){ // sending file request to the peer
        perror("Client - send()");
        exit(EXIT_FAILURE);
    }
    printf("Client - get_file_from_client: sent MSG_FILEREQ\n");
    if(recv(peerFD,(char*)&msg_filesrv, sizeof(msg_filesrv),0) < 0){ // recieving size from of file from peer
        perror("Cleint - recv()");
        exit(EXIT_FAILURE);
    }
    printf("Client - get_file_from_client: received MSG_FILESRV: file length : %d\n",msg_filesrv.m_file_size);
    if(msg_filesrv.m_file_size < 0) { // if the peer did not want to share with this client the file
        printf("Client - peer did not want to share file (probability of 1/100)\n");
        close(serverFD);
        close(peerFD);
        return false;
    }

    printf("Client - get_file_from_client: opened file < %s >\n", msg_dirent->m_name);
    if((fd = open(msg_dirent->m_name, O_WRONLY | O_CREAT, 0666)) == -1){ // opening a new file with the same name
        perror("Client - open()");
        exit(EXIT_FAILURE);
    }

    num_of_iterations = (msg_filesrv.m_file_size / P_BUFF_SIZE) + 1;
    last_size = msg_filesrv.m_file_size % P_BUFF_SIZE;
    last_buff = (char*)malloc(sizeof(char) * last_size);
    if(!last_buff){
        perror("Peer - malloc()");
        close(fd);
        exit(EXIT_FAILURE);
    }
    for ( i = 0 ; i < num_of_iterations ; ++i ) {
        if( i == num_of_iterations -1 ){
            printf("Client - receiving last buffer from peer\n");
            if (recv(peerFD, last_buff, (size_t)last_size, 0) < 0) { // recieving size from of file from peer
                perror("Cleint - recv()");
                exit(EXIT_FAILURE);
            }
            printf("Client - writing last buffer into file\n");
            if (write(fd, last_buff, (size_t)last_size) == -1) { // writing into the file the data that was given
                perror("Client - write()");
                exit(EXIT_FAILURE);
            }
            printf("Client - get_file_from_client : received the content of the file\n");
            continue;
        }
        printf("Client - receiving full buffer from peer\n");
        if(recv(peerFD,&buff, sizeof(buff),0) < 0) { // recieving size from of file from peer
            perror("Cleint - recv()");
            exit(EXIT_FAILURE);
        }
        printf("Client - writing full buffer to file\n");
        if(write(fd,buff,(size_t)msg_filesrv.m_file_size) == -1){ // writing into the file the data that was given
            perror("Client - write()");
            exit(EXIT_FAILURE);
        }
    }

    printf("Client - get_file_from_client: obtained file < %s > from client on port:%d\n",msg_dirent->m_name, msg_dirent->m_port );
    close(fd);

    msg_finish_input.m_type = MSG_FINISH_INPUT;
    /* Sending FINISH_INPUT to peer */
    if(send(peerFD,(char*)&msg_finish_input, sizeof(msg_finish_input),0) < 0 ){ // closing connection
        perror("Client - send()");
        exit(EXIT_FAILURE);
    }

    close(peerFD);
    /*  Sending MSG_NOTIFY to the server and updating his "library of files" afterwards sending him MSG_FINISH_INPUT */
    printf("Client : msg_dirent->m_name : %s\n", msg_dirent->m_name);
    strcpy(msg_notify.m_name,msg_dirent->m_name);
    msg_notify.m_type = MSG_NOTIFY;
    msg_notify.m_addr = server_address->sin_addr.s_addr;
    msg_notify.m_port = *my_port;
    printf("Client - get_file_from_client: sending MSG_NOTIFY to update server that this client holds file: < %s >\n", msg_notify.m_name);
    if(send(serverFD,(char*)&msg_notify, sizeof(msg_notify),0) < 0 ) { // updating the server that we have a new file
        perror("Client - send()");
        exit(EXIT_FAILURE);
    }
    printf("Client - receiving MSG_ACK from Server\n");
    if(recv(serverFD,&msg_ack, sizeof(msg_ack),0) < 0) { // receiving ack on the file
        perror("Cleint - recv()");
        exit(EXIT_FAILURE);
    }
    if(*my_port == 0) {
        *my_port = msg_ack.m_port;
    }
    printf("Client: received port from server : %d\n",*my_port);

    return true;
}

/**********************************************************************************************************/

in_port_t leech(int serverFD,char* argv[],int argc,struct sockaddr_in* server_address) {

    int             i;
    msg_dirreq_t    msg_dirreq;
    msg_dirhdr_t    msg_dirhdr;
    msg_dirent_t    msg_dirent;
    in_port_t       my_port = 0;


    msg_dirreq.m_type = MSG_DIRREQ;
    printf("Client - get_list: sending MSG_DIRREQ \n");
    if(send(serverFD,(char*)&msg_dirreq, sizeof(msg_dirreq),0) < 0 ){
        perror("Client - send()");
        exit(EXIT_FAILURE);
    }
    if(recv(serverFD,(char*)&msg_dirhdr, sizeof(msg_dirhdr),0) < 0){
        perror("Cleint - recv()");
        exit(EXIT_FAILURE);
    }
    printf("Client - get_list: received MSG_DIRHDR with %d items\n",msg_dirhdr.m_count);
    for (i = 0; i < msg_dirhdr.m_count ; ++i) {
        if(recv(serverFD,(char*)&msg_dirent, sizeof(msg_dirent),0) < 0){ // inserting into msg_dirent the client/mini-server attributes
            perror("Cleint - recv()");
            exit(EXIT_FAILURE);
        }

        printf("Client - get_list: received MSG_DIRENT for <%s> @ 127.0.0.1:%u\n",msg_dirent.m_name,(unsigned int)msg_dirent.m_port);

        if(isRelevantFile(msg_dirent.m_name,argv,argc)) { // the filename is identical to a file that this client asked for
            if(getFileFromClient(serverFD,&msg_dirent,server_address,&my_port)) {
                argv[i] = NULL;
            }
        }
    }
    printf("Client - finished leeching\n");
    return my_port;
}


/* Given a msg_dirent, returns true if the port inside is already ib the array */
/**********************************************************************************************************/

bool containsPeer(struct sockaddr_in peers[],int size, msg_dirent_t msg_dirent) {
    int i = 0;

    for (i = 0; i < size; ++i)
        if(peers[i].sin_port == msg_dirent.m_port) // the port is already inside the data structure
            return true;
    return false;
}

/**********************************************************************************************************/

void shutDownClient(int sockFD){
    int                     i, tmpSock, peers_idx = 0;
    msg_dirreq_t            msg_dirreq;
    msg_dirhdr_t            msg_dirhdr;
    msg_dirent_t            msg_dirent;
    msg_shutdown_t          msg_shutdown;
    struct sockaddr_in      peers[MAX_PEERS], tmp_addr;

    isShutDown = true; // stopping loop
    printf("Client - shut_down: receiving MSG_SHUTDOWN\nClient - shut_down: Shutting Down\n");

    msg_shutdown.m_type = MSG_SHUTDOWN;
    msg_dirreq.m_type = MSG_DIRREQ;
    printf("Client - shut_down: sending MSG_DIRENT (SHUTDOWN)\n");
    if(send(sockFD,(char*)&msg_dirreq, sizeof(msg_dirreq),0) < 0 ){
        perror("Client - send()");
        exit(EXIT_FAILURE);
    }
    if(recv(sockFD,(char*)&msg_dirhdr, sizeof(msg_dirhdr),0) < 0){
        perror("Cleint - recv()");
        exit(EXIT_FAILURE);
    }
    printf("Client - shut_down: received MSG_DIRHDR with %d items (SHUTDOWN)\n",msg_dirhdr.m_count);

    for (i = 0; i < msg_dirhdr.m_count ; ++i) {
        if (recv(sockFD, (char *) &msg_dirent, sizeof(msg_dirent), 0) < 0) { // inserting into msg_dirent the client's attributes
            perror("Cleint - recv()");
            exit(EXIT_FAILURE);
        }
        printf("Client - shut_down: received MSG_DIRENT for <%s> @ 127.0.0.1:%u\n",msg_dirent.m_name, msg_dirent.m_port);
        if(!containsPeer(peers,peers_idx,msg_dirent)){ // inserting array peers only unique ports
            peers[peers_idx].sin_family = AF_INET;
            peers[peers_idx].sin_addr.s_addr = msg_dirent.m_addr;
            peers[peers_idx++].sin_port = msg_dirent.m_port;
        }
    }

    tmp_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1",&tmp_addr.sin_addr);
    for (i = 0; i < peers_idx; ++i) {
        if ((tmpSock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
            perror("Client - socket()");
            exit(EXIT_FAILURE);
        }
        tmp_addr.sin_port = htons(peers[i].sin_port);
        if (connect(tmpSock, (struct sockaddr *) &tmp_addr, sizeof(tmp_addr)) != 0) {
            perror("Client - connect() - in for_loop -  shutDownClient()");
            close(tmpSock);
            close(sockFD);
            exit(EXIT_FAILURE);
        }
        printf("Client - shut_down: sending MSG_SHUTDOWN to peer at 127.0.0.1: %d\n",peers[i].sin_port);
        if (send(tmpSock, (char *) &msg_shutdown, sizeof(msg_shutdown), 0) < 0) { // sending msg_shutDown to the specific client
            perror("Client - send()");
            exit(EXIT_FAILURE);
        }
        close(tmpSock); // closing the connection with him
    }
    printf("Client - shut_down: sending MSG_SHUTDOWN to server at 127.0.0.1: %d\n",P_SRV_PORT);
    if(send(sockFD,(char*)&msg_shutdown, sizeof(msg_shutdown),0) < 0 ){ // sending msg_shutDown to the SERVER !!
        perror("Client - send()");
        exit(EXIT_FAILURE);
    }
    close(sockFD);
    printf("Server - Exiting Successfully\n");
    exit(EXIT_SUCCESS);
}


/* Given the argv from the input checks its validataion */
/**********************************************************************************************************/

Action getAction(char* act){
    if(strcmp(act,"seed") == 0)
        return act_seed;
    if(strcmp(act,"leech") == 0)
        return act_leech;
    if(strcmp(act,"shutdown") == 0)
        return act_shutDown;
    printf("Error occured - no such command %s\n",act);
    exit(EXIT_FAILURE);
}

/**********************************************************************************************************/

int main(int argc, char* argv[]) {
    int                 sock_server_fd, sock_client_fd, message, connected_fd;
    bool                isFinishInput;
    Action              action;
    struct sockaddr_in  server_address;
    in_port_t           my_port = 0;
    msg_finish_input_t  msg_finish_input;

    if(argc < 2){
        printf("Not enough arguments\n");
        exit(EXIT_FAILURE);
    }
    action = getAction(argv[1]);
    if((sock_server_fd = socket(AF_INET,SOCK_STREAM,0)) == -1){
        perror("Client - socket()");
        exit(EXIT_FAILURE);
    }
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(P_SRV_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    inet_aton("127.0.0.1",&server_address.sin_addr);

    if(connect(sock_server_fd,(struct sockaddr*)&server_address,sizeof(server_address)) != 0 ) {
        perror("Client - connect()");
        close(sock_server_fd);
        exit(EXIT_FAILURE);
    }

    switch(action) {
        case act_shutDown:
            shutDownClient(sock_server_fd);
            break;
        case act_seed:
            if (argc < 3) {
                perror("seed -> needs at least one file");
                close(sock_server_fd);
                exit(EXIT_FAILURE);
            }
            my_port = seed(sock_server_fd, argv + 2, argc - 2, &server_address); // first two arguments are: #1 -> run file, #2 -> action
            break;
        case act_leech:
            if (argc < 3) {
                perror("leech -> needs at least one file");
                close(sock_server_fd);
                exit(EXIT_FAILURE);
            }
            my_port = leech(sock_server_fd,argv + 2, argc - 2, &server_address);
            break;
    }

    /* Sending the server a message that the client finished his "job"  */
    printf("Client - sending the server MSG_FINISH_INPUT\n");
    msg_finish_input.m_type = MSG_FINISH_INPUT;
    if(send(sock_server_fd,(char*)&msg_finish_input, sizeof(msg_finish_input),0) < 0 ) {
        perror("Client - send()");
        exit(EXIT_FAILURE);
    }
    close(sock_server_fd);

    /*  Moving from being a client to act like a server (answer requests and shutdown)  */
    server_address.sin_port = htons(my_port);

    if((sock_client_fd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("Client - socket()");
        exit(EXIT_FAILURE);
    }
    if((bind(sock_client_fd,(struct sockaddr*)&server_address,sizeof(server_address))) == -1) {
        perror("Server - bind()");
        close(sock_server_fd);
        exit(EXIT_FAILURE);
    }
    if ((listen(sock_client_fd, MAX_QUEUE_SIZE)) != 0) {
        printf("Server - listen()");
        close(sock_server_fd);
        exit(EXIT_FAILURE);
    }


    /* From now on acts like a server - gives files to peers or shutsdown  */

    printf("Client : From now on im acting like a server\n");
    isShutDown = false;
    while(!isShutDown) {
        if ((connected_fd = accept(sock_client_fd, NULL, NULL)) < 0) { // starting a conversation with a peer
            perror("Client - accept()");
            close(sock_server_fd);
            exit(EXIT_FAILURE);
        }
        isFinishInput = false;
        while (!isFinishInput) { // while is in a conversation with another peer 
            if (recv(connected_fd, &message, sizeof(int), MSG_PEEK) <= 0)
                continue;

            switch (message) {

                case MSG_SHUTDOWN:
                    close(connected_fd);
                    close(sock_client_fd);
                    isShutDown = true;
                    isFinishInput = true;
                    break;

                case MSG_FILEREQ:
                    sendFileToPeer(connected_fd);
                    break;

                case MSG_FINISH_INPUT:
                    isFinishInput = true;
                    break;

                default:
                    break;
            }
        }
        printf("Peer - listening and waiting\n");
    }
    printf("Peer : EXIT SUCCESSFULLY\n");
    return 0;
}