/* 
Yuvan Rengifo
I pledge my honor that I have abided by the Stevens Honor System.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

/* Helper to make sure Arg is okay */
void check_directory(char *directory) {
    DIR *d;
    struct stat st;
    int i = stat(directory, &st); /* Get directory status */

    if (i != 0) {
        /* If directory/file doesnt exist*/
        if (errno == ENOENT) {
            fprintf(stderr, "Permission denied. %s cannot be read.", directory);
        } 
        exit(1);
    }

    /* Check that its a directory, not a file */
    if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "The first argument has to be a directory.");
        exit(1);
    }

    /* Try to Open the directory*/
    if (!(d = opendir(directory))) {
        /* Permission error*/
        if (errno == EACCES) {
            fprintf(stderr, "Permission denied. %s cannot be read.", directory);
        }
        exit(1);
    }
    closedir(d);
}

int main(int argc, char *argv[]) {

    /* Extra arguments or no arguments given*/
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <DIRECTORY>\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    check_directory(argv[1]);   /* Make sure its a directory*/

    int pipe_ls_to_sort[2]; /*pipe ls to srot */ 
    int pipe_sort_to_parent[2]; /*pipe sort to parent */

    /* Pipe data */
    pipe(pipe_ls_to_sort);
    pipe(pipe_sort_to_parent);

    if (fork() == 0) { /* Child 1: ls */
        dup2(pipe_ls_to_sort[1], STDOUT_FILENO);

        close(pipe_ls_to_sort[0]);
        close(pipe_sort_to_parent[0]);
        close(pipe_sort_to_parent[1]);

        execlp("ls", "ls", "-ai", argv[1], NULL); /* No return since process image is replaced, unless it fails*/

        fprintf(stderr, "Error: ls failed.\n");
        exit(EXIT_FAILURE);
    }

    if (fork() == 0) { /* Child 2: sort */
        dup2(pipe_ls_to_sort[0], STDIN_FILENO);
        dup2(pipe_sort_to_parent[1], STDOUT_FILENO);

        close(pipe_ls_to_sort[1]);
        close(pipe_sort_to_parent[0]);

        execlp("sort", "sort", NULL);

        fprintf(stderr, "Error: sort failed.\n");
        exit(EXIT_FAILURE);
    }

    /* Parent */
    close(pipe_ls_to_sort[0]);
    close(pipe_ls_to_sort[1]);
    close(pipe_sort_to_parent[1]);

    char buffer[1024];
    int filenum = 0;

    FILE *fd = fdopen(pipe_sort_to_parent[0], "r");
    
    while (fgets(buffer, sizeof(buffer), fd) != NULL) {
        filenum++;
        fputs(buffer, stdout);
    }
    fclose(fd);
    
    wait(NULL); /* Wait for child*/
    wait(NULL);

    printf("Total files: %d\n", filenum);
    return EXIT_SUCCESS;
}
