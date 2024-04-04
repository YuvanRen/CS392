/*
    Yuvan Rengifo 
    Cs 392
    Minishell
    I pledge my honor that I have abided by the Stevens Honor System

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>

#define BLUE "\x1b[34;1m"
#define GREEN "\x1b[32;1m"        /* For printing directories */
#define RED "\x1b[38;2;128;0;0m"  /* For errors */
#define DEFAULT "\x1b[0m"

void createMini() {
    char cwd[1024];     /* Didnt use Malloc() */
    getcwd(cwd, sizeof(cwd));
    printf("%s[%s]> %s", BLUE, cwd, DEFAULT);   /* Prompt for minishell*/
}

void do_cmd(char *cmd, char **args) {
    char currentDir[1024];  /* No Malloc()*/
/* Handle cd */
    if (strcmp(cmd, "cd") == 0) {

    char *path = args[1]; /* Set path to parsed arg*/

    if(args[1] == NULL){
        goto cont;
    }
    if(args[2] != NULL){
        fprintf(stderr, "%sError: Too many arguments passed to cd.\n", RED);
        return;
    }
cont:
    if (path == NULL || strcmp(path, "~") == 0) { /* If no other arguments or ~ */

        struct passwd *pw = getpwuid(getuid()); /* Get users database entry*/   

        /* Syscall Error handler*/
        if (pw == NULL) {
            fprintf(stderr, "%sError: Cannot get passwd entry. %s.\n",RED , strerror(errno));
            return;
        }

        /* Access home directory of the user */
        path = pw->pw_dir;
    }

    /* Couldnt change directory */
    if (chdir(path) != 0) {
        fprintf(stderr, "%sError: Cannot change directory to %s. %s.\n",RED, path, strerror(errno));
    }

/* Handle exit */
    } else if (strcmp(cmd, "exit") == 0) { 
        if(args[1] != NULL){
            fprintf(stderr, "%sError: Too many arguments passed to exit.\n", RED);
            return;
        }
        exit(EXIT_SUCCESS);

/* Handle pwd */
    } else if (strcmp(cmd, "pwd") == 0) {

        if(args[1] != NULL){
            fprintf(stderr, "%sError: Too many arguments passed to pwd.\n", RED);
            return;
        }
        getcwd(currentDir, sizeof(currentDir));

        /* Error handler */
        if(currentDir == NULL ){
            fprintf(stderr, "%sError: Cannot get current directory %s. \n", RED, strerror(errno));
            return;
        }
        printf("%s\n", currentDir);

/* Handle lf */
    }else if(strcmp(cmd, "lf") == 0){

        if(args[1] != NULL){
            fprintf(stderr, "%sError: Too many arguments passed to lf.\n", RED);
            return;
        }
        DIR *curr;
        curr = opendir(".");

        if(!curr){
        fprintf(stderr, "%sError: Cannot open current directory.\n", RED);
        return;
    }
        struct stat dirStat;   
        char Path[1024]; /* To change color of directories */


        struct dirent *dir;
        while ((dir = readdir(curr)) != NULL) {
            /* Construct path to file*/
            snprintf(Path, sizeof(Path), "./%s", dir->d_name);
            stat(Path, &dirStat);

            /* Dont print these files */
            if(strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0){ 
                continue;
            }
            /* No hidden files */
            else if (dir->d_name[0] == '.') {
                continue;

            /* Printing directories */
            }else if (S_ISDIR(dirStat.st_mode)) {
                printf("%s%s%s\n", GREEN, dir->d_name, DEFAULT);
        
           }else{
                printf("%s\n",dir->d_name);
            }
             
        }
    
/* Handle lp */
    }else if(strcmp(cmd, "lp") == 0){

    DIR *proc = opendir("/proc");
    struct dirent *dir;

    /* Couldnt open /proc */
    if (!proc) {
        fprintf(stderr, "%sError: Cannot open /proc directory.%s\n", RED, DEFAULT);
        return;
    }

    while ((dir = readdir(proc)) != NULL) {
        int pid = atoi(dir->d_name);

        if (pid <= 0) continue;

        /* To read from status */
        char status[256];
        char line[256];

        snprintf(status, sizeof(status), "/proc/%d/status", pid);
        FILE *statusFile = fopen(status, "r");

        uid_t uid = -1;

        if (statusFile) {
            while (fgets(line, sizeof(line), statusFile)) {
                if (strncmp(line, "Uid:", 4) == 0) {    /* Get uid*/
                    sscanf(line, "Uid:\t%u", &uid);
                    break;
                }
            }
            fclose(statusFile);
        }

        /* Skip cases*/
        if (uid == -1) continue; 
        struct passwd *pw = getpwuid(uid);
        if (!pw) continue;

        /* Do CommandLine */
        char cmdline[256] = {0};

        snprintf(status, sizeof(status), "/proc/%d/cmdline", pid);
        FILE *cmdlineFile = fopen(status, "r");
        if (cmdlineFile) {
            if (fgets(cmdline, sizeof(cmdline), cmdlineFile) != NULL) {
                // Replace null terminators with spaces for display
                for (int i = 0; cmdline[i] && i < sizeof(cmdline); ++i) {
                    if (cmdline[i] == '\0') cmdline[i] = ' ';
                }
            }
            fclose(cmdlineFile);
        }
        printf("%d %s %s\n", pid, pw->pw_name, cmdline);
    }

    closedir(proc);
    }else {
        pid_t pid = fork();

        /* Syscall Error handler*/
        if (pid == -1) {
            fprintf(stderr, "%sError: fork() failed. %s.\n", RED, strerror(errno));
            return;

        } else if (pid == 0) {
            
            /* Child Process */
            if (execvp(cmd, args) == -1) {
                fprintf(stderr, "%sError: exec() failed. %s.\n", RED,strerror(errno));

                exit(EXIT_FAILURE);
            }
        } else {
            /* Parent waits*/
            int status;

            if (waitpid(pid, &status, 0) == -1) {
                fprintf(stderr, "%sError: wait() failed. %s.\n", RED, strerror(errno));
            }
        }
    }
}


volatile sig_atomic_t interrupt = 0; /* Interrupt */

/* Got a SIGINT*/
void handler(int i) {
    interrupt = 1;
}

void setupSignal() {
    
    struct sigaction sa;
    
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);

    /* Restart system calls*/
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        fprintf(stderr, "%sError: Cannot register signal handler. %s.\n", RED, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int main() {
    char input[1024];
    char *args[64];
   
    
    while (1) {  /* Infinitetly run until exit is called */

        /* Avoid crashing from SIGINT*/
        if (interrupt) {
            printf("\n");
            interrupt = 0; 
        }
        createMini();

        if (!fgets(input, 1024, stdin)) {

            if (interrupt) { /* fgets() interrupt by SIGINT */
                interrupt = 0; /* Reset */
                continue;
            }
            fprintf(stderr, "%sError: Failed to read from stdin. %s.\n", RED, strerror(errno));
        }
        
        input[strlen(input) - 1] = '\0'; /* No new line */
        
        int i = 0;
        args[i] = strtok(input, " ");
        
        while (args[i] != NULL) {
            args[++i] = strtok(NULL, " ");
        }
        
        do_cmd(args[0], args);
    }
    
    return 0;
}