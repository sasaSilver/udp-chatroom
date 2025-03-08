#include "chatroom.h"

#define MAXCLIENTS 10
#define MAXMSG MAXSEND - 10

typedef struct {
    sockaddr_t addr;
    char name[NAMELEN];
    int id;
} client_t;

client_t *clients[MAXCLIENTS] = {NULL};
int sockfd;
pthread_t broadcast_thread;

void bind_socket(sockaddr_t servaddr);
void run_server();

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        throw("WSAStartup failed: %d", WSAGetLastError());
#endif
    if (argc < 2)
        throw("[ERROR] A port number in program arguments is needed\n");
    else if (argc > 2) 
        throw("[ERROR] Too many arguments");
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
    
    signal(SIGINT, &on_app_close);
    signal(SIGTERM, &on_app_close);
    
    run_server();
}

void bind_socket(sockaddr_t servaddr) {
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
        throw("[ERROR] Failed to bind socket");
}

// send a message to all clients whose id is not equal to except_id
// pass -1 for except_id to send to everyone
void broadcast_except(char *message, int except_id) {
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL || i == except_id)
            continue;
        send_message(&clients[i]->addr, message);
    }
    printf("[BROADCAST] %s", message);
}

int receive_message(sockaddr_t *from, char *message) {
    socklen_t fromlen = sizeof(struct sockaddr_storage);
    int status = recvfrom(sockfd, message, MAXSEND, 0, (struct sockaddr*) from, &fromlen);
    if (status < 0)
        throw("[ERROR] Failed to receive message");
    message[status] = '\0';
    return status;
}

void *handle_client(void* arg) {
    client_t *client = (client_t*)arg;
    char msgbuffer[MAXSEND];
    char client_name_header[NAMELEN + 2];
    snprintf(client_name_header, sizeof(client_name_header), "%s:", client->name);

    while (1) {
        receive_message(NULL, msgbuffer);
        char message[MAXSEND + NAMELEN + 2];
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
    char names_message[MAXSEND] = "[SERVER] All chat participants: ";
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL)
        continue;
        strcat(names_message, clients[i]->name);
        strcat(names_message, " ");
    }
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

void add_to_chatroom(sockaddr_t *clientaddr, char *reqest) {
    reqest += 3; // skip previous client id, cmd prefix, and cmd register chars.
    client_t *client = create_client(clientaddr, reqest);
    if (client == NULL) {
        send_message(clientaddr, "[SERVER] The chat room is full!");
        printf("[INFO] Client %s tried to join the chat room, but it's full\n", reqest);
        return;
    }
    // reply with client's id: "i<client_id>"
    char client_id_response[MAXSEND];
    sprintf(client_id_response, "%c%c", CMD_ID, client->id + '0');
    send_message(clientaddr, client_id_response);
    
    // notify that client joined
    char welcome_msg[MAXSEND];
    sprintf(welcome_msg, "[SERVER] %s has joined the chat room", client->name);
    broadcast_except(welcome_msg, -1);
    
    printf("[INFO] %s joined\n", client->name);
}

void remove_client(client_t* client) {
    char leave_msg[MAXSEND];
    sprintf(leave_msg, "[SERVER] %s has left the chat room", client->name);
    clients[client->id] = NULL;
    free(client);
    broadcast_except(leave_msg, -1);
}

void print_ids_participants() {
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (clients[i] == NULL) continue;
        printf("%d %s\n", clients[i]->id, clients[i]->name);
    }
}

void *server_send(void* arg) {
    char input_buffer[MAXMSG];
    char broadcasted_message[MAXSEND];
    while (1) {
        fgets(input_buffer, MAXSEND - 10, stdin);
        if (input_buffer[0] == CMD_PREFIX) {
            if (input_buffer[1] == CMD_ALL)
                print_ids_participants();
            else if (0) {
                // other server commands
            }
            else
                printf("[WARNING] Unknown command: %s\n", input_buffer);
            continue;
        }
        // do not add '\n' at end of format as a '\n' remains after fgets
        sprintf(broadcasted_message, "[SERVER] %s", input_buffer);
        broadcast_except(broadcasted_message, -1);
    }
}

client_t *verify_client(char client_id) {
    if (client_id < '0' || client_id > '9')
        return NULL;                  // allowed ids are 0-9
    return clients[client_id - '0'];  // still can be null
}

void unknown_client_command(sockaddr_t *clientaddr, char *message) {
    char unknown_cmd_message[MAXSEND];
    sprintf(unknown_cmd_message, "[SERVER] Unknown command: %s", message);
    send_message(clientaddr, unknown_cmd_message);
}

void send_help(sockaddr_t *clientaddr) {
    static char *help_message = 
        "[HELP]\n"
        "!r - Register in the chatroom.\n"
        "!q - Leave the chatroom.\n"
        "!i - Show your user ID.\n"
        "!a - Show all users in the chatroom\n"
        "     Also available as a server command that lists"
        "     all connected users and their ids\n"
        "!h - Display this message.\n"
    ;
    send_message(clientaddr, help_message);
}

void run_server() {
    pthread_create(&broadcast_thread, NULL, server_send, NULL); // thread to send server messages
    
    char message[MAXSEND];
    sockaddr_t clientaddr;
    client_t *client;
    while (1) {
        int nreceived = receive_message(&clientaddr, message);
        message[nreceived] = '\0';
        printf("[RECEIVED] %s\n", message);
        
        // commands that can be executed by non-registered clients
        if (message[1] == CMD_PREFIX) {
            if (message[2] == CMD_REG)
                add_to_chatroom(&clientaddr, message);
            else if (message[2] == CMD_HELP)
                send_help(&clientaddr);
        }
        // verify a client to send their messages or handle commands
        else if ((client = verify_client(message[0]))) {
            if (message[1] == CMD_PREFIX) {
                if (message[2] == CMD_ALL)
                    show_all_participants(&client->addr);
                else if (message[2] == CMD_LEAVE)
                    remove_client(client);
                else
                    unknown_client_command(&client->addr, message + 1);
                continue;
            }
            char broadcasted_msg[NAMELEN + MAXSEND + 2];
            sprintf(broadcasted_msg, "%s: %s", client->name, message + 1);
            broadcast_except(broadcasted_msg, client->id);
        }
    }
}

void on_app_close(int signum) {
    pthread_cancel(broadcast_thread);
    cleanup_socket();
}