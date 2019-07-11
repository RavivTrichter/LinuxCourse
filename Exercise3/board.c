//
// Created by Raviv Trichter on 12/3/18.
//            204312086

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <stdbool.h>
#include <memory.h>
#include <unistd.h>
#include "board.h"


/***********************************************************/
// Turns off the first bit,6 = 110 => so doing and operator
void turnOff(int* flag){
    *flag &= 6;
}

/***********************************************************/
//Turns on the first bit with an or operator
void turnOn(int* flag){
    *flag |= 1;
}

/***********************************************************/
//returns true iff the LSB is on
bool isTurnedOn(int* flag){
    return *flag % 2 == 1 ? true : false;
}

/***********************************************************/
char* myGetLine(int fd){
    ssize_t     nr;
    char        buff, *line;
    int         i = 0;

    line = (char*)malloc((B_DATE_LEN + B_CAT_LEN + B_TEXT_LEN + 1) * sizeof(char)); // max size of a line

    while((nr = read(fd,&buff, sizeof(buff))) != 0 && buff != '\n') {
        if(nr == -1){
            perror("read() in while loop -> myGetLine()");
            exit(EXIT_FAILURE);
        }
        line[i++] = buff; // concatenating all characters into line
    }
    if(nr == 0){ // EOF
        return NULL;
    }
    line[i] = '\0';
    return line; // line is realesed in while loop (main function)
}
/***********************************************************/
//gets a date as a string and returns a struct date initialized
date_t getDate(char* line) {
    date_t  date;
    char*   s;

    s = strdup(line);

    date.day = atoi(strtok(s,"/"));
    date.month = atoi(strtok(NULL,"/"));
    date.year = atoi(strtok(NULL," "));
    free(s);
    return date;
}

/***********************************************************/

void printAd(int idx, board_ent_t board){
    int day = board.date.day,month = board.date.month,year = board.date.year, i;
    printf( day > 9 ? "%d: %d/" : "%d: 0%d/", idx,day);
    printf( month > 9 ? "%d/%d  " : "0%d/%d  ",month,year);
    printf("%s  ",board.category);
    for (i = 0; i < B_DATE_LEN + B_CAT_LEN; ++i) // Random size that i got (it equals 30)
        printf("%c",board.text[i]);
    printf("\n");
}

/***********************************************************/

int main(int argc, char* argv[]) {
    int         shmid,fd,i = 0;
    board_t*    board;
    pid_t       pid;
    char        ch = '1', *line, *cleanScreen = "\033[2J\033[;H";
    date_t      date;

    if(argc < 2){
        perror("Not enough arguments for program");
        exit(EXIT_FAILURE);
    }

    /*  create the segment: */
    if ((shmid = shmget(B_KEY, sizeof(board_t),0644 | IPC_CREAT)) == -1) {
        perror("shmget()");
        exit(EXIT_FAILURE);
    }

    /* Attach the segment to the board */
    board = shmat(shmid, (void *)0, 0);
    if (board == (char*)(-1)) {
        perror("shmat()");
        exit(EXIT_FAILURE);
    }
    board->flag = 0; // initializing the flag to zero
    if((fd = open(argv[1],O_RDONLY,0644)) == -1){
        perror("open() -> Input File");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if(pid == 0){ // son's process
        if((ch = read(0,&ch,1)) == -1){
            perror("read() -> son's process");
            exit(EXIT_FAILURE);
        }
        turnOff(&board->flag);
        exit(EXIT_SUCCESS);
    }
	else { // father's process
        turnOn(&board->flag);
        while(isTurnedOn(&board->flag)){
            line = myGetLine(fd);
            if(!line){ // setting file to the beginning because of EOF
                lseek(fd,0,SEEK_SET);
                continue;
            }
            if( i % 20 == 0 )
                printf("%s",cleanScreen);
            board->entries[i].date = getDate(line);
            strncpy(board->entries[i].category,line + B_DATE_LEN,B_CAT_LEN); // copies category
            strcpy(board->entries[i].text,line + B_DATE_LEN + B_CAT_LEN); // copies text
            printAd(i,board->entries[i]);
            if(++i == B_ENT_CNT) // viewed all entries, so starting over
                i = 0;
            free(line); // allocated every time in function myGetLine()
            sleep(1);
        }
    }
    /* detach from the segment */
    if (shmdt(board) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }


    /* Closing the file */
    if(close(fd) == -1){
        perror("close()");
        exit(EXIT_FAILURE);
    }
    /* releasing the shared memory */
    if(shmctl(shmid,IPC_RMID,NULL) == -1){
        perror("shmctl()");
        exit(EXIT_FAILURE);
    }

    return 0;
}