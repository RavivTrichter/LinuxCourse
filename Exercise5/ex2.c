//
// Created by Raviv Trichter on 1/8/19.
//


#include <stdio.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <termios.h>


#define BUFF_SIZE 512

int dbl_copy(int f1, int t1, int f2, int t2) {
    int 	i, c, max, n, cnt=0;
    char 	buff[BUFF_SIZE];
    fd_set 	fdset;
    
    FD_ZERO(&fdset);
    FD_SET(f1, &fdset);
    FD_SET(f2, &fdset);

	max = (f1 > f2 )? f1 + 1 : f2 + 1;

	if ((c = select(max, &fdset, NULL,NULL, NULL)) < 0) {
		perror("select()");
		exit(EXIT_FAILURE);
	}	
	if (c > 0) {
		if (FD_ISSET(f1, &fdset)) {
			if((n = read(f1, buff, sizeof(buff))) < 0 ){
				perror("read()");
				exit(EXIT_FAILURE);
			}
			if((write(t1, buff, n)) < 0 ){
				perror("write()");
				exit(EXIT_FAILURE);
			}
		}
		if (FD_ISSET(f2, &fdset)) {
			if((n = read(f2, buff, sizeof(buff))) < 0 ){
				perror("read()");
				exit(EXIT_FAILURE);
			}
			if((write(t2, buff, n)) < 0 ){
				perror("write()");
				exit(EXIT_FAILURE);
			}
		}
	}
    return n;
}

int main(int argc, char* argv[]){
	int fdlist[2];

	if(pipe(fdlist) < 0){
		perror("pipe()");
		exit(EXIT_FAILURE);
	}

	/* acting like cat */
	while(1) {
		dbl_copy(STDIN_FILENO,fdlist[1],fdlist[0],STDOUT_FILENO);
	}

	return 0;
}