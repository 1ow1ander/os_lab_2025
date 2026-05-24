CC = gcc
CFLAGS = -Wall -Wextra -pedantic -O2 -pthread

CLIENT_TARGET = client
SERVER_TARGET = server
COMMON_OBJ = common.o

CLIENT_SRC = client.c
SERVER_SRC = server.c
COMMON_SRC = common.c

all: $(CLIENT_TARGET) $(SERVER_TARGET)

# сборка клиента
$(CLIENT_TARGET): $(CLIENT_SRC) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Сборка сервера
$(SERVER_TARGET): $(SERVER_SRC) $(COMMON_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# компиляция общего модуля
$(COMMON_OBJ): $(COMMON_SRC) common.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_TARGET) $(SERVER_TARGET) $(COMMON_OBJ)

.PHONY: all clean help