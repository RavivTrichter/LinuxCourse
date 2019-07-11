//
// Created by Raviv Trichter on 1/8/19.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>


int ctl_echo(int fd, int flag) {

    struct termios term;

    if(isatty(fd) == 0) {
        perror("isAtty()");
        return -1;
    }

    if(tcgetattr(fd, &term) < 0) {
        perror("tcgetattr()");
        return -1;
    }

    term.c_lflag = (flag == 0) ? term.c_lflag & (~ECHO) : term.c_lflag | ECHO;
   
    if(tcsetattr(fd, TCSAFLUSH, &term) < 0) {
        perror("tcsetattr()");
        return -1;
    }
 
    return 0;
}



int ctl_eof(int fd, int flag) {
    struct termios  term;

    if (isatty(fd) == 0) { // predicate - returns true if is a terminal
        perror("isatty()");
        return -1;
    }

    if (tcgetattr(fd, &term) < 0) {
        perror("tcgetattr()");
        return -1;
    }

    term.c_lflag = (flag == 0) ? term.c_lflag & (~VEOF) : term.c_lflag | VEOF;

    if (tcsetattr(fd, TCSAFLUSH, &term) < 0) {
        perror("tcsetattr()");
        return -1;
    }

}



int main(int argc, char* argv[]) {

    if(argc < 3){
        perror("Error - Not Enough Arguments");
        exit(EXIT_FAILURE);
    }

    if(strcmp(argv[1],"eof") == 0){
        if(strcmp(argv[2],"on") == 0)
            ctl_eof(STDERR_FILENO,1);
        else if(strcmp(argv[2],"off") == 0)
            ctl_eof(STDERR_FILENO,0);
        else {
            printf("Problem occured\n");
            exit(EXIT_FAILURE);
        }
    }
    else if(strcmp("echo",argv[1]) == 0) {
        if(strcmp(argv[2],"on") == 0)
            ctl_echo(STDERR_FILENO,1);
        else if(strcmp(argv[2],"off") == 0)
            ctl_echo(STDERR_FILENO,0);
        else {
            printf("Problem occured\n");
            exit(EXIT_FAILURE);
        }
    }else {
        printf("No such command %s\n",argv[1]);
        exit(EXIT_FAILURE);
    }
    return 0;
}