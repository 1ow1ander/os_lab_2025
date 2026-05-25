CC = gcc
CFLAGS = -Wall -Wextra

all: cp_tcpclient cp_tcpserver cp_udpclient cp_udpserver

tcpclient: cp_tcpclient.c
	$(CC) $(CFLAGS) -o cp_tcpclient cp_tcpclient.c

tcpserver: cp_tcpserver.c
	$(CC) $(CFLAGS) -o cp_tcpserver cp_tcpserver.c

udpclient: cp_udpclient.c
	$(CC) $(CFLAGS) -o cp_udpclient cp_udpclient.c

udpserver: cp_udpserver.c
	$(CC) $(CFLAGS) -o cp_udpserver cp_udpserver.c

clean:
	rm -f cp_tcpclient cp_tcpserver cp_udpclient cp_udpserver

run-tcp-server:
	./cp_tcpserver 10050 100

run-udp-server:
	./cp_udpserver 20001 1024