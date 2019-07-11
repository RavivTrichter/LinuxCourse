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

#define NUM_OF_CATEGORIES 9 // two categories are treated differently

/***********************************************************/

void turnOn(int* flag) {
	*flag |= 2;
}

/***********************************************************/

bool isTurnedOn(int* flag){
	int num = *flag & 2;
	if(num == 2)
		return true;
    return false;
}

/***********************************************************/

void turnOff(int* flag){
    *flag &= 5;
}

/***********************************************************/
//returns true if the category is contained in the category list
bool ContainsCategory(char* category, char* Categories[]){
    int     i;
    pid_t   pid;

    for(i = 0 ; i < NUM_OF_CATEGORIES ; i++){
        if(strcmp(Categories[i],category) == 0)
            return true;
    }
    return false;
}

/***********************************************************/
//one string is copied with a lot of whitespaces, so this
//strcmp checks the size of the short one
bool myStrCmp(char* shortStr,char* longStr){
    int i, len = strlen(shortStr);

    for (i = 0; i < len; ++i)
        if(longStr[i] != shortStr[i])
            return false;
    return true;
}

/***********************************************************/
//difference is that this function uses myStrCmp
bool Contains(char* category, char* Categories[],int size){
    int     i;
    pid_t   pid;

    for(i = 0 ; i < size ; i++){
        if(myStrCmp(Categories[i],category))
            return true;
    }
    return false;
}

/***********************************************************/

void printAdByCat(board_ent_t board){
    int         i, len = strlen(board.text);
    const int   max_text = 70;

    printf("%s  ",board.category);
    for (i = 0; i < max_text && i < len ; ++i)
        printf("%c",board.text[i]);
    printf("\n");
}

/***********************************************************/

int main(int argc, char* argv[]){
    int         i, sz = 0, shmid,ch;
    char        *categories[argc],*cleanScreen = "\033[2J\033[;H"; // if all the categories that were given are valid that is the maximum size
    board_t*    board;
    pid_t       pid;
    char*       Original_Categories[NUM_OF_CATEGORIES] = {"Appliances","Cameras","Computers","Electronics",
                                                        "Jewelry","Motorcycles","Music","Televisions","Tools"};

    if(argc < 2){
        perror("Not enough arguments");
        exit(EXIT_FAILURE);
    }
	
	//checks which categories were given (special cases : "Home Audio Video" , "Arts Crafts") cause they have a space between the words
    for(i = 1; i < argc ; ++i){
        if(strcmp(argv[i],"Home") == 0){
             if( i+2 < argc && strcmp(argv[i+1],"Audio") == 0 && strcmp(argv[i+2],"Video") == 0){
                categories[sz++] = strdup("Home Audio Video");
                i += 2; // for the next two words "Audio" and "Video"
            }
            continue;
        }   
        else if(strcmp(argv[i],"Arts") == 0){
            if(i + 1 < argc && strcmp(argv[i+1],"Crafts") == 0){
                categories[sz++] = strdup("Arts Crafts");
                ++i;
            }
            continue;
        }
        if(ContainsCategory(argv[i],Original_Categories)) // category that was given exist
            categories[sz++] = strdup(argv[i]);
    }
    if(sz == 0){
        perror("No categories were given");
        exit(EXIT_FAILURE);
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
    else { // father's process
        turnOn(&board->flag);
        printf("Listing categories for ");
        for (i = 0; i < sz; ++i)
            printf( i != sz-1 ? "%s, " : "%s :\n",categories[i]);
        printf("\n");
        while(isTurnedOn(&board->flag)) {
            for (i = 0; i < B_ENT_CNT; ++i) {
                if (!Contains(board->entries[i].category, categories, sz)) { // specific category isn't relevant
                    continue;
                }
                printAdByCat(board->entries[i]);
            }
            printf("%s",cleanScreen);
            sleep(5);
        }
    }
    for(i = 0 ; i < sz ; ++i) // freeing strings that were allocated above
        free(categories[i]);
	
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