#include "collector.h"

#include <stdio.h>
#include <unistd.h>

void run_collector(int read_fd, int write_fd_to_close) {
    close(write_fd_to_close);

    FILE* stream = fdopen(read_fd, "r");
    if (stream == NULL) {
        return;
    }

    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), stream) != NULL) {
        fputs(buffer, stdout);
    }

    fclose(stream);
}
