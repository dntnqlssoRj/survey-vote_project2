CC = gcc
CFLAGS = -Iinclude
LDFLAGS = -pthread

all: server client

server:
	$(CC) $(CFLAGS) src/server/server_main.c $(LDFLAGS) -o src/server/server

client:
	$(CC) $(CFLAGS) src/client/client_main.c       -o src/client/client

clean:
	rm -f src/server/server src/client/client