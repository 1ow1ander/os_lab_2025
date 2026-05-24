/* gcc factorial.c -o factorial -pthread
 * ./factorial -k 10 --pnum=4 --mod=1000
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

long long global_result = 1;
int mod;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// для передачи данных в поток
typedef struct {
    int start;
    int end;
} Range;

void* compute_partial(void* arg) {
    Range* range = (Range*)arg;
    long long partial = 1;
    int i;
    
    // произведение чисел от start до end по модулю mod
    for (i = range->start; i <= range->end; i++) {
        partial = (partial * i) % mod;
    }
    
    pthread_mutex_lock(&mutex);
    global_result = (global_result * partial) % mod;
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    int k, pnum;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0) {
            k = atoi(argv[++i]);
        } else if (strncmp(argv[i], "--pnum=", 7) == 0) {
            pnum = atoi(argv[i] + 7);
        } else if (strncmp(argv[i], "--mod=", 6) == 0) {
            mod = atoi(argv[i] + 6);
        }
    }
    
    if (k == 0 || pnum == 0 || mod == 0) {
        printf("Использование: %s -k <число> --pnum=<число> --mod=<число>\n", argv[0]);
        return 1;
    }
    

    pthread_t threads[pnum];
    Range ranges[pnum];
    

    int total_numbers = k;
    int base_size = total_numbers / pnum;
    int remainder = total_numbers % pnum;
    
    int current_start = 1;
    for (int i = 0; i < pnum; i++) {
        int size = base_size;
        if (i < remainder) size++;
        int current_end = current_start + size - 1;
        
        ranges[i].start = current_start;
        ranges[i].end = current_end;
        
        current_start = current_end + 1;
    }
    
    for (int i = 0; i < pnum; i++) {
        if (pthread_create(&threads[i], NULL, compute_partial, &ranges[i]) != 0) {
            perror("pthread_create");
            exit(1);
        }
    }
    
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Факториал %d! по модулю %d = %lld\n", k, mod, global_result);
    
    return 0;
}