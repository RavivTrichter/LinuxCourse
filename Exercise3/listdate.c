//
// Created by Raviv Trichter 204312086 on 12/3/18.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>
#include "board.h"

typedef enum {OnlyFrom, FromAndUpTo} Condition;

/***********************************************************/
//doing and with 3 = 11(binary) assures that the bit for 2^2 will be 0
void turnOff(int* flag){
    *flag &= 3;
}

/***********************************************************/
//will get 4 only if the bit 2^2 is on
bool isTurnedOn(int* flag){
	int num = *flag & 4;
	if( num == 4 )
		return true;
	return false;
}

/***********************************************************/

void turnOn(int* flag){
    *flag |= 4;
}

/***********************************************************/

void printAdByDate(board_ent_t board){
    const int   max_text = 70;
    int         i,day = board.date.day,month = board.date.month,year = board.date.year, len = strlen(board.text);
    printf( day > 9 ? "%d/" : "0%d/",day);
    printf( month > 9 ? "%d/%d\t" : "0%d/%d\t",month,year);
    for (i = 0; i < max_text && i < len ; ++i)
        printf("%c",board.text[i]);
    printf("\n");
}

/***********************************************************/

date_t getDate(char* line) {
    date_t  date;
    char*   s;

    s = strdup(line);
    date.day = atoi(strtok(s,"/"));
    date.month = atoi(strtok(NULL,"/"));
    date.year = atoi(strtok(NULL," "));
	if(date.day < 1 || date.day > 31 || date.month < 1 || date.month > 12 || date.year < 1970 || date.year > 2020){
		perror("Date given is invalid");
		exit(EXIT_FAILURE);
	}
    free(s);
    return date;
}

/***********************************************************/
//returns true if the bigger date is after the smaller one
bool checkDate(date_t smaller,date_t bigger){
    if(smaller.year < bigger.year)
        return true;
    if(smaller.year == bigger.year && smaller.month < bigger.month)
        return true;
    if(smaller.year == bigger.year && smaller.month == bigger.month 
        && smaller.day < bigger.day)
        return true;
    return false;
}

/***********************************************************/
//sends by the Condition
bool isValidDate(board_ent_t board, Condition condition,date_t from, date_t upTo){

    if(condition == OnlyFrom)
        return checkDate(from,board.date);
    else
        return (checkDate(from,board.date) && checkDate(board.date,upTo));
}

/***********************************************************/

int main(int argc, char* argv[]){
    int         i, sz = 0, shmid,ch;
    board_t*    board;
    pid_t       pid;
    date_t      from,upTo;
    Condition   condition;
    char        *cleanScreen = "\033[2J\033[;H";


    if(argc < 2 || argc > 3){
        perror("Not enough arguments");
        exit(EXIT_FAILURE);
    }
    if(argc == 2){ // got only one date
        condition = OnlyFrom;
        from = getDate(argv[1]);
    }
    else if(argc == 3){ // got two dates
        condition = FromAndUpTo;
        from = getDate(argv[1]);
        upTo = getDate(argv[2]);
    }
	
	/*  create the segment: */
    if ((shmid = shmget(B_KEY, sizeof(board_t), 0644 | IPC_CREAT)) == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

	/* Attach the segment to the board */
    board = shmat(shmid, (void *)0, 0);
    if (board == (char*)(-1)) {
        perror("shmat");
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
    else {
        turnOn(&board->flag);
        if(condition == OnlyFrom)
            printf("Listing dates from %s\n\n",argv[1]);
        else
            printf("Listing dates from %s up to %s\n\n",argv[1],argv[2]);
        while(isTurnedOn(&board->flag)) {
            for (i = 0; i < B_ENT_CNT; ++i) {
                if(isValidDate(board->entries[i],condition,from,upTo))
                    printAdByDate(board->entries[i]);
            }
            printf("%s",cleanScreen);
            sleep(5);
        }
    }
    /* detach from the segment */
    if (shmdt(board) == -1) {
        perror("shmdt");
        exit(1);
    }
	
	    /* releasing the shared memory */
    if(shmctl(shmid,IPC_RMID,NULL) == -1){
        perror("shmctl()");
        exit(EXIT_FAILURE);
    }

    return 0;
}