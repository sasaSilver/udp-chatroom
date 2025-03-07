#include "chatroom.h"

int sockfd;

typedef struct client {
    char id;
    int logged_in;
} client_t;
client_t client; // current client

sockaddr_t servaddr;
pthread_t receive_thread;

void run_client();

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        throw("[ERROR] WSAStartup failed: %d", WSAGetLastError());
#endif
    if (argc < 2) 
        throw("[ERROR] Server IP and port in program arguments are needed");
    else if (argc > 3) 
        throw("[ERROR] Too many arguments");
    
    sockfd = setup_socket(AF_INET, SOCK_DGRAM, 0);
    servaddr = setup_server(argv[1], atoi(argv[2])); //ip, port
    
    signal(SIGINT, &on_app_close);
    signal(SIGTERM, &on_app_close);
    
    run_client();
    return 0;
}

void send_logged_in(char *message) {
    if (client.logged_in)
        send_message(&servaddr, message);
}

int receive_message(char *message) {
    int status = recv(sockfd, message, MAXMSG, 0);
    if (status < 0)
        throw("[ERROR] Failed to receive message");
    message[status] = '\0';
    return status;
}

void *receive_routine(void* arg) {
    char msgbuffer[MAXMSG];
    while (1) {
        if (!client.logged_in)
            continue;
        receive_message(msgbuffer);
        printf("%s\n", msgbuffer);
    }
}

char register_client() {
    printf("Your name: ");
    char name[NAMELEN];
    fgets(name, NAMELEN, stdin);
    name[strcspn(name, "\n")] = '\0';
    
    char register_request[MAXMSG];
    sprintf(register_request, "r%s", name);
    // no need to be logged in to register
    send_message(&servaddr, register_request);
    
    char client_id_reply[MAXMSG];
    receive_message(client_id_reply);
        
    int client_id = atoi(client_id_reply + 1);
    if (client_id == -1)
        throw("[ERROR] Chatroom is full. Unable to connect");
    return client_id + '0';
}

void leave_chatroom(char client_id) {
    char quit_request[MAXMSG];
    sprintf(quit_request, "%c%c%c", client_id, CMD_PREFIX, CMD_LEAVE);
    send_logged_in(quit_request);
}

void get_all_participants(char client_id) {
    char all_participants_request[MAXMSG];
    sprintf(all_participants_request, "%c%c%c", client_id, CMD_PREFIX, CMD_ALL);
    send_logged_in(all_participants_request);
}

void run_client() {
    char buffer[MAXMSG - 2];
    
    printf("Register with !r. See !h for other commands.\n");
    
    while (1) {
        fgets(buffer, MAXMSG - 2, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (buffer[0] == CMD_PREFIX) {
            if (buffer[1] == CMD_REG) {
                client.id = register_client();
                client.logged_in = 1;
                pthread_create(&receive_thread, NULL, receive_routine, NULL);
            }
            else if (buffer[1] == CMD_LEAVE) {
                leave_chatroom(client.id);
                pthread_cancel(receive_thread);
                client.logged_in = 0;
            }
            else if (buffer[1] == CMD_ALL)
                get_all_participants(client.id);
            else if (buffer[1] == CMD_ID)
                printf("%c\n", client.id);
            else if (buffer[1] == CMD_HELP)
                help();
            continue;
        }
        char message[MAXMSG];
        sprintf(message, "%c%s", client.id, buffer);
        send_logged_in(message);
    }
}

void on_app_close(int signum) {
    pthread_cancel(receive_thread);
    cleanup_socket(sockfd);
    if (client.logged_in)
        leave_chatroom(client.id);
}