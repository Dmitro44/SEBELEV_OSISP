#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_CMD_LEN 1024
#define MAX_OUT_LEN 4096

void sigchld_handler(int s) {
    (void)s;
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void handle_client(int client_fd) {
    char cmd_buffer[MAX_CMD_LEN];
    char out_buffer[MAX_OUT_LEN];
    ssize_t bytes_received;

    while (1) {
        memset(cmd_buffer, 0, sizeof(cmd_buffer));
        bytes_received = recv(client_fd, cmd_buffer, sizeof(cmd_buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            break;
        }

        cmd_buffer[strcspn(cmd_buffer, "\r\n")] = 0;

        if (strlen(cmd_buffer) == 0) {
            continue;
        }

        if (strcmp(cmd_buffer, "exit") == 0) {
            break;
        }

        char cmd_with_stderr[MAX_CMD_LEN + 8];
        snprintf(cmd_with_stderr, sizeof(cmd_with_stderr), "%s 2>&1", cmd_buffer);

        FILE *fp = popen(cmd_with_stderr, "r");
        if (fp == NULL) {
            const char *err_msg = "Failed to run command\n";
            send(client_fd, err_msg, strlen(err_msg), 0);
            continue;
        }

        int output_sent = 0;
        while (fgets(out_buffer, sizeof(out_buffer), fp) != NULL) {
            send(client_fd, out_buffer, strlen(out_buffer), 0);
            output_sent = 1;
        }
        
        send(client_fd, "\0", 1, 0);
        
        if (!output_sent) {
            const char *no_out_msg = "(no output)\n";
            send(client_fd, no_out_msg, strlen(no_out_msg), 0);
        }

        pclose(fp);
    }

    close(client_fd);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct sigaction sa;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            perror("accept failed");
            continue;
        }

        printf("New connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid == -1) {
            perror("fork failed");
            close(client_fd);
        } else if (pid == 0) {
            close(server_fd);
            handle_client(client_fd);
        } else {
            close(client_fd);
        }
    }

    close(server_fd);
    return 0;
}