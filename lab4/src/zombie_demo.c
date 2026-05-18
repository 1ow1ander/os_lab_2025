#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// gcc -o zombie_demo zombie_demo.c
// ./zombie_demo
// в другом терминале ps aux | grep Z

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // дочерний процесс
        printf("Child process (PID %d) is running. It will exit now.\n", getpid());
        exit(0);  // завершается и становится зомби
    } else {
        // родительский процесс
        printf("Parent process (PID %d) is sleeping for 30 seconds.\n", getpid());
        printf("During this time, run: ps aux | grep Z\n");
        printf("You will see the zombie process (PID %d).\n", pid);
        
        // родитель спит 30 секунд и не вызывает wait()
        // 30 сек ребёнок будет зомби
        sleep(30);

        printf("Parent wakes up and now calls wait() to clean the zombie.\n");
        wait(NULL);   // родитель вызывает wait
        printf("Zombie cleaned. Check again: no zombie for PID %d\n", pid);
    }
    return 0;
}