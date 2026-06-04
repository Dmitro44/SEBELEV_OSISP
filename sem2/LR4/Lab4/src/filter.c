#include "filter.h"
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

char *filter_line(const char *line, Dictionary *dict) {
    if (!line || !dict)
        return NULL;

    size_t len = strlen(line);
    size_t cap = len * 3 + 1;
    char *result = malloc(cap);
    if (!result)
        return NULL;

    size_t i = 0;
    size_t res_i = 0;

    while (i < len) {
        if (isalpha((unsigned char)line[i])) {
            size_t word_start = i;
            while (i < len && isalpha((unsigned char)line[i])) {
                i++;
            }
            size_t word_len = i - word_start;

            char *word = malloc(word_len + 1);
            if (!word) {
                free(result);
                return NULL;
            }
            strncpy(word, line + word_start, word_len);
            word[word_len] = '\0';

            if (dict_is_forbidden(dict, word)) {
                for (size_t j = 0; j < word_len; j++) {
                    result[res_i++] = '*';
                }
            } else {
                for (size_t j = 0; j < word_len; j++) {
                    result[res_i++] = word[j];
                }
            }
            free(word);
        } else {
            result[res_i++] = line[i++];
        }
    }
    result[res_i] = '\0';

    char *shrinked = realloc(result, res_i + 1);
    if (shrinked) {
        result = shrinked;
    }
    return result;
}
