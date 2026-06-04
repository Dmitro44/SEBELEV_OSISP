#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    char **words;
    size_t count;
    size_t capacity;
} Dictionary;

Dictionary *dict_create(void);
void dict_load(Dictionary *dict, FILE *f);
bool dict_is_forbidden(Dictionary *dict, const char *word);
void dict_free(Dictionary *dict);

#endif
