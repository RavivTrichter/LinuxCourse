//
// Created by Raviv Trichter on 1/8/19.
//


#define _XOPEN_SOURCE 600

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

/*******************************************************************************************************
											Exercise 1
*******************************************************************************************************/

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

/*******************************************************************************************************
											Exercise 2
*******************************************************************************************************/

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


/*******************************************************************************************************
											Exercise 3
*******************************************************************************************************/

void filter() {
	int 	fdm, fds, num;
	pid_t 	pid;
	char 	*pts_name;
	
	if ((fdm = posix_openpt(O_RDWR)) < 0){
		perror("posix_openpt()");
		exit(EXIT_FAILURE);
	}
	
	pid = fork();
	if (pid == 0) {
		if(grantpt(fdm) < 0){
			perror("grantpt()");
			exit(EXIT_FAILURE);
		}
		if (unlockpt(fdm) < 0 ){
			perror("unlockpt()");
			exit(EXIT_FAILURE);
		}
		if((pts_name = ptsname(fdm)) == NULL){
			perror("ptsname()");
			exit(EXIT_FAILURE);
		}

		if((fds = open(pts_name, O_RDWR)) < 0){
			perror("open()");
			exit(EXIT_FAILURE);
		}
		close(fdm);
		close(STDIN_FILENO);
		dup(fds);
		close(STDOUT_FILENO);
		dup(fds);
		close(STDERR_FILENO);
		dup(fds);
		close(fds);
		execlp("/bin/cat","cat",NULL);
		exit(EXIT_FAILURE);
	}

	while (1){
		// ctl_echo(fdm, 0); //ECHO is off - the input i insert i not printed at all to the output
		ctl_eof(fdm, 0); //VEOF is off - 
		dbl_copy(STDIN_FILENO, fdm, fdm, STDOUT_FILENO);
	}

}



int main(int argc, char* argv) {

	filter();
	
	return 0;
}