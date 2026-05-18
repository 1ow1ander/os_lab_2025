#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include "utils.h"
#include "sum_lib.h"

// gcc -o parallel_sum parallel_sum.c sum_lib.c utils.c -pthread -I.
// ./parallel_sum --threads_num 4 --seed 42 --array_size 1000000

struct ThreadArgs {
    struct SumArgs sum_args;
    int result;
};

void *ThreadSum(void *args) {
    struct ThreadArgs *thread_args = (struct ThreadArgs *)args;
    thread_args->result = Sum(&thread_args->sum_args);
    return NULL; // результат передастся через поле result
}

int main(int argc, char **argv) {
    int threads_num = 0;
    int array_size = 0;
    int seed = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--threads_num") == 0 && i+1 < argc) {
            threads_num = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--seed") == 0 && i+1 < argc) {
            seed = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--array_size") == 0 && i+1 < argc) {
            array_size = atoi(argv[++i]);
        }
    }

    if (threads_num <= 0 || array_size <= 0 || seed <= 0) {
        printf("Usage: %s --threads_num N --seed S --array_size A\n", argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    if (!array) {
        perror("malloc");
        return 1;
    }
    GenerateArray(array, array_size, seed);

    // потоки
    pthread_t threads[threads_num];
    struct ThreadArgs args[threads_num];

    // разбиваем массив на части
    int base = array_size / threads_num;
    int rem = array_size % threads_num;
    int start = 0;
    for (int i = 0; i < threads_num; i++) {
        int chunk = base + (i < rem ? 1 : 0);
        args[i].sum_args.array = array;
        args[i].sum_args.begin = start;
        args[i].sum_args.end = start + chunk;
        start += chunk;
    }

    // засекаем время
    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // создание потоков
    for (int i = 0; i < threads_num; i++) {
        if (pthread_create(&threads[i], NULL, ThreadSum, &args[i]) != 0) {
            printf("Error: pthread_create failed for thread %d\n", i);
            free(array);
            return 1;
        }
    }

    // ожидание завершения потоков и сумма результатов
    int total_sum = 0;
    for (int i = 0; i < threads_num; i++) {
        pthread_join(threads[i], NULL);
        total_sum += args[i].result;
    }

    // время
    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);
    double elapsed_ms = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_ms += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    printf("Total sum: %d\n", total_sum);
    printf("Elapsed time: %f ms\n", elapsed_ms);
    return 0;
}