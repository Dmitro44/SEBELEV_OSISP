#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

typedef struct {
    int *array;
    int start;
    int end;
} sort_args_t;

int compare_ints(const void *a, const void *b) {
    return (*(int *)a - *(int *)b);
}

void *sort_fragment(void *arg) {
    sort_args_t *args = (sort_args_t *)arg;
    qsort(args->array + args->start, args->end - args->start, sizeof(int), compare_ints);
    return NULL;
}

void merge_sorted_fragments(int *array, int size, int threads_count) {
    int *temp = malloc(size * sizeof(int));
    int *indices = calloc(threads_count, sizeof(int));
    int *chunk_ends = malloc(threads_count * sizeof(int));
    
    int chunk_size = size / threads_count;
    for (int i = 0; i < threads_count; i++) {
        indices[i] = i * chunk_size;
        chunk_ends[i] = (i == threads_count - 1) ? size : (i + 1) * chunk_size;
    }

    for (int i = 0; i < size; i++) {
        int min_val = 0;
        int min_idx = -1;

        for (int j = 0; j < threads_count; j++) {
            if (indices[j] < chunk_ends[j]) {
                if (min_idx == -1 || array[indices[j]] < min_val) {
                    min_val = array[indices[j]];
                    min_idx = j;
                }
            }
        }
        temp[i] = min_val;
        indices[min_idx]++;
    }

    memcpy(array, temp, size * sizeof(int));
    free(temp);
    free(indices);
    free(chunk_ends);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <array_size> <threads_count>\n", argv[0]);
        return 1;
    }

    int size = atoi(argv[1]);
    int threads_count = atoi(argv[2]);

    if (size <= 0 || threads_count <= 0) {
        printf("Invalid arguments\n");
        return 1;
    }

    int *array = malloc(size * sizeof(int));
    srand(time(NULL));
    for (int i = 0; i < size; i++) {
        array[i] = rand() % 1000000;
    }

    pthread_t *threads = malloc(threads_count * sizeof(pthread_t));
    sort_args_t *args = malloc(threads_count * sizeof(sort_args_t));

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int chunk_size = size / threads_count;
    for (int i = 0; i < threads_count; i++) {
        args[i].array = array;
        args[i].start = i * chunk_size;
        args[i].end = (i == threads_count - 1) ? size : (i + 1) * chunk_size;
        
        if (pthread_create(&threads[i], NULL, sort_fragment, &args[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    for (int i = 0; i < threads_count; i++) {
        pthread_join(threads[i], NULL);
    }

    merge_sorted_fragments(array, size, threads_count);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("Array size: %d\n", size);
    printf("Threads: %d\n", threads_count);
    printf("Time elapsed: %.6f seconds\n", elapsed);

    int sorted = 1;
    for (int i = 0; i < size - 1; i++) {
        if (array[i] > array[i + 1]) {
            sorted = 0;
            break;
        }
    }
    printf("Result: %s\n", sorted ? "SUCCESS (Sorted)" : "FAILURE (Not sorted)");

    free(array);
    free(threads);
    free(args);

    return 0;
}
