#include "chatroom.h"

#define MAXCLIENTS 10
#define MAXMSG MAXSEND - 10

typedef struct {
    int sock;
    char name[NAMELEN];
    int id;
} client_t;

int sockfd;
client_t *clients[MAXCLIENTS] = {NULL};
pthread_t broadcast_thread;

// Function declarations
void bind_socket(sockaddr_t servaddr);
void run_server();
void *handle_client(void *arg);
client_t *find_client_by_sock(int sock);

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        cr_error("WSAStartup failed: %d", WSAGetLastError());
#endif
    if (argc < 2)
        cr_error("A port number in program arguments is needed");
    else if (argc > 2) 
        cr_error("Too many arguments");
    sockfd = cr_setup_socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_t servaddr = cr_setup_server(NULL, atoi(argv[1])); // null (to accept connections on all networks), port
    bind_socket(servaddr);
    
    if (listen(sockfd, MAXCLIENTS) < 0)
        cr_error("Failed to listen on socket");

    cr_log("Server is listening on %s:%d...", inet_ntoa(servaddr.sin_addr), atoi(argv[1]));

    signal(SIGINT, &cr_on_app_close);
    signal(SIGTERM, &cr_on_app_close);
    
    run_server();
}

void bind_socket(sockaddr_t servaddr) {
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0)
        cr_error("Failed to set socket options");
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        cr_error("Failed to bind socket");
}

int client_exists(int client_id) {
    return 0 <= client_id && client_id < MAXCLIENTS && clients[client_id];
}

// send a message to all clients whose id is not equal to except_id
// pass -1 for except_id to send to everyone
void broadcast_except(char *message, int except_id) {
    char full_msg[MAXSEND];
    char message_start = client_exists(except_id) ? USER_MESSAGE_START : SERVER_MESSAGE_START;
    snprintf(full_msg, MAXSEND, "%c%s", message_start, message);
    
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL || i == except_id)
            continue;
        send(clients[i]->sock, full_msg, strlen(full_msg), 0);
    }
    
    printf("[BROADCAST] %s\n", message);
}

void receivefrom(sockaddr_t *from, char *message) {
    socklen_t fromlen = sizeof(struct sockaddr_storage);
    int status = recvfrom(sockfd, message, MAXSEND, 0, (struct sockaddr*) from, &fromlen);
    if (status < 0)
        cr_error("Failed to receive message");
    message[status] = '\0';
}

void send_all_clients(int client_sock) {
    char names_message[MAXSEND];
    snprintf(names_message, MAXSEND, "%c%s", SERVER_MESSAGE_START, "All connected clients: ");
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL)
            continue;
        strcat(names_message, clients[i]->name);
        strcat(names_message, " ");
    }
    send(client_sock, names_message, strlen(names_message), 0);
}

client_t *create_client(int sock, char *name) {
    client_t *client = (client_t*)malloc(sizeof(client_t));
    client->id = -1;
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL) { // found a free id
            clients[i] = client;
            client->id = i;
            break;
        }
    }
    if (client->id == -1) { // didn't find a free id
        free(client);
        return NULL;
    }
    client->sock = sock;
    strncpy(client->name, name + 1, 50);
    return client;
}

void add_to_chatroom(int sock, char *name) {
    client_t *client = create_client(sock, name);
    if (client == NULL) {
        free(client);
        char no_id_reply[MAXSEND];
        snprintf(no_id_reply, MAXSEND, "%c%c", CMD_ID, EMPTY_ID);
        send(sock, no_id_reply, strlen(no_id_reply), 0);
        cr_log("Client %s failed to join the chatroom", name);
        return;
    }
    // reply with client's id: "i<client_id>"
    char client_id_response[MAXSEND];
    sprintf(client_id_response, "%c%c", CMD_ID, client->id + '0');
    send(sock, client_id_response, strlen(client_id_response), 0);
    
    // notify that client joined
    char welcome_msg[MAXSEND];
    sprintf(welcome_msg, "%s has joined the chatroom", client->name);
    broadcast_except(welcome_msg, -1);
    
    cr_log("%s joined", client->name);
}

void remove_client(client_t* client) {
    char leave_msg[MAXSEND];
    sprintf(leave_msg, "%s has left the chatroom", client->name);
    clients[client->id] = NULL;
    free(client);
    broadcast_except(leave_msg, -1);
}

void send_client_id(client_t *client) {
    char client_id_reply[MAXSEND];
    snprintf(client_id_reply, MAXSEND, "%c%c%c", SERVER_MESSAGE_START, CMD_ID, client->id + '0');
    send(client->sock, client_id_reply, strlen(client_id_reply), 0);
}

void print_ids_participants() {
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL)
            continue;
        printf("%d %s\n", clients[i]->id, clients[i]->name);
    }
}

void *server_send_routine(void* arg) {
    char input_buffer[MAXMSG];
    char broadcasted_message[MAXSEND];
    while (fgets(input_buffer, MAXSEND - 10, stdin)) {
        input_buffer[strcspn(input_buffer, "\n")] = '\0';
        if (input_buffer[0] == CMD_PREFIX) {
            if (input_buffer[1] == CMD_ALL)
                print_ids_participants();
            else
                cr_warn("Unknown command: %s\n", input_buffer);
            continue;
        }
        // do not add '\n' at end of format as a '\n' remains after fgets
        sprintf(broadcasted_message, "%s", input_buffer);
        broadcast_except(broadcasted_message, -1);
    }
    return NULL;
}

client_t *verify_client(char client_id) {
    if (client_id < '0' || client_id > '9')
        return NULL;                  // allowed ids are 0-9
    return clients[client_id - '0'];  // still can be null
}

void unknown_client_command(int sock, char *command) {
    char unknown_cmd_message[MAXSEND];
    sprintf(unknown_cmd_message, "%cUnknown command: %s", SERVER_MESSAGE_START, command);
    send(sock, unknown_cmd_message, strlen(unknown_cmd_message), 0);
}

void send_help(int sock) {
    static char *help_message = 
        "\n"
        "!r - Register in the chatroom.\n"
        "!q - Leave the chatroom.\n"
        "!i - Show your user ID.\n"
        "!a - Show all users in the chatroom\n"
        "     Also available as a server command that lists"
        "     all connected users and their ids\n"
        "!h - Display this message."
        "\n\n";
    send(sock, help_message, strlen(help_message), 0);
}

void run_server() {
    // thread to send server messages
    pthread_create(&broadcast_thread, NULL, server_send_routine, NULL);
    
    sockaddr_t clientaddr;
    int client_sock;
    socklen_t client_len = sizeof(clientaddr);
    
    while (1) {
        client_sock = accept(sockfd, (struct sockaddr*)&clientaddr, &client_len);
        if (client_sock < 0)
            cr_error("Failed to accept connection");
            
        // Create a new thread to handle this client
        pthread_t client_thread;
        client_thread_args_t *args = malloc(sizeof(client_thread_args_t));
        args->sock = client_sock;
        args->addr = clientaddr;
        pthread_create(&client_thread, NULL, handle_client, args);
        pthread_detach(client_thread);
    }
}

void *handle_client(void *arg) {
    client_thread_args_t *args = (client_thread_args_t*)arg;
    int client_sock = args->sock;
    free(arg);
    
    char message[MAXSEND];
    client_t *client;
    
    while (1) {
        int status = recv(client_sock, message, MAXSEND, 0);
        if (status <= 0) {
            // Client disconnected
            if ((client = find_client_by_sock(client_sock))) {
                remove_client(client);
            }
            break;
        }
        message[status] = '\0';
        printf("[RECEIVED] %s\n", message);
        
        // Handle client messages
        if (message[1] == CMD_PREFIX) {
            if (message[2] == CMD_REG)
                add_to_chatroom(client_sock, message + 2);
            else if (message[2] == CMD_HELP)
                send_help(client_sock);
        }
                
        if ((client = verify_client(message[0]))) {
            if (message[1] == CMD_PREFIX) {
                if (message[2] == CMD_LEAVE)
                    remove_client(client);
                else if (message[2] == CMD_ALL)
                    send_all_clients(client_sock);
                else if (message[2] == CMD_ID)
                    send_client_id(client);
                else
                    unknown_client_command(client_sock, message + 1);
                continue;
            }
            char broadcasted_msg[NAMELEN + MAXSEND + 2];
            sprintf(broadcasted_msg, "%s: %s", client->name, message + 1);
            broadcast_except(broadcasted_msg, client->id);
        }
    }
    
#ifdef _WIN32
    closesocket(client_sock);
#else
    close(client_sock);
#endif
    return NULL;
}

client_t *find_client_by_sock(int sock) {
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] && clients[i]->sock == sock)
            return clients[i];
    }
    return NULL;
}

void cr_on_app_close(int signum) {
    pthread_cancel(broadcast_thread);
    cr_cleanup_socket();
}
