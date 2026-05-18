#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <getopt.h>
#include <signal.h>
#include <errno.h>
#include "find_min_max.h"
#include "utils.h"

// флаг для обработчика сигнала
static volatile sig_atomic_t timeout_flag = 0;

// обработчик сигнала SIGALRM
void alarm_handler(int sig) {
    timeout_flag = 1;   // таймаут наступил
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int pnum = -1;
    bool with_files = false;
    int timeout = -1;   // по умолчанию таймаута нет

    while (true) {
        static struct option options[] = {
            {"seed",       required_argument, 0, 0},
            {"array_size", required_argument, 0, 0},
            {"pnum",       required_argument, 0, 0},
            {"by_files",   no_argument,      0, 'f'},
            {"timeout",    required_argument, 0, 0},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, "f", options, &option_index);
        if (c == -1) break;

        switch (c) {
            case 0:
                switch (option_index) {
                    case 0: // seed
                        seed = atoi(optarg);
                        if (seed <= 0) {
                            printf("seed must be positive\n");
                            return 1;
                        }
                        break;
                    case 1: // array_size
                        array_size = atoi(optarg);
                        if (array_size <= 0) {
                            printf("array_size must be positive\n");
                            return 1;
                        }
                        break;
                    case 2: // pnum
                        pnum = atoi(optarg);
                        if (pnum <= 0) {
                            printf("pnum must be positive\n");
                            return 1;
                        }
                        break;
                    case 3: // by_files
                        with_files = true;
                        break;
                    case 4: // timeout
                        timeout = atoi(optarg);
                        if (timeout <= 0) {
                            printf("timeout must be positive\n");
                            return 1;
                        }
                        break;
                    default:
                        printf("Index %d is out of options\n", option_index);
                }
                break;
            case 'f':
                with_files = true;
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o\n", c);
        }
    }

    if (optind < argc) {
        printf("Has at least one no-option argument\n");
        return 1;
    }

    if (seed == -1 || array_size == -1 || pnum == -1) {
        printf("Usage: %s --seed num --array_size num --pnum num [--by_files] [--timeout num]\n", argv[0]);
        return 1;
    }

    int *array = malloc(sizeof(int) * array_size);
    if (array == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }
    GenerateArray(array, array_size, seed);

    // массивы для хранения pid дочерних процессов
    pid_t *child_pids = malloc(sizeof(pid_t) * pnum);
    if (child_pids == NULL) {
        perror("malloc child_pids");
        free(array);
        return 1;
    }

    // для pipe и файлов
    int *pipe_read_fds = NULL;
    char **filenames = NULL;

    if (!with_files) {
        pipe_read_fds = malloc(sizeof(int) * pnum);
        if (pipe_read_fds == NULL) {
            perror("malloc pipe_read_fds");
            free(array);
            free(child_pids);
            return 1;
        }
    } else {
        filenames = malloc(sizeof(char*) * pnum);
        if (filenames == NULL) {
            perror("malloc filenames");
            free(array);
            free(child_pids);
            return 1;
        }
        for (int i = 0; i < pnum; i++) filenames[i] = NULL;
    }

    // разбиение массива на части
    int base_size = array_size / pnum;
    int remainder = array_size % pnum;
    int current_start = 0;

    struct timeval start_time;
    gettimeofday(&start_time, NULL);

    // создание дочерних процессов
    for (int i = 0; i < pnum; i++) {
        int chunk_size = base_size + (i < remainder ? 1 : 0);
        int begin = current_start;
        int end = current_start + chunk_size;
        current_start = end;

        if (!with_files) {
            // pipe
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                free(array);
                free(child_pids);
                free(pipe_read_fds);
                return 1;
            }
            pipe_read_fds[i] = pipefd[0];
            int write_fd = pipefd[1];

            pid_t child_pid = fork();
            if (child_pid < 0) {
                perror("fork");
                free(array);
                free(child_pids);
                free(pipe_read_fds);
                return 1;
            }

            if (child_pid == 0) {
                // дочерний
                close(pipefd[0]);
                struct MinMax local = GetMinMax(array, begin, end);
                if (write(write_fd, &local, sizeof(struct MinMax)) != sizeof(struct MinMax)) {
                    perror("write to pipe");
                    exit(1);
                }
                close(write_fd);
                free(array);
                exit(0);
            } else {
                // родитель
                child_pids[i] = child_pid;
                close(write_fd);
            }
        } else {
            // файлы
            char template[] = "/tmp/minmax_XXXXXX";
            int fd = mkstemp(template);
            if (fd == -1) {
                perror("mkstemp");
                free(array);
                free(child_pids);
                for (int j = 0; j < i; j++) free(filenames[j]);
                free(filenames);
                return 1;
            }
            close(fd);
            char *fname = strdup(template);
            if (!fname) {
                perror("strdup");
                free(array);
                free(child_pids);
                for (int j = 0; j < i; j++) free(filenames[j]);
                free(filenames);
                return 1;
            }
            filenames[i] = fname;

            pid_t child_pid = fork();
            if (child_pid < 0) {
                perror("fork");
                free(array);
                free(child_pids);
                for (int j = 0; j <= i; j++) free(filenames[j]);
                free(filenames);
                return 1;
            }

            if (child_pid == 0) {
                // дочерний
                FILE *file = fopen(fname, "w");
                if (!file) {
                    perror("fopen");
                    exit(1);
                }
                struct MinMax local = GetMinMax(array, begin, end);
                fprintf(file, "%d %d", local.min, local.max);
                fclose(file);
                free(array);
                exit(0);
            } else {
                child_pids[i] = child_pid;
            }
        }
    }

    // если задан таймаут, устанавливаем alarm
    if (timeout > 0) {
        signal(SIGALRM, alarm_handler);
        alarm(timeout);
    }

    // ожидание завершения детей с учётом таймаута
    int active = pnum;
    while (active > 0) {
        int status;
        pid_t pid = wait(&status);
        if (pid == -1) {
            if (errno == EINTR && timeout_flag) {
                // таймаут наступил
                break;
            }
            perror("wait");
            continue;
        }
        active--;
    }

    // если таймаут наступил, убиваем всех детей и выходим
    if (timeout_flag) {
        printf("Timeout (%d seconds) expired, killing all children.\n", timeout);
        for (int i = 0; i < pnum; i++) {
            kill(child_pids[i], SIGKILL);
        }
        // чтобы не осталось зомби
        while (wait(NULL) > 0);
        // выход с ошибкой
        free(array);
        free(child_pids);
        if (!with_files) free(pipe_read_fds);
        else {
            for (int i = 0; i < pnum; i++) free(filenames[i]);
            free(filenames);
        }
        return 1;
    }

    // если таймаута не было, собрать результаты
    struct MinMax min_max;
    min_max.min = INT_MAX;
    min_max.max = INT_MIN;

    for (int i = 0; i < pnum; i++) {
        int min = INT_MAX, max = INT_MIN;
        if (!with_files) {
            // чтение из pipe
            struct MinMax local;
            if (read(pipe_read_fds[i], &local, sizeof(struct MinMax)) != sizeof(struct MinMax)) {
                perror("read from pipe");
                close(pipe_read_fds[i]);
                continue;
            }
            close(pipe_read_fds[i]);
            min = local.min;
            max = local.max;
        } else {
            // чтение из файла
            FILE *file = fopen(filenames[i], "r");
            if (!file) {
                perror("fopen read");
                continue;
            }
            if (fscanf(file, "%d %d", &min, &max) != 2) {
                fprintf(stderr, "Failed to read from %s\n", filenames[i]);
            }
            fclose(file);
            remove(filenames[i]);
            free(filenames[i]);
        }
        if (min < min_max.min) min_max.min = min;
        if (max > min_max.max) min_max.max = max;
    }

    struct timeval finish_time;
    gettimeofday(&finish_time, NULL);
    double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);
    free(child_pids);
    if (!with_files) free(pipe_read_fds);
    else free(filenames);

    printf("Min: %d\n", min_max.min);
    printf("Max: %d\n", min_max.max);
    printf("Elapsed time: %fms\n", elapsed_time);
    return 0;
}