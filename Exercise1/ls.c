#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <dirent.h>
#include <time.h>

/* enumeration of the three type that a file cam get */
typedef enum FileType {Regular ,Directory, Symbolic } FileType;

/* A function that prints the permissions of a given file */
void printPermissions(FileType fileType,mode_t fileMode){
    char permissions[11];

    if(fileType != Regular)
        permissions[0] = fileType == Directory ? 'd' : 'l'; // if true then 'd' else Hard Linked so 'l'
    else
        permissions[0] = '-';
    permissions[1] = (fileMode & S_IRUSR) == S_IRUSR ? 'r' : '-';
    permissions[2] = (fileMode & S_IWUSR) == S_IWUSR ? 'w' : '-';
    permissions[3] = (fileMode & S_IXUSR) == S_IXUSR ? 'x' : '-';
    permissions[4] = (fileMode & S_IRGRP) == S_IRGRP ? 'r' : '-';
    permissions[5] = (fileMode & S_IWGRP) == S_IWGRP ? 'w' : '-';
    permissions[6] = (fileMode & S_IXGRP) == S_IXGRP ? 'x' : '-';
    permissions[7] = (fileMode & S_IROTH) == S_IROTH ? 'r' : '-';
    permissions[8] = (fileMode & S_IWOTH) == S_IWOTH ? 'w' : '-';
    permissions[9] = (fileMode & S_IXOTH) == S_IXOTH ? 'x' : '-';
    permissions[10] = '\0';
    printf("%s   ",permissions);
}

/* Returns type of file by a given stat struct */
FileType getFileType(mode_t mode) {
    if(S_ISREG(mode))
        return Regular;
    else if(S_ISDIR(mode))
        return Directory;
    else
        return Symbolic;
}

/* the ls function for every type of file */
void ls(char* path,struct stat* buff,FileType curr_file_type) {
    struct passwd* pwd;
    struct group* grp;
    struct tm* time_info;
    char time_buffer[15], *ptr,tmp_string[PATH_MAX];
    time_buffer[14] = '\0';

    if(lstat(path, buff) == -1){
        perror("stat() function in ls");
        exit(EXIT_FAILURE);
    }
    printPermissions(curr_file_type,buff->st_mode);
    printf("%3lu ",(unsigned long)buff->st_nlink); //print nlinks
    if ((pwd = getpwuid(buff->st_uid)) != NULL) //print uid
        printf("%6s  ", pwd->pw_name);
    else
        perror("getpwuid() ");
    if ((grp = getgrgid(buff->st_gid)) != NULL) // print gid
        printf("%6s  ", grp->gr_name);
    else
        perror("getgrgid()");
    printf("%6llu  ",(unsigned long long)buff->st_size); //prints the size of bytes
    time_info = localtime((const time_t*)&buff->st_mtim);
    strftime(time_buffer,sizeof(time_buffer),"%b %d %Y",time_info);
    printf("%s  ", time_buffer);// print last modified time
    /*printing the file's name */
    if(curr_file_type != Symbolic)
        printf("%s\n", (strrchr(path,'/')+1));
    else{
        printf("%s -> ", (strrchr(path,'/')+1));
        ptr = realpath(path,tmp_string);
        if(!ptr)
            perror("realpath()");
        else
            printf("%s\n",(strrchr(ptr,'/')+1));
    }
}

/* getting the string from argv[], and sending to ls with the appropriate file type*/
void gettingReadyForLS(char* path){
    struct stat buff;
    DIR *dir;
    struct dirent *entry;
    FileType file_type;
    char tmp_path[PATH_MAX];

    if(lstat(path, &buff) == -1){
        perror("stat() function");
        exit(EXIT_FAILURE);
    }
    file_type = getFileType(buff.st_mode);
    if(file_type == Regular || file_type == Symbolic)
        ls(path,&buff,file_type);
    else{ // file_type must be a directory
        if ((dir = opendir(path)) == NULL){
            perror("opendir() error");
            exit(EXIT_FAILURE);
        }
        else {
            while ((entry = readdir(dir)) != NULL) { // reading all sub-directories
                strcpy(tmp_path,path);
                strcat(tmp_path,"/");
                strcat(tmp_path,entry->d_name);
                if(lstat(tmp_path, &buff) == -1){
                    perror("lstat() in while");
                    exit(EXIT_FAILURE);
                }
                file_type = getFileType(buff.st_mode);
                ls(tmp_path,&buff,file_type);
            }
            closedir(dir);
        }
    }
}



int main(int argc, char* argv[]) {
    int idx = argc;
    char* cwd;

    if(argc < 2){
        cwd = (char*)malloc(sizeof(char)*PATH_MAX);
        if (!getcwd(cwd, PATH_MAX)){ // getting the current path
            perror("getcwd()");
            free(cwd);
            exit(EXIT_FAILURE);
        }
        gettingReadyForLS(cwd);
        free(cwd);
        return 0;
    } // no arguments

    while(idx-- > 1){
        gettingReadyForLS(argv[idx]);
        printf("\n"); //printing a '\n' for separation of every argument
    }
    return 0;
}