#include <stdarg.h>
#include <signal.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#include "chatroom.h"

void cleanup_socket(int sockfd) {
#ifdef _WIN32
    closesocket(sockfd);
#else
    close(sockfd);
#endif
}

extern int sockfd;

int setup_socket(int domain, int type, int protocol) {
    sockfd = socket(domain, type, protocol);
    if (sockfd < 0) {
        perror("Error: Failed to create socket\n");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

sockaddr_t setup_server(char* ip, int port) {
    sockaddr_t servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_addr.s_addr = ip ? inet_addr(ip) : INADDR_ANY;
    servaddr.sin_port = htons(port);
    servaddr.sin_family = AF_INET;
    return servaddr;
}

void send_message(sockaddr_t *to, char* message) {
    int status = sendto(sockfd, message, strlen(message), 0, (struct sockaddr*) to, sizeof(sockaddr_t));
    if (status < 0) {
        perror("Error: Failed to send message\n");
        exit(EXIT_FAILURE);
    }
}

void throw(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    vfprintf(stderr, format, args);
    
    va_end(args);
    exit(EXIT_FAILURE);
}

void help() {
    printf("[HELP]\n");
    printf("!r - Register in the chatroom.\n");
    printf("!q - Leave the chatroom.\n");
    printf("!i - Show your user ID.\n");
    printf("!a - Show all users in the chatroom\n");
    printf("     Also available as a server command that lists");
    printf("     all connected users and their ids\n");
    printf("!h - Display this message.\n");
}
