#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "chatroom.h"

typedef struct sockaddr_in sockaddr_t;
pthread_mutex_t chat_mutex = PTHREAD_MUTEX_INITIALIZER;

int sockfd;
int setup_socket(int domain, int type, int protocol);
sockaddr_t setup_serv(char* ip, int port);
void run_client(sockaddr_t servaddr);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        perror("Error: Server IP and port in program arguments are needed\n");
        exit(EXIT_FAILURE);
    } else if (argc > 3) {
        perror("Error: Too many arguments\n");
        exit(EXIT_FAILURE);
    }

    sockfd = setup_socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_t servaddr = setup_serv(argv[1], atoi(argv[2]));
    run_client(servaddr);
    close(sockfd);
    return 0;
}

// creates a socket file descriptor
int setup_socket(int domain, int type, int protocol) {
    int sockfd = socket(domain, type, protocol);
    if (sockfd < 0) {
        perror("Error: Failed to create socket\n");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

// creates an internet socket of the server
sockaddr_t setup_serv(char* ip, int port) {
    sockaddr_t servaddr;
    bzero(&servaddr, sizeof(servaddr));
    if (!inet_aton(ip, &servaddr.sin_addr)) {
        perror("Error: Invalid server address\n");
        exit(EXIT_FAILURE);
    }
    servaddr.sin_port = htons(port);
    servaddr.sin_family = AF_INET;
    return servaddr;
}

int send_message(sockaddr_t servaddr, char* message) {
    int nreceived = sendto(sockfd, message, strlen(message), 0, (struct sockaddr*) &servaddr, sizeof(servaddr));
    if (nreceived < 0) {
        perror("Error: Failed to send message\n");
        exit(EXIT_FAILURE);
    }
    return nreceived;
}

int receive_message(char* message) {
    static socklen_t server_struct_len = sizeof(sockaddr_t);
    int nreceived = recvfrom(sockfd, message, MSGBUFFER, 0, NULL, &server_struct_len);
    if (nreceived < 0) {
        perror("Error: Failed to receive message\n");
        exit(EXIT_FAILURE);
    }
    message[nreceived] = '\0';
    return nreceived;
}

void *receive_routine(void* arg) {
    char msgbuffer[MSGBUFFER];
    while (1) {
        receive_message(msgbuffer);
        printf("%s", msgbuffer);
    }
}

char register_client(sockaddr_t servaddr) {
    printf("Your name: ");
    char name[NAMELEN];
    fgets(name, NAMELEN, stdin);
    name[strcspn(name, "\n")] = '\0';
    
    char register_request[MSGBUFFER];
    sprintf(register_request, "r%s", name);
    send_message(servaddr, register_request);
    
    char client_id_reply[4];
    int nreceived = receive_message(client_id_reply);
    client_id_reply[nreceived] = '\0';
        
    int client_id = atoi(client_id_reply + 1);
    if (client_id == -1) {
        perror("Error: Chatroom is full. Unable to connect\n");
        exit(EXIT_FAILURE);
    }
    return client_id + '0';
}

void leave_chatroom(sockaddr_t servaddr, char client_id) {
    char quit_request[4];
    sprintf(quit_request, "%c%c%c", client_id, CMD_CLIENT, CMD_LEAVE);
    send_message(servaddr, quit_request);
}

void get_all_participants(sockaddr_t servaddr, char client_id) {
    char all_participants_request[4];
    sprintf(all_participants_request, "%c%c%c", client_id, CMD_CLIENT, CMD_ALL);
    send_message(servaddr, all_participants_request);
}

void run_client(sockaddr_t servaddr) {
    pthread_t receive_thread;
    char buffer[MSGBUFFER - 2];
    char client_id;
    while (1) {
        fgets(buffer, MSGBUFFER - 2, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (buffer[0] == CMD_CLIENT) {
            if (buffer[1] == CMD_LEAVE) {
                leave_chatroom(servaddr, client_id);
                pthread_cancel(receive_thread);
            }
            else if (buffer[1] == CMD_ALL)
                get_all_participants(servaddr, client_id);
            else if (buffer[1] == CMD_REG) {
                client_id = register_client(servaddr);
                pthread_create(&receive_thread, NULL, receive_routine, NULL);
            }
            else if (buffer[1] == CMD_ID)
                printf("%c\n", client_id);
            continue;
        }
        char message[MSGBUFFER];
        sprintf(message, "%c%s", client_id, buffer);
        send_message(servaddr, message);
    }
}
