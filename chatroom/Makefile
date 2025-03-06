CC = gcc
CFLAGS = -Wall

ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
	CLEAN = del client.exe server.exe
else
    LIBS = 
	CLEAN = rm -f client server
endif

all: client server

client: client.c chatroom.c
	$(CC) $(CFLAGS) -o client client.c chatroom.c $(LIBS)

server: server.c chatroom.c
	$(CC) $(CFLAGS) -o server server.c chatroom.c $(LIBS)

clean:
	$(CLEAN)