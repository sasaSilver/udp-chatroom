#include "chatroom.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

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

int send_message(sockaddr_t *to, char* message) {
    int status = sendto(sockfd, message, strlen(message), 0, (struct sockaddr*) to, sizeof(sockaddr_t));
    if (status < 0) {
        perror("Error: Failed to send message\n");
        exit(EXIT_FAILURE);
    }
    return status;
}
