/* Raviv Trichter 204312086 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>


#define MAX_CHARS 128 // assuming that every line will not be more than 128 chars
#define MAX_ARGS 20 // assuming that every line will not be more than 20 arguments
typedef enum {Regular,ReadInto,WriteInto,ReadAndWrite} Status; // describes where to read and write from

bool run_in_backround; // flag to tell if contains "&" string


/* Reads all input from user and returns the number of strings that were received */
/**********************************************************************************/
char** breakInputByTokens(int* size){
    char            *token,buff,tmp_line[MAX_CHARS],**args,line[MAX_CHARS];
    ssize_t         nr;
    int i = *size;

    args = (char**)malloc(sizeof(char*)*MAX_ARGS); // allocating a place for 20 arguments

    while((nr = read(0,&buff, sizeof(buff))) != 0 && buff != '\n') {
        if(nr == -1){
            perror("read() in while loop -> breakInputToTokens");
            exit(EXIT_FAILURE);
        }
        line[i++] = buff; // concatenating all characters into line
    }
    line[i] = '\0'; // making it a string
    i = 0;
    strcpy(tmp_line,line);
    token = strtok(tmp_line," ");
    if(strcmp(token,"exit") == 0)
        return NULL;
    while(token != NULL){ // copying & allocating memory for the strings
        args[i] = (char*)malloc(sizeof(char) * (strlen(token)+1));
        strcpy(args[i++],token);
        token = strtok(NULL," ");
    }
    args[i] = NULL;
    *size = i;
    return args; // number of strings that were received
}


/*  Frees all strings  */
/**********************************************************************************/
void freeStrings(char* args[], int size){
    int i;

    for (i = 0; i < size; ++i)
        free(args[i]);
}

/* sets the IO for the command */
/**********************************************************************************/
void setIO(Status status ,char* input, char* output){
    switch(status){
        case Regular:
            break;
        case ReadInto:
            if(close(0) == -1){
                perror("close (0) - Status => ReadInto ");
                exit(EXIT_FAILURE);
            }
            if(open(input,O_RDONLY,0777) == -1){
                perror("open() - Status => ReadInto ");
                exit(EXIT_FAILURE);
            }
            break;
        case WriteInto:
            if(close(1) == -1){
                perror("close (1) - Status => WriteInto ");
                exit(EXIT_FAILURE);
            }
            if(open(output,O_WRONLY | O_CREAT,0777) == -1){
                perror("open() - Status => WriteInto");
                exit(EXIT_FAILURE);
            }
            break;
        case ReadAndWrite:
            if(close(0) == -1){
                perror("close (0) - Status => ReadAndWrite ");
                exit(EXIT_FAILURE);
            }
            if(open(input,O_RDONLY,0777) == -1){
                perror("open() - Status => ReadAndWrite");
                exit(EXIT_FAILURE);
            }
            if(close(1) == -1){
                perror("close (1) - Status => ReadAndWrite ");
                exit(EXIT_FAILURE);
            }
            if(open(output,O_WRONLY | O_CREAT,0777) == -1){
                perror("open()- Status => ReadAndWrite");
                exit(EXIT_FAILURE);
            }
            break;
    }
}
/* copying the neccessary strings (without "<", ">" and file names) to another 2D Array */
/**********************************************************************************/
void copyStrings(char** arguments,int args_size,char** args_without_symbols,int args_without_size){
    int i;

    freeStrings(arguments,args_size); // free the current strings in arguments
    for(i = 0; i < args_without_size ; ++i){
        arguments[i] = (char*)malloc(sizeof(char)*(strlen(args_without_symbols[i])+1));
        strcpy(arguments[i],args_without_symbols[i]);
    }
    arguments[i] = NULL;
    /* free the strings with ">" et. c  */
    freeStrings(args_without_symbols,args_without_size);
}

/* getting the command and its arguments and running them with execvp function */
/**********************************************************************************/
int run(char** arguments, int* sz){
    int         i = 0,j = 0 ,wait_stat,size = *sz;
    pid_t       pid;
    char       **arguments_without_symbols = NULL, *input_def = NULL, *output_def = NULL;
    Status      status = Regular;

    for (i = 0; i < size; ++i) {
        if(strcmp(arguments[i],"<") == 0) {
            status = (status != Regular) ? ReadAndWrite : ReadInto;
            input_def = strdup(arguments[i+1]);
            continue;
        }
        else if(strcmp(arguments[i],">") == 0) {
            status = (status != Regular) ? ReadAndWrite : WriteInto;
            output_def = arguments[i + 1];
            continue;
        }
    }
    run_in_backround = (strcmp("&",arguments[size-1]) == 0);
    if(run_in_backround) // erasing the "&" string
        arguments[--size] = '\0';

    pid = fork();
    if(pid == 0) {  // son of process
        setIO(status,input_def,output_def); // set the relevenat IO streams (contains "<" or ">")
        if(status != Regular) {
            arguments_without_symbols = (char**) malloc(size * sizeof(char *));
            for (i = 0; i < size; ++i) {
                if (strcmp(arguments[i], ">") == 0 || strcmp(arguments[i], "<") == 0) {
                    i++; // so the file won't get inside the arguments
                    continue;
                }
                arguments_without_symbols[j] = (char *) malloc(sizeof(char) * strlen(arguments[i])+1);
                strcpy(arguments_without_symbols[j++], arguments[i]);
            }
            copyStrings(arguments,size,arguments_without_symbols,j); // j is the number of relevant symbols
            *sz = j; // updating the real size value
        }
        execvp(arguments[0], arguments);
        perror("execvp()");
        exit(EXIT_FAILURE);
    }
    if(!run_in_backround) { //father is waiting for his son,otherwise father continues
        wait(&wait_stat);
        if (WIFSIGNALED(wait_stat)) {
            perror("wait() - returned with a problem");
            exit(EXIT_FAILURE);
        }
    }
}
/**********************************************************************************/

int main() {
    char    **arguments;
    int     size = 0;

    run_in_backround = false;

    while(1){
        printf("@ ");
        fflush(stdout);
        arguments = breakInputByTokens(&size);
        if(!arguments) // got back "exit" as a string
            break;
        run(arguments,&size);
        freeStrings(arguments, size);
        size = 0;
    }

    return 0;
}