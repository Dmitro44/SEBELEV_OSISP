#import "lib/stp2024.typ"
#show: stp2024.template

#include "lab_title.typ"

#stp2024.full_outline()

= Постановка задачи

Разработать программу на языке #emph("C"), реализующую фильтр автоматической цензуры. Программа должна читать входной поток данных (текст) и заменять все вхождения запрещенных слов на последовательность символов «\*\*\*». Список запрещенных слов должен загружаться из внешнего текстового файла (словаря). Программа должна поддерживать работу как со стандартными потоками ввода/вывода (#emph("stdin"), #emph("stdout")), так и с файлами, указанными через аргументы командной строки.

= Выполнение работы

== Архитектура программы

Программа реализована на языке #emph("C") стандарта #emph("C18") @c18_standard.
 Архитектура построена по модульному принципу и включает следующие компоненты:

- Модуль #emph("dictionary") отвечает за динамическое управление списком запрещенных слов. Словарь реализован в виде структуры с динамическим массивом строк для обеспечения эффективного хранения и поиска.
- Модуль #emph("filter") содержит логику обработки текста. Выполняет разбиение строки на слова, проверку каждого слова по словарю и формирование отфильтрованного результата.
- Главный модуль #emph("main") координирует работу программы, обрабатывает аргументы командной строки и организует цикл чтения данных.

== Обработка аргументов командной строки

Для разбора параметров запуска используется стандартная функция #emph("getopt") @getopt_man. Программа поддерживает следующие флаги:

- #emph("-d <path>") -- путь к файлу словаря (обязательный параметр);
- #emph("-i <path>") -- путь к входному текстовому файлу (по умолчанию #emph("stdin"));
- #emph("-o <path>") -- путь к выходному файлу (по умолчанию #emph("stdout")).

При отсутствии флага #emph("-d") или невозможности открыть указанные файлы программа выводит соответствующее сообщение об ошибке в поток #emph("stderr") и завершает работу с ненулевым кодом статуса.

== Алгоритм фильтрации и работа с памятью

Загрузка словаря выполняется построчно. Каждое слово из файла очищается от символов переноса строки и сохраняется в динамической памяти. Поиск в словаре осуществляется без учета регистра символов с помощью функции #emph("strcasecmp").

Обработка основного текста выполняется построчно с использованием функции #emph("getline") @getline_man, что позволяет эффективно работать со строками произвольной длины. Функция фильтрации идентифицирует слова как последовательности алфавитных символов (#emph("isalpha")). Знаки препинания, пробелы и цифры сохраняются без изменений. Для каждой обработанной строки выделяется новый буфер памяти, который освобождается после вывода результата.

== Сборка и тестирование

Сборка проекта осуществляется с помощью утилиты #emph("make"). В #emph("Makefile") определены цели для компиляции исполняемого файла и очистки проекта.

Тестирование работоспособности программы проводилось вручную на различных наборах входных данных, включая передачу аргументов командной строки для работы с файлами и использование стандартных потоков ввода/вывода. Программа корректно обрабатывает пустые файлы, длинные строки текста и сложные случаи пунктуации.

#pagebreak()

#stp2024.heading_unnumbered[Вывод]

В ходе выполнения лабораторной работы была разработана программа фильтрации текста на языке #emph("C"), реализующая механизм автоматической цензуры. В процессе реализации были закреплены навыки работы с динамическими структурами данных, системными вызовами для работы с файловой системой и средствами обработки аргументов командной строки. Особое внимание было уделено вопросам безопасного управления памятью и тестирования программного обеспечения в среде #emph("Linux"). Программа демонстрирует стабильность работы и корректную обработку текстовых данных.

#bibliography("bibliography.bib")

#stp2024.appendix(title: [Листинг программного кода], type: [обязательное],
[
  #stp2024.listing[main.c][
```
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
```
  ]
  
  #stp2024.listing[filter.c][
```
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
```
  ]
]
)
