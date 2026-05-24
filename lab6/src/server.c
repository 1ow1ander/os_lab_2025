/* cd lab6
 * cd src
 * ./server --port=20001 --tnum=2
 * ./server --port=20002 --tnum=2
 * ./server --port=20003 --tnum=2
 * ./client --k=10 --mod=1000 --servers=servers.txt
 * ctrl+C
 */

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <inttypes.h>
#include "common.h"

// параметры для вычисления факториала на одном потоке
struct FactorialArgs {
    uint64_t begin;   // начало диапазона
    uint64_t end;     // конец
    uint64_t mod;     // модуль
};

// произведение всех чисел от begin до end по модулю mod
uint64_t Factorial(const struct FactorialArgs *args) {
    uint64_t ans = 1;
    for (uint64_t i = args->begin; i <= args->end; i++) {
        ans = MultModulo(ans, i, args->mod);
    }
    return ans;
}

// ф-я, которую будет исполнять поток
void* ThreadFactorial(void *args) {
    struct FactorialArgs *fargs = (struct FactorialArgs*)args;
    uint64_t result = Factorial(fargs);
    return (void*)(uintptr_t)result;
}

int main(int argc, char **argv) {
    int tnum = -1;
    int port = -1;

    while (1) {
        static struct option options[] = {
            {"port", required_argument, 0, 0},
            {"tnum", required_argument, 0, 0},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        int c = getopt_long(argc, argv, "", options, &option_index);
        if (c == -1) break;
        
        switch (c) {
            case 0:
                switch (option_index) {
                    case 0:
                        port = atoi(optarg);
                        break;
                    case 1:
                        tnum = atoi(optarg);
                        break;
                }
                break;
            default:
                fprintf(stderr, "Unknown argument\n");
                return 1;
        }
    }
    
    if (port == -1 || tnum == -1) {
        fprintf(stderr, "Usage: %s --port <port> --tnum <threads>\n", argv[0]);
        return 1;
    }
    
    // создание сокета (TCP)
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }
    
    int opt_val = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
    
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons((uint16_t)port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(server_fd, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("bind");
        return 1;
    }
    
    if (listen(server_fd, 128) < 0) {
        perror("listen");
        return 1;
    }
    
    printf("Server listening on port %d, using %d threads per request\n", port, tnum);

    while (1) {
        struct sockaddr_in client;
        socklen_t client_len = sizeof(client);
        // accept() блокируется, пока не придёт новый клиент
        int client_fd = accept(server_fd, (struct sockaddr*)&client, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        char buffer[sizeof(uint64_t) * 3];
        ssize_t read_bytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (read_bytes != sizeof(buffer)) {
            fprintf(stderr, "Client sent incomplete data (%zd bytes)\n", read_bytes);
            close(client_fd);
            continue;
        }
        
        uint64_t begin, end, mod;
        memcpy(&begin, buffer, sizeof(uint64_t));
        memcpy(&end, buffer + sizeof(uint64_t), sizeof(uint64_t));
        memcpy(&mod, buffer + 2 * sizeof(uint64_t), sizeof(uint64_t));
        
        printf("Received task: [%" PRIu64 ", %" PRIu64 "] mod %" PRIu64 "\n", begin, end, mod);
        
        uint64_t total_numbers = end - begin + 1;
        uint64_t base_size = total_numbers / tnum;
        uint64_t remainder = total_numbers % tnum;
        
        pthread_t threads[tnum];
        struct FactorialArgs args[tnum];
        
        uint64_t current = begin;
        for (int i = 0; i < tnum; i++) {
            uint64_t size = base_size;
            if (i < remainder) size++;
            
            uint64_t start = current;
            uint64_t finish = current + size - 1;
            current = finish + 1;
            
            args[i].begin = start;
            args[i].end = finish;
            args[i].mod = mod;
            
            if (pthread_create(&threads[i], NULL, ThreadFactorial, &args[i]) != 0) {
                perror("pthread_create");
                close(client_fd);
                goto next_client;
            }
        }
        
        uint64_t total = 1;
        for (int i = 0; i < tnum; i++) {
            void *res_ptr;
            uint64_t partial;
            if (pthread_join(threads[i], &res_ptr) != 0) {
                perror("pthread_join");
                partial = 1;
            } else {
                partial = (uint64_t)(uintptr_t)res_ptr;
            }
            total = MultModulo(total, partial, mod);
        }
        
        printf("Computed partial product = %" PRIu64 "\n", total);
        
        char response[sizeof(uint64_t)];
        memcpy(response, &total, sizeof(uint64_t));
        if (send(client_fd, response, sizeof(response), 0) < 0) {
            perror("send");
        }
        
        close(client_fd);
        
        next_client:
        continue;
    }
    
    close(server_fd);
    return 0;
}