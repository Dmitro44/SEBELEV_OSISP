#include "dictionary.h"
#include <ctype.h>
#include <string.h>
#include <strings.h>

#define INITIAL_CAPACITY 16

Dictionary *dict_create(void) {
    Dictionary *dict = malloc(sizeof(Dictionary));
    if (!dict)
        return NULL;
    dict->capacity = INITIAL_CAPACITY;
    dict->count = 0;
    dict->words = malloc(dict->capacity * sizeof(char *));
    if (!dict->words) {
        free(dict);
        return NULL;
    }
    return dict;
}

void dict_load(Dictionary *dict, FILE *f) {
    if (!dict || !f)
        return;

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), f)) {
        size_t len = strlen(buffer);
        while (len > 0 && isspace((unsigned char)buffer[len - 1])) {
            buffer[len - 1] = '\0';
            len--;
        }

        if (buffer[0] == '\0')
            continue;

        if (dict->count >= dict->capacity) {
            dict->capacity *= 2;
            char **new_words =
                realloc(dict->words, dict->capacity * sizeof(char *));
            if (!new_words)
                return;
            dict->words = new_words;
        }

        dict->words[dict->count] = strdup(buffer);
        if (dict->words[dict->count]) {
            dict->count++;
        }
    }
}

bool dict_is_forbidden(Dictionary *dict, const char *word) {
    if (!dict || !word)
        return false;

    for (size_t i = 0; i < dict->count; i++) {
        if (strcasecmp(dict->words[i], word) == 0) {
            return true;
        }
    }
    return false;
}

void dict_free(Dictionary *dict) {
    if (!dict)
        return;
    for (size_t i = 0; i < dict->count; i++) {
        free(dict->words[i]);
    }
    free(dict->words);
    free(dict);
}
