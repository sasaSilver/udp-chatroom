#include "chatroom.h"

#ifdef _WIN32
#include <winsock2.h>
typedef int socklen_t; // Define socklen_t as int for Windows
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

int send_message(sockaddr_t *toaddr, char* message) {
    int nreceived = sendto(sockfd, message, strlen(message), 0, (struct sockaddr*) toaddr, sizeof(sockaddr_t));
    if (nreceived < 0) {
        perror("Error: Failed to send message\n");
        exit(EXIT_FAILURE);
    }
    return nreceived;
}

int receive_message(sockaddr_t *from, char *message) {
    static socklen_t server_struct_len = sizeof(struct sockaddr_in);
    int nreceived = recvfrom(sockfd, message, MSGBUFFER, 0, (struct sockaddr*) from, &server_struct_len);
    if (nreceived < 0) {
        perror("Error: Failed to receive message\n");
        exit(EXIT_FAILURE);
    }
    message[nreceived] = '\0';
    return nreceived;
}
