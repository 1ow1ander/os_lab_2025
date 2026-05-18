#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <seed> <array_size>\n", argv[0]);
        return 1;
    }

    char *seed = argv[1];
    char *array_size = argv[2];

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // дочерний процесс
        // execlp ищет программу в PATH
        execlp("./sequential_min_max", "sequential_min_max", seed, array_size, NULL);
        // Если execlp вернулся, значит ошибка
        perror("exec failed");
        exit(1);
    } else {
        // родитель ждёт завершения дочернего процесса
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Code_4: child exited with code %d\n", WEXITSTATUS(status));
        } else {
            printf("Code_4: child terminated abnormally\n");
        }
    }

    return 0;
}