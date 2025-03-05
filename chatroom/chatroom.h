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
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h> // Include pthread.h for Unix/Linux
#endif

#define CMD_REG 'r'
#define CMD_PREFIX '!'
#define CMD_LEAVE 'q'
#define CMD_ID 'i'
#define CMD_ALL 'a'

#define NAMELEN 50
#define MSGBUFFER 256

typedef struct sockaddr_in sockaddr_t;

void cleanup_socket(int sockfd);
int setup_socket(int domain, int type, int protocol);
struct sockaddr_in setup_server(char* ip, int port);
int send_message(sockaddr_t *toaddr, char* message);
int receive_message(sockaddr_t *from, char *message);

#endif
