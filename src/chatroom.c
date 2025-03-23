#include <stdarg.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "chatroom.h"

extern int sockfd;

void cr_cleanup_socket() {
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}


int cr_setup_socket(int domain, int type, int protocol) {
    sockfd = socket(domain, type, protocol);
    if (sockfd < 0)
        cr_error("Failed to create socket\n");
    return sockfd;
}

sockaddr_t cr_setup_server(char* ip, int port) {
    sockaddr_t servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_addr.s_addr = ip ? inet_addr(ip) : INADDR_ANY;
    servaddr.sin_port = htons(port);
    servaddr.sin_family = AF_INET;
    return servaddr;
}

// TODO change to use vargs
void cr_send(sockaddr_t *to, char* message) {
    int status = sendto(sockfd, message, strlen(message), 0, (struct sockaddr*) to, sizeof(sockaddr_t));
    if (status < 0)
        cr_error("Failed to send message\n");
}


void cr_log(char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char log_format[MAXSEND];
    snprintf(log_format, sizeof(log_format), "[INFO] %s\n", format);
    vfprintf(stdout, log_format, args);
    
    va_end(args);
}

void cr_warn(char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char warn_format[MAXSEND];
    snprintf(warn_format, sizeof(warn_format), "[WARNING] %s\n", format);
    vfprintf(stderr, warn_format, args);
    
    va_end(args);
}

void cr_error(char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char err_format[MAXSEND];
    snprintf(err_format, sizeof(err_format), "[ERROR] %s\n", format);
    vfprintf(stderr, err_format, args);
    
    va_end(args);
    exit(EXIT_FAILURE);
}