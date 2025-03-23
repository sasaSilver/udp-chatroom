CC = gcc
CFLAGS = -Wall -Werror

ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32 -lpthread
	CLEAN = del /F client server client.exe server.exe
else
    LIBS = -lpthread
	CLEAN = rm -f client server client.exe server.exe
endif

all: client server

client: client.c chatroom.c
	$(CC) $(CFLAGS) -o client client.c chatroom.c $(LIBS)

server: server.c chatroom.c
	$(CC) $(CFLAGS) -o server server.c chatroom.c $(LIBS)

clean:
	$(CLEAN)

# check: test/test.c
# 	$(CC) $(CFLAGS) -o test/test test/test.c $(LIBS)
# 	./test/test
# 	make clean