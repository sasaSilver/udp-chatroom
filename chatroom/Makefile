CC = gcc
CFLAGS = -Wall

ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32
else
    LIBS = 
endif

all: client server

client: client.c chatroom.c
	$(CC) $(CFLAGS) -o client client.c chatroom.c $(LIBS)

server: server.c chatroom.c
	$(CC) $(CFLAGS) -o server server.c chatroom.c $(LIBS)

clean:
	rm -f client server