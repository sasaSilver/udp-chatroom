#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "chatroom.h"

#define MAXCLIENTS 10

typedef struct sockaddr_in sockaddr_t;

typedef struct {
    sockaddr_t addr;
    char name[NAMELEN];
    int id;
} client_t;

client_t *clients[MAXCLIENTS] = {NULL};

// server's socket file descriptor
int sockfd;
int setup_socket(int domain, int type, int protocol);
sockaddr_t setup_server(int port);
void bind_server(sockaddr_t servaddr);
void run_server(sockaddr_t servaddr);
void send_message(sockaddr_t toaddr, char* message);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        perror("Error: A port number in program arguments is needed\n");
        exit(EXIT_FAILURE);
    } else if (argc > 2) {
        perror("Error: Too many arguments\n");
        exit(EXIT_FAILURE);
    }

    sockfd = setup_socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_t servaddr = setup_server(atoi(argv[1]));
    bind_server(servaddr);
    printf("Server listening on port %d\n", atoi(argv[1]));
    run_server(servaddr);
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
sockaddr_t setup_server(int port) {
    sockaddr_t servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);
    servaddr.sin_family = AF_INET;
    return servaddr;
}

void bind_server(sockaddr_t servaddr) {
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Error: Failed to bind socket\n");
        exit(EXIT_FAILURE);
    }
}

void send_message(sockaddr_t toaddr, char* message) {
    if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr*) &toaddr, sizeof(toaddr)) < 0) {
        perror("Error: Failed to send message\n");
        exit(EXIT_FAILURE);
    }
    printf("sent: %s\n", message);
}

int receive_message(sockaddr_t *fromaddr, char* message) {
    static socklen_t server_struct_len = sizeof(*fromaddr);
    int nreceived = recvfrom(sockfd, message, MSGBUFFER, 0, (struct sockaddr *)fromaddr, &server_struct_len);
    if (nreceived < 0) {
        perror("Error: Failed to receive message\n");
        exit(EXIT_FAILURE);
    }
    message[nreceived] = '\0';
    printf("got: %s\n", message);
    return nreceived;
}

void *handle_client(void* arg) {
    client_t *client = (client_t*)arg;
    char msgbuffer[MSGBUFFER];
    char client_name_header[NAMELEN + 2];
    snprintf(client_name_header, sizeof(client_name_header), "%s:", client->name);

    while (1) {
        int nreceived = receive_message(NULL, msgbuffer);
        char message[MSGBUFFER + NAMELEN + 2];
        snprintf(message, sizeof(message), "%s%s", client_name_header, msgbuffer);
        for (int i = 0; i < MAXCLIENTS; i++) {
            if (clients[i] == NULL || i == client->id)
                continue;
            send_message(clients[i]->addr, message);
        }
    }
    return NULL;
}

int add_client(client_t *client) {
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL) {
            clients[i] = client;
            client->id = i;
            return i;
        }
    }
    return -1;
}

void remove_client(client_t *client) {
    clients[client->id] = NULL;
    free(client);
}

void broadcast_except(char *message, int except_id) {
    for (int i = 0; i < MAXCLIENTS - 1; i++) {
        if (clients[i] == NULL || i == except_id)
            continue;
        send_message(clients[i]->addr, message);
    }
}

void show_all_participants(sockaddr_t clientaddr) {
    char names_message[MSGBUFFER] = "Server: All chat participants: ";
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL)
            continue;
        strcat(names_message, clients[i]->name);
        strcat(names_message, " ");
    }
    strcat(names_message, "\n");
    send_message(clientaddr, names_message);
}

void client_leave(client_t* client) {
    char leave_msg[NAMELEN + 33];
    sprintf(leave_msg, "Server: %s has left the chat room\n", client->name);
    remove_client(client);
    broadcast_except(leave_msg, -1);
}

void *server_send(void* arg) {
    char input_buffer[MSGBUFFER - 9];
    char broadcasted_message[MSGBUFFER];
    while (1) {
        fgets(input_buffer, MSGBUFFER - 9, stdin);
        if (input_buffer[0] == CMD_CLIENT) {
            if (input_buffer[1] == CMD_ALL) {
                for (int i = 0; i < MAXCLIENTS; i++) {
                    if (clients[i] == NULL) continue;
                    printf("%d %s\n", clients[i]->id, clients[i]->name);
                }
            }
            continue;
        }
        sprintf(broadcasted_message, "Server: %s", input_buffer);
        broadcast_except(broadcasted_message, -1);
    }
}

void run_server(sockaddr_t servaddr) {
    char message[MSGBUFFER];
    sockaddr_t clientaddr;
    pthread_t broadcast_thread;
    pthread_create(&broadcast_thread, NULL, server_send, NULL);
    while (1) {
        int nreceived = receive_message(&clientaddr, message);
        message[nreceived] = '\0';
        
        // register a client
        if (message[0] == CMD_REG) {
            // create a client
            client_t *client = (client_t*)malloc(sizeof(client_t));
            int client_id = add_client(client);
            if (client_id < 0) {
                send_message(clientaddr, "Server: The chat room is full!\n");
                free(client);
                continue;
            }
            client->id = client_id;
            client->addr = clientaddr;
            strncpy(client->name, message + 1, 50);
            
            // reply with client's id
            char client_id_response[3];
            sprintf(client_id_response, "%c%c", CMD_ID, client_id + '0');
            send_message(clientaddr, client_id_response);
            
            // notify that client joined
            char welcome_msg[NAMELEN + 35];
            sprintf(welcome_msg, "Server: %s has joined the chat room\n", client->name);
            broadcast_except(welcome_msg, -1);
        }
        // client sends a message or a command
        else if ('0' <= message[0] && message[0] <= '9') {
            client_t* client = clients[message[0] - '0'];
            if (client == NULL) {
                fprintf(stderr, "Error: Invalid client ID <%c> received\n", message[0]);
                continue;
            }
            if (message[1] == CMD_CLIENT) {
                if (message[2] == CMD_ALL)
                    show_all_participants(clientaddr);
                else if (message[2] == CMD_LEAVE)
                    client_leave(client);
                continue;
            }
            char broadcasted_msg[NAMELEN + MSGBUFFER + 2];
            sprintf(broadcasted_msg, "%s: %s\n", client->name, message + 1);
            broadcast_except(broadcasted_msg, client->id);
        }
    }
}