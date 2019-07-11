//
// Created by Raviv Trichter on 12/29/18.
//

#include "p2p.h"

/* Global Variables */
bool isShutDown;
file_ent_t files[MAX_FILES];
int numOfCurrentFiles, clientPort;



/**********************************************************************************************************/
void shutDown() {
    isShutDown = true;
    printf("Server - notify: receiving MSG_SHUTDOWN\nServer - notify: Shutting Down\n");
}

/**********************************************************************************************************/

void dirReq(int sockFD) {
    msg_dirreq_t    msg_dirreq;
    msg_dirhdr_t    msg_dirhdr;
    msg_dirent_t    msg_dirent;
    int             i = 0;

    printf("Server - dirreq: receiving MSG_DIRREQ\n");
    if(recv(sockFD,(char*)&msg_dirreq,sizeof(msg_dirreq),0) < 0){
        perror("Server - recv() -> MSG_NOTIFY");
        close(sockFD);
        exit(EXIT_FAILURE);
    }
    msg_dirhdr.m_type = MSG_DIRHDR;
    msg_dirhdr.m_count = numOfCurrentFiles;
    printf("Server - notify: sending MSG_NOTIFY with count = %d\n",numOfCurrentFiles);
    if(send(sockFD,(char*)&msg_dirhdr, sizeof(msg_dirhdr),0) < 0 ){
        perror("Server - send()");
        close(sockFD);
        exit(EXIT_FAILURE);
    }
    msg_dirent.m_type = MSG_DIRENT;
    for(i = 0 ; i < numOfCurrentFiles ; ++i){ // sending all files to a client
        msg_dirent.m_port = files[i].fe_port;
        msg_dirent.m_addr = files[i].fe_addr;
        strcpy(msg_dirent.m_name,files[i].fe_name);
        if(send(sockFD,(char*)&msg_dirent, sizeof(msg_dirent),0) < 0 ){
            perror("Server - send()");
            close(sockFD);
            exit(EXIT_FAILURE);
        }
    }
}


/**********************************************************************************************************/

void notify(int sockFD) {
    msg_notify_t    msg_notify;
    msg_ack_t       msg_ack;

    printf("Server - notify: receiving MSG_NOTIFY\n");
    if(recv(sockFD, &msg_notify, sizeof(msg_notify), 0) < 0 ) {
        printf("Server - Error occurred while receiving MSG_NOTIFY\n");
        close(sockFD);
        return;
    }

    msg_ack.m_type = MSG_ACK;
    if(msg_notify.m_port == 0) {
        printf("Server - notify: assigned port %d\n", clientPort);
        msg_ack.m_port = (in_port_t)clientPort++;
    }
    else
        msg_ack.m_port = msg_notify.m_port;
    strcpy(files[numOfCurrentFiles].fe_name, msg_notify.m_name);
    files[numOfCurrentFiles].fe_addr = msg_notify.m_addr;
    files[numOfCurrentFiles++].fe_port = msg_ack.m_port;
    printf("Server - notify: sending MSG_ACK\n");
    if(send(sockFD,(char*)&msg_ack, sizeof(msg_ack),0) < 0 ){
        perror("Server - send()");
        close(sockFD);
        exit(EXIT_FAILURE);
    }
}

/**********************************************************************************************************/

int main(int argc, char* argv[]) {
    int                 sockFD = 0, connectedFD = 0, message = -1;
    struct sockaddr_in  myaddr;
    bool                isFinishInput;

    myaddr.sin_family = AF_INET;
    myaddr.sin_port = htons(P_SRV_PORT);
    myaddr.sin_addr.s_addr = INADDR_ANY;

    if((sockFD = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("Server - socket()");
        exit(EXIT_FAILURE);
    }
    if(inet_aton("127.0.0.1", &myaddr.sin_addr) != 1 ) {
        perror("Server - inet_aton()");
        close(sockFD);
        exit(EXIT_FAILURE);
    }
    printf("Server - server: opening socket on 127.0.0.1:%d\n", P_SRV_PORT);
    if((bind(sockFD,(struct sockaddr*)&myaddr,sizeof(myaddr))) == -1) {
        perror("Server - bind()");
        close(sockFD);
        exit(EXIT_FAILURE);
    }
    if ((listen(sockFD, MAX_QUEUE_SIZE)) != 0) {
        printf("Server - listen()");
        close(sockFD);
        exit(EXIT_FAILURE);
    }

    isShutDown = false;
    numOfCurrentFiles = 0;
    clientPort = P_SRV_PORT + 1;

    while(!isShutDown) {
        if ((connectedFD = accept(sockFD, NULL, NULL)) < 0) { // starting a conversation with a peer
            perror("Server - accept()");
            close(sockFD);
            exit(EXIT_FAILURE);
        }
        isFinishInput = false;
        while (!isFinishInput) { // while is in a conversation with a client

            if (recv(connectedFD, &message, sizeof(int), MSG_PEEK) <= 0)
                continue;

            switch (message) {
                case MSG_NOTIFY:
                    notify(connectedFD);
                    break;

                case MSG_DIRREQ:
                    dirReq(connectedFD);
                    break;

                case MSG_SHUTDOWN:
                    shutDown();
                    close(connectedFD);
                    close(sockFD);

                case MSG_FINISH_INPUT:
                    printf("Server - Received MSG_FINISH_INPUT from client\n");
                    isFinishInput = true;
                    break;
                default:
                    break;
            }
        }
        printf("Server : finished with a client, waiting for the next one\n");
    }
    printf("Server : EXIT SUCCESSFULLY\n");
    return 0;
}


