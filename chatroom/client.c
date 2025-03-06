#include "chatroom.h"

int sockfd;

typedef struct client {
    char id;
    int logged_in;
} client_t;
client_t client; // current client

void run_client(sockaddr_t servaddr);

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
#endif
    if (argc < 2) {
        perror("Error: Server IP and port in program arguments are needed\n");
        exit(EXIT_FAILURE);
    } else if (argc > 3) {
        perror("Error: Too many arguments\n");
        exit(EXIT_FAILURE);
    }
    sockfd = setup_socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_t servaddr = setup_server(argv[1], atoi(argv[2])); //ip, port
    run_client(servaddr);
    cleanup_socket(sockfd);
    return 0;
}

void send_logged_in(sockaddr_t *to, char *message) {
    if (client.logged_in)
        send_message(to, message);
}

int receive_message(char *message) {
    int status = recv(sockfd, message, MAXMSG, 0);
    if (status < 0) {
        perror("Error: Failed to receive message\n");
        exit(EXIT_FAILURE);
    }
    message[status] = '\0';
    return status;
}

void *receive_routine(void* arg) {
    char msgbuffer[MAXMSG];
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
    
    char register_request[MAXMSG];
    sprintf(register_request, "r%s", name);
    // no need to be logged in to register
    send_message(&servaddr, register_request);
    
    char client_id_reply[MAXMSG];
    int nreceived = receive_message(client_id_reply);
    client_id_reply[nreceived] = '\0';
        
    int client_id = atoi(client_id_reply + 1);
    if (client_id == -1) {
        perror("[ERROR]: Chatroom is full. Unable to connect\n");
        exit(EXIT_FAILURE);
    }
    return client_id + '0';
}

void leave_chatroom(sockaddr_t servaddr, char client_id) {
    char quit_request[MAXMSG];
    sprintf(quit_request, "%c%c%c", client_id, CMD_PREFIX, CMD_LEAVE);
    send_logged_in(&servaddr, quit_request);
}

void get_all_participants(sockaddr_t servaddr, char client_id) {
    char all_participants_request[MAXMSG];
    sprintf(all_participants_request, "%c%c%c", client_id, CMD_PREFIX, CMD_ALL);
    send_logged_in(&servaddr, all_participants_request);
}

void run_client(sockaddr_t servaddr) {
    pthread_t receive_thread;
    char buffer[MAXMSG - 2];
    while (1) {
        fgets(buffer, MAXMSG - 2, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';
        if (buffer[0] == CMD_PREFIX) {
            if (buffer[1] == CMD_LEAVE) {
                leave_chatroom(servaddr, client.id);
                client.logged_in = 0;
                pthread_cancel(receive_thread);
            }
            else if (buffer[1] == CMD_ALL)
                get_all_participants(servaddr, client.id);
            else if (buffer[1] == CMD_REG) {
                client.id = register_client(servaddr);
                client.logged_in = 1;
                pthread_create(&receive_thread, NULL, receive_routine, NULL);
            }
            else if (buffer[1] == CMD_ID)
                printf("%c\n", client.id);
            continue;
        }
        char message[MAXMSG];
        sprintf(message, "%c%s", client.id, buffer);
        send_logged_in(&servaddr, message);
    }
}
