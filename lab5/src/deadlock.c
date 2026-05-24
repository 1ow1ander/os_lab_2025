/*
 * gcc deadlock.c -o deadlock -pthread
 * ./deadlock
 * ctrl+C
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t mutexA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexB = PTHREAD_MUTEX_INITIALIZER;


void* thread1_func(void* arg) {
    printf("Поток 1: пытаюсь захватить mutex A\n");
    pthread_mutex_lock(&mutexA);
    printf("Поток 1: захватил mutex A\n");
    
    usleep(10000);
    
    printf("Поток 1: пытаюсь захватить mutex B\n");
    pthread_mutex_lock(&mutexB);
    printf("Поток 1: захватил mutex B (успех!)\n");
    
    pthread_mutex_unlock(&mutexB);
    pthread_mutex_unlock(&mutexA);
    
    printf("Поток 1: завершился\n");
    return NULL;
}

void* thread2_func(void* arg) {
    printf("Поток 2: пытаюсь захватить mutex B\n");
    pthread_mutex_lock(&mutexB);
    printf("Поток 2: захватил mutex B\n");
    
    usleep(10000);
    
    printf("Поток 2: пытаюсь захватить mutex A\n");
    pthread_mutex_lock(&mutexA);
    printf("Поток 2: захватил mutex A (успех!)\n");
    
    pthread_mutex_unlock(&mutexA);
    pthread_mutex_unlock(&mutexB);
    
    printf("Поток 2: завершился\n");
    return NULL;
}

int main() {
    pthread_t t1, t2;
    
    if (pthread_create(&t1, NULL, thread1_func, NULL) != 0) {
        perror("pthread_create t1");
        exit(1);
    }
    if (pthread_create(&t2, NULL, thread2_func, NULL) != 0) {
        perror("pthread_create t2");
        exit(1);
    }
    
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    
    printf("Оба потока завершились (эта строка не напечатается при deadlock)\n");
    return 0;
}