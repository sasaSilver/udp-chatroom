#ifndef CHATROOM_H
#define CHATROOM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#ifdef _WIN32

#include <winsock2.h>
#include <windows.h>
#include <Pthread.h>

typedef int socklen_t;
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)

#else

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#endif

#define CMD_PREFIX '!'
#define CMD_REG 'r'
#define CMD_LEAVE 'q'
#define CMD_ID 'i'
#define CMD_ALL 'a'
#define CMD_HELP 'h'

#define NAMELEN 50
#define MAXSEND 256
typedef struct sockaddr_in sockaddr_t;

void cleanup_socket();
int setup_socket(int domain, int type, int protocol);
sockaddr_t setup_server(char* ip, int port);
void send_message(sockaddr_t *to, char* message);
void throw(const char *format, ...);
void on_app_close(int signum);

#endif
