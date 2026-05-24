#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include "common.h"

// хранение информации об одном сервере
struct Server {
    char ip[255];
    int port;
};

// для передачи данных в поток клиента
struct ClientThreadArgs {
    struct Server server;   // какой сервер
    uint64_t begin;         // начало диапазона для него
    uint64_t end;           // конец диапазона
    uint64_t mod;           // модуль
    uint64_t *result;       // куда записать ответ от сервера
};

// преобразование строки в uint64_t
bool ConvertStringToUI64(const char *str, uint64_t *val) {
    char *end = NULL;
    unsigned long long i = strtoull(str, &end, 10);
    if (errno == ERANGE) {
        fprintf(stderr, "Out of uint64_t range: %s\n", str);
        return false;
    }
    if (errno != 0)
        return false;
    *val = i;
    return true;
}

// ф-я, которую будет выполнять каждый поток клиента
void* client_thread_func(void* arg) {
    struct ClientThreadArgs *args = (struct ClientThreadArgs*)arg;
    
    // получить IP-адрес сервера по имени
    struct hostent *hostname = gethostbyname(args->server.ip);
    if (hostname == NULL) {
        fprintf(stderr, "gethostbyname failed for %s\n", args->server.ip);
        *args->result = 0;  // ошибка
        return NULL;
    }
    
    // заполнить структуру sockaddr_in для сервера
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(args->server.port);
    server.sin_addr.s_addr = *((unsigned long*)hostname->h_addr);
    
    // создать сокет TCP
    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if (sck < 0) {
        fprintf(stderr, "Socket creation failed for %s:%d\n", args->server.ip, args->server.port);
        *args->result = 0;
        return NULL;
    }
    
    // подключение к серверу
    if (connect(sck, (struct sockaddr*)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Connection failed to %s:%d\n", args->server.ip, args->server.port);
        close(sck);
        *args->result = 0;
        return NULL;
    }
    
    // подготовить данные для отправки
    char task[sizeof(uint64_t) * 3];
    memcpy(task, &args->begin, sizeof(uint64_t));
    memcpy(task + sizeof(uint64_t), &args->end, sizeof(uint64_t));
    memcpy(task + 2 * sizeof(uint64_t), &args->mod, sizeof(uint64_t));
    
    // отправить задание серверу
    if (send(sck, task, sizeof(task), 0) < 0) {
        fprintf(stderr, "Send failed to %s:%d\n", args->server.ip, args->server.port);
        close(sck);
        *args->result = 0;
        return NULL;
    }
    
    // получить ответ
    char response[sizeof(uint64_t)];
    if (recv(sck, response, sizeof(response), 0) < 0) {
        fprintf(stderr, "Receive failed from %s:%d\n", args->server.ip, args->server.port);
        close(sck);
        *args->result = 0;
        return NULL;
    }
    
    // скопировать ответ в переданный указатель
    memcpy(args->result, response, sizeof(uint64_t));
    
    // закрыть сокет
    close(sck);
    return NULL;
}

// чтение файла со списком серверов
int read_servers(const char *filename, struct Server **servers) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return -1;
    }
    
    int count = 0;
    int capacity = 4;
    *servers = malloc(sizeof(struct Server) * capacity);
    char line[512];
    
    while (fgets(line, sizeof(line), f)) {
        // убрать символ новой строки
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0) continue;
        
        // разделить на ip и port
        char *colon = strchr(line, ':');
        if (!colon) {
            fprintf(stderr, "Invalid server line: %s\n", line);
            continue;
        }
        *colon = '\0';
        char *ip = line;
        int port = atoi(colon + 1);
        if (port <= 0) {
            fprintf(stderr, "Invalid port in: %s\n", line);
            continue;
        }
        
        // расширить массив при необходимости
        if (count >= capacity) {
            capacity *= 2;
            *servers = realloc(*servers, sizeof(struct Server) * capacity);
        }
        strcpy((*servers)[count].ip, ip);
        (*servers)[count].port = port;
        count++;
    }
    fclose(f);
    return count;
}

int main(int argc, char **argv) {
    uint64_t k = -1;
    uint64_t mod = -1;
    char servers_file[255] = {0};
    
    // парсинг аргументов командной строки
    while (1) {
        static struct option options[] = {
            {"k", required_argument, 0, 0},
            {"mod", required_argument, 0, 0},
            {"servers", required_argument, 0, 0},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;
        
        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        if (!ConvertStringToUI64(optarg, &k)) return 1;
                        break;
                    case 1:
                        if (!ConvertStringToUI64(optarg, &mod)) return 1;
                        break;
                    case 2:
                        strncpy(servers_file, optarg, sizeof(servers_file)-1);
                        break;
                }
                break;
            default:
                fprintf(stderr, "Unknown argument\n");
                return 1;
        }
    }
    
    if (k == -1 || mod == -1 || strlen(servers_file) == 0) {
        fprintf(stderr, "Usage: %s --k <num> --mod <num> --servers <file>\n", argv[0]);
        return 1;
    }
    
    // чтение списка серверов
    struct Server *servers = NULL;
    int servers_num = read_servers(servers_file, &servers);
    if (servers_num <= 0) {
        fprintf(stderr, "No servers read from file %s\n", servers_file);
        return 1;
    }
    printf("Loaded %d servers\n", servers_num);
    

    uint64_t total_numbers = k;
    uint64_t base_size = total_numbers / servers_num;
    uint64_t remainder = total_numbers % servers_num;
    
    uint64_t *results = malloc(sizeof(uint64_t) * servers_num);
    pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);
    struct ClientThreadArgs *thread_args = malloc(sizeof(struct ClientThreadArgs) * servers_num);
    
    uint64_t current_start = 1;
    for (int i = 0; i < servers_num; i++) {
        uint64_t size = base_size;
        if (i < remainder) size++;
        
        uint64_t start = current_start;
        uint64_t end = current_start + size - 1;
        current_start = end + 1;
        
        thread_args[i].server = servers[i];
        thread_args[i].begin = start;
        thread_args[i].end = end;
        thread_args[i].mod = mod;
        thread_args[i].result = &results[i];
        
        if (pthread_create(&threads[i], NULL, client_thread_func, &thread_args[i]) != 0) {
            perror("pthread_create");
            results[i] = 0;
        }
    }
    
    for (int i = 0; i < servers_num; i++) {
        pthread_join(threads[i], NULL);
    }
    
    uint64_t total = 1;
    for (int i = 0; i < servers_num; i++) {
        total = MultModulo(total, results[i], mod);
    }
    
    printf("Final answer: %llu! mod %llu = %llu\n", k, mod, total);
    
    free(servers);
    free(results);
    free(threads);
    free(thread_args);
    
    return 0;
}