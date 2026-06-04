#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    char input[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return EXIT_FAILURE;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address / Address not supported");
        close(sock);
        return EXIT_FAILURE;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("Connected to server %s:%d\n", ip, port);

    while (1) {
        printf("> ");
        fflush(stdout);

        if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) continue;

        if (send(sock, input, strlen(input), 0) < 0) {
            perror("Send failed");
            break;
        }

        if (strcmp(input, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }

        memset(buffer, 0, BUFFER_SIZE);
        while (1) {
            int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received <= 0) break;

            int end_found = 0;
            if (buffer[bytes_received - 1] == '\0') {
                end_found = 1;
                bytes_received--; 
            }

            if (bytes_received > 0) {
                buffer[bytes_received] = '\0';
                printf("%s", buffer);
            }

            if (end_found) break;
            memset(buffer, 0, BUFFER_SIZE);
        }
        printf("\n");
    }

    close(sock);
    return EXIT_SUCCESS;
}
