#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "manager.h"
#include "collector.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory> [max_agents]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *directory = argv[1];
    int max_agents = 4;

    if (argc >= 3) {
        max_agents = atoi(argv[2]);
        if (max_agents <= 0) {
            fprintf(stderr, "Invalid max_agents value. Must be > 0.\n");
            exit(EXIT_FAILURE);
        }
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t collector_pid = fork();
    if (collector_pid == -1) {
        perror("fork collector");
        exit(EXIT_FAILURE);
    }

    if (collector_pid == 0) {
        run_collector(pipefd[0], pipefd[1]);
        exit(EXIT_SUCCESS);
    } else {
        close(pipefd[0]);

        run_manager(directory, max_agents, pipefd[1]);

        close(pipefd[1]);

        if (waitpid(collector_pid, NULL, 0) == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }

    return EXIT_SUCCESS;
}
