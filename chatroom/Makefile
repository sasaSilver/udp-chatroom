CC = gcc
CFLAGS = -Wall

ifeq ($(OS),Windows_NT)
    LIBS = -lws2_32 -lpthread
	CLEAN = del client.exe server.exe client server .\test\test.exe .\test\test
else
    LIBS = -lpthread
	CLEAN = rm -f client server client.exe server.exe ./test/test ./test/test.exe
endif

all: client server check

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