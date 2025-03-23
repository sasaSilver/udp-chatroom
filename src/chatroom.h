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
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12) // windows fix

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

#define EMPTY_ID '_'
#define USER_MESSAGE_START '`'
#define SERVER_MESSAGE_START '~'

#define NAMELEN 50
#define MAXSEND 256
typedef struct sockaddr_in sockaddr_t;

void cr_cleanup_socket();
int cr_setup_socket(int domain, int type, int protocol);
sockaddr_t cr_setup_server(char* ip, int port);
// TODO change to use format and vargs
void cr_send(sockaddr_t *to, char* message);
void cr_on_app_close(int signum);

void cr_log(char *format, ...);
void cr_warn(char *format, ...);
void cr_error(char *format, ...);

#endif
