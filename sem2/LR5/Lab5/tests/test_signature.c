#include <stdio.h>
#include <stdlib.h>

#include "signature.h"

static int check_signature(const char* path, unsigned char expected) {
    unsigned char actual = calculate_xor_signature(path);
    if (actual != expected) {
        fprintf(stderr, "expected 0x%02X for %s, got 0x%02X\n", expected, path, actual);
        return 1;
    }
    return 0;
}

int main(void) {
    int failures = 0;

    failures += check_signature("tests/mock_fs/dir1/file1.txt", 0x41);
    failures += check_signature("tests/mock_fs/dir1/file2.txt", 0x01);
    failures += check_signature("tests/mock_fs/root_file.txt", 0x26);

    if (failures != 0) {
        return EXIT_FAILURE;
    }

    puts("test_signature passed");
    return EXIT_SUCCESS;
}
