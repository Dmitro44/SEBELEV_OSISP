#include "agent.h"

#include "signature.h"

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void run_agent(const char* dirpath, int write_fd, int read_fd_to_close) {
    if (read_fd_to_close >= 0) {
        close(read_fd_to_close);
    }

    DIR* dir = opendir(dirpath);
    if (dir == NULL) {
        close(write_fd);
        exit(EXIT_SUCCESS);
    }

    struct dirent* entry = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        size_t path_len = strlen(dirpath) + 1 + strlen(entry->d_name) + 1;
        char* full_path = malloc(path_len);
        if (full_path == NULL) {
            continue;
        }

        snprintf(full_path, path_len, "%s/%s", dirpath, entry->d_name);

        struct stat st;
        if (lstat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
            unsigned char signature = calculate_xor_signature(full_path);
            long timestamp = (long)st.st_mtime;
            pid_t agent_pid = getpid();

            dprintf(write_fd, "%s-%u-%ld-%d\n", full_path, signature, timestamp, (int)agent_pid);
        }

        free(full_path);
    }

    closedir(dir);
    close(write_fd);
    exit(EXIT_SUCCESS);
}
