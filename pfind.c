#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* Yuvan Rengifo*/
/* I pledge my honor that i have abided by the Stevens Honor system*/


// Validate the permissions string
int VpermissionsStr(const char *str) {

    for (int i = 0; i < 9; ++i) {   // Iterate each char and divide the sections to match rwx
        if (i % 3 == 0 && str[i] != 'r' && str[i] != '-') 
        return 0; // R

        if (i % 3 == 1 && str[i] != 'w' && str[i] != '-')
         return 0; // W

        if (i % 3 == 2 && str[i] != 'x' && str[i] != '-') 
        return 0; // X

    }
    return 1; 
}


void FindFiles(const char *path, const char *pstr) {
    DIR *dir = opendir(path);
    
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        // Ignore the current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path
        char full_path[PATH_MAX];

        if (path[strlen(path) - 1] == '/') {
            // don't add another slash
            snprintf(full_path, sizeof(full_path), "%s%s", path, entry->d_name);
        } 
        else {
            // add a slash between the base path and the entry name
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        }

        struct stat entry_stat;

        if (stat(full_path, &entry_stat) == -1) {
          
            continue;
        }

        // Encode file types and permissions
        if (S_ISREG(entry_stat.st_mode)) {
            //buffer
            char mode_str[10];
            snprintf(mode_str, sizeof(mode_str), "%c%c%c%c%c%c%c%c%c",
                     (entry_stat.st_mode & S_IRUSR) ? 'r' : '-',
                     (entry_stat.st_mode & S_IWUSR) ? 'w' : '-',
                     (entry_stat.st_mode & S_IXUSR) ? 'x' : '-',
                     (entry_stat.st_mode & S_IRGRP) ? 'r' : '-',
                     (entry_stat.st_mode & S_IWGRP) ? 'w' : '-',
                     (entry_stat.st_mode & S_IXGRP) ? 'x' : '-',
                     (entry_stat.st_mode & S_IROTH) ? 'r' : '-',
                     (entry_stat.st_mode & S_IWOTH) ? 'w' : '-',
                     (entry_stat.st_mode & S_IXOTH) ? 'x' : '-');

            if (strcmp(mode_str, pstr) == 0) {
                // Print the path without unnecessary slashes
                printf("%s\n", full_path);
            }
        } else if (S_ISDIR(entry_stat.st_mode)) {
            // Recursively search directory
            FindFiles(full_path, pstr);
        }
    }

    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <directory> <pstring>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Assign args
    const char *directory = argv[1];
    const char *pstring = argv[2];

    if (!VpermissionsStr(pstring)) {
        fprintf(stderr, "Error: Permissions string '%s' is invalid.\n", pstring);
        return EXIT_FAILURE;
    }

    // Search for the validated permissions in directory
    FindFiles(directory, pstring);

    return EXIT_SUCCESS;
}
