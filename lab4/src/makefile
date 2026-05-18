CC=gcc
CFLAGS=-I.

all: parallel_min_max process_memory parallel_sum

parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

parallel_sum: utils.o sum_lib.o
	$(CC) -o parallel_sum parallel_sum.c utils.o sum_lib.o $(CFLAGS)

sum_lib.o : sum_lib.h sum_lib.h
	$(CC) -o sum_lib.o -c sum_lib.c $(CFLAGS)

process_memory : process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

clean :
	rm -f utils.o find_min_max.o sum_lib.o parallel_min_max process_memory parallel_sum