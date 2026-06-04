#include "manager.h"

#include "agent.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void wait_for_one_child(int* active_agents) {
    int status = 0;
    pid_t pid = -1;

    do {
        pid = waitpid(-1, &status, 0);
    } while (pid == -1 && errno == EINTR);

    if (pid > 0) {
        (*active_agents)--;
        return;
    }

    if (pid == -1 && errno == ECHILD) {
        *active_agents = 0;
    }
}

static void traverse_dir(const char* dirpath, int max_agents, int write_fd, int* active_agents) {
    while (*active_agents >= max_agents) {
        wait_for_one_child(active_agents);
    }

    (*active_agents)++;
    pid_t pid = fork();
    if (pid == 0) {
        run_agent(dirpath, write_fd, -1);
        _exit(EXIT_FAILURE);
    }

    if (pid < 0) {
        (*active_agents)--;
        return;
    }

    DIR* dir = opendir(dirpath);
    if (dir == NULL) {
        return;
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
        if (lstat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            traverse_dir(full_path, max_agents, write_fd, active_agents);
        }

        free(full_path);
    }

    closedir(dir);
}

void run_manager(const char* root_dir, int max_agents, int write_fd) {
    if (root_dir == NULL) {
        return;
    }

    if (max_agents < 1) {
        max_agents = 1;
    }

    int active_agents = 0;
    traverse_dir(root_dir, max_agents, write_fd, &active_agents);

    while (active_agents > 0) {
        wait_for_one_child(&active_agents);
    }
}
