#include "chatroom.h"
#include "server.h"

typedef struct {
    sockaddr_t addr;
    char name[NAMELEN];
    int id;
} client_t;

client_t *clients[MAXCLIENTS] = {NULL};

// server's socket file descriptor
int sockfd;

void bind_socket(sockaddr_t servaddr);
void run_server();

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
#endif
    if (argc < 2) {
        fprintf(stderr, "[ERROR]: A port number in program arguments is needed\n");
        exit(EXIT_FAILURE);
    } else if (argc > 2) {
        fprintf(stderr, "[ERROR]: Too many arguments\n");
        exit(EXIT_FAILURE);
    }
    sockfd = setup_socket(AF_INET, SOCK_DGRAM, 0);
#ifdef _WIN32
    // windows fix for recvfrom
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
    WSAIoctl(sockfd, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, NULL, 0, &dwBytesReturned, NULL, NULL);
#endif
    // null (to accept connections on all networks), port
    sockaddr_t servaddr = setup_server(NULL, atoi(argv[1]));
    bind_socket(servaddr);
    printf("[INFO] Server listening on port %d...\n", atoi(argv[1]));
    run_server();
    cleanup_socket(sockfd);
    return 0;
}

void bind_socket(sockaddr_t servaddr) {
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("[ERROR]: Failed to bind socket\n");
        exit(EXIT_FAILURE);
    }
}

// send a message to all clients whose id is not equal to except_id
// pass -1 for except_id to send to everyone
void broadcast_except(char *message, int except_id) {
    for (int i = 0; i < MAXCLIENTS - 1; i++) {
        if (clients[i] == NULL || i == except_id)
            continue;
        send_message(&clients[i]->addr, message);
    }
    printf("[BROADCAST]: %s\n", message);
}

int receive_message(sockaddr_t *from, char *message) {
    socklen_t fromlen = sizeof(struct sockaddr_storage);
    int status = recvfrom(sockfd, message, MAXMSG, 0, (struct sockaddr*) from, &fromlen);
    if (status < 0) {
        perror("[ERROR]: Failed to receive message\n");
        exit(EXIT_FAILURE);
    }
    message[status] = '\0';
    return status;
}

void *handle_client(void* arg) {
    client_t *client = (client_t*)arg;
    char msgbuffer[MAXMSG];
    char client_name_header[NAMELEN + 2];
    snprintf(client_name_header, sizeof(client_name_header), "%s:", client->name);

    while (1) {
        receive_message(NULL, msgbuffer);
        char message[MAXMSG + NAMELEN + 2];
        snprintf(message, sizeof(message), "%s%s", client_name_header, msgbuffer);
        for (int i = 0; i < MAXCLIENTS; i++) {
            if (clients[i] == NULL || i == client->id)
                continue;
            send_message(&clients[i]->addr, message);
        }
    }
    return NULL;
}

void show_all_participants(sockaddr_t *clientaddr) {
    char names_message[MAXMSG] = "Server: All chat participants: ";
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL)
        continue;
        strcat(names_message, clients[i]->name);
        strcat(names_message, " ");
    }
    strcat(names_message, "\n");
    send_message(clientaddr, names_message);
}

client_t *create_client(sockaddr_t *clientaddr, char *name) {
    client_t *client = (client_t*)malloc(sizeof(client_t));
    client->id = -1;
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL) { // found a free id
            clients[i] = client;
            client->id = i;
            break;
        }
    }
    if (client->id == -1) {
        free(client);
        return NULL;
    }
    client->addr = *clientaddr;
    strncpy(client->name, name + 1, 50);
    return client;
}

void add_to_chatroom(sockaddr_t *clientaddr, char *name) {
    client_t *client = create_client(clientaddr, name);
    if (client == NULL) {
        send_message(clientaddr, "Server: The chat room is full!\n");
        return;
    }
    
    // reply with client's id: "i<client_id>"
    char client_id_response[MAXMSG];
    sprintf(client_id_response, "%c%c", CMD_ID, client->id + '0');
    send_message(clientaddr, client_id_response);
    
    // notify that client joined
    char welcome_msg[NAMELEN + 35];
    sprintf(welcome_msg, "Server: %s has joined the chat room\n", client->name);
    broadcast_except(welcome_msg, -1);
}

void remove_client(client_t* client) {
    char leave_msg[NAMELEN + 33];
    sprintf(leave_msg, "Server: %s has left the chat room\n", client->name);
    clients[client->id] = NULL;
    free(client);
    broadcast_except(leave_msg, -1);
}

void *server_send(void* arg) {
    char input_buffer[MAXMSG - 9];
    char broadcasted_message[MAXMSG];
    while (1) {
        fgets(input_buffer, MAXMSG - 9, stdin);
        if (input_buffer[0] == CMD_PREFIX) {
            if (input_buffer[1] == CMD_ALL) {
                for (int i = 0; i < MAXCLIENTS; i++) {
                    if (clients[i] == NULL) continue;
                    printf("%d %s\n", clients[i]->id, clients[i]->name);
                }
            }
            if (1) {
                // other server commands
            }
            continue;
        }
        // do not add '\n' at end of format as a '\n' remains after fgets
        sprintf(broadcasted_message, "Server: %s", input_buffer);
        broadcast_except(broadcasted_message, -1);
    }
}

client_t *verify_client(char *message) {
    if (message[0] < '0' || message[0] > '9')
        return NULL;                  // allowed ids are 0-9
    return clients[message[0] - '0']; // still can be null
}

void run_server() {
    char message[MAXMSG];
    sockaddr_t clientaddr;
    pthread_t broadcast_thread;
    pthread_create(&broadcast_thread, NULL, server_send, NULL);
    while (1) {
        int nreceived = receive_message(&clientaddr, message);
        message[nreceived] = '\0';
        printf("[RECEIVED]: %s\n", message);
        // non-registered clients send messages without their id
        if (message[0] == CMD_REG) {
            add_to_chatroom(&clientaddr, message);
        }
        // client sends a message or a command
        else {
            client_t* client = verify_client(message);
            if (client == NULL)
                continue;
            if (message[1] == CMD_PREFIX) {
                if (message[2] == CMD_ALL)
                    show_all_participants(&clientaddr);
                else if (message[2] == CMD_LEAVE)
                    remove_client(client);
                continue;
            }
            char broadcasted_msg[NAMELEN + MAXMSG + 2];
            sprintf(broadcasted_msg, "%s: %s\n", client->name, message + 1);
            broadcast_except(broadcasted_msg, client->id);
        }
    }
}