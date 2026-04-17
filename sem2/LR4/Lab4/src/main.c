#include "dictionary.h"
#include "filter.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int opt;
    char *dict_file = NULL;
    char *input_file = NULL;
    char *output_file = NULL;

    while ((opt = getopt(argc, argv, "d:i:o:")) != -1) {
        switch (opt) {
        case 'd':
            dict_file = optarg;
            break;
        case 'i':
            input_file = optarg;
            break;
        case 'o':
            output_file = optarg;
            break;
        default:
            fprintf(stderr,
                    "Usage: %s -d dict.txt [-i input.txt] [-o output.txt]\n",
                    argv[0]);
            return 1;
        }
    }

    if (!dict_file) {
        fprintf(stderr, "Error: Dictionary file (-d) is required.\n");
        return 1;
    }

    FILE *df = fopen(dict_file, "r");
    if (!df) {
        perror("Failed to open dictionary file");
        return 1;
    }

    Dictionary *dict = dict_create();
    if (!dict) {
        fprintf(stderr, "Failed to create dictionary.\n");
        fclose(df);
        return 1;
    }

    dict_load(dict, df);
    fclose(df);

    FILE *in = stdin;
    if (input_file) {
        in = fopen(input_file, "r");
        if (!in) {
            perror("Failed to open input file");
            dict_free(dict);
            return 1;
        }
    }

    FILE *out = stdout;
    if (output_file) {
        out = fopen(output_file, "w");
        if (!out) {
            perror("Failed to open output file");
            if (in != stdin)
                fclose(in);
            dict_free(dict);
            return 1;
        }
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read_bytes;

    while ((read_bytes = getline(&line, &len, in)) != -1) {
        char *filtered = filter_line(line, dict);
        if (filtered) {
            fputs(filtered, out);
            free(filtered);
        }
    }

    free(line);

    if (in != stdin)
        fclose(in);
    if (out != stdout)
        fclose(out);
    dict_free(dict);

    return 0;
}
