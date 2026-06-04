#include "signature.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

unsigned char calculate_xor_signature(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (file == NULL) {
        fprintf(stderr, "failed to open %s: %s\n", filepath, strerror(errno));
        return 0;
    }

    unsigned char signature = 0;
    unsigned char buffer[4096];
    size_t bytes_read = 0;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i < bytes_read; ++i) {
            signature ^= buffer[i];
        }
    }

    if (ferror(file)) {
        fprintf(stderr, "failed to read %s\n", filepath);
        signature = 0;
    }

    fclose(file);
    return signature;
}
