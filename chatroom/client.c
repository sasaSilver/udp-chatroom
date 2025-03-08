#include "chatroom.h"

#define MAXMSG MAXSEND - 2
#define EMPTY_ID '_'

int sockfd;

char client_id = EMPTY_ID;

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

int receive_message(char *message) {
    int status = recv(sockfd, message, MAXSEND, 0);
    if (status < 0)
        throw("[ERROR] Failed to receive message");
    message[status] = '\0';
    return status;
}

void *receive_routine(void* arg) {
    char msgbuffer[MAXSEND];
    while (1) {
        receive_message(msgbuffer);
        printf("%s\n", msgbuffer);
    }
}

char register_client() {
    printf("Your name: ");
    char name[NAMELEN];
    fgets(name, NAMELEN, stdin);
    name[strcspn(name, "\n")] = '\0';
    
    char register_request[MAXSEND];
    sprintf(register_request, "%c%c%c%s", client_id, CMD_PREFIX, CMD_REG, name);
    // no need to be logged in to register
    send_message(&servaddr, register_request);
    
    char client_id_reply[MAXSEND];
    receive_message(client_id_reply);
        
    int client_id = atoi(client_id_reply + 1);
    if (client_id == -1)
        throw("[ERROR] Chatroom is full. Unable to connect");
    return client_id + '0';
}

void run_client() {
    char buffer[MAXMSG];
    
    printf("Register with !r. See !h for other commands.\n");
    while (fgets(buffer, MAXMSG, stdin)) {
        buffer[strcspn(buffer, "\n")] = '\0';
        if (buffer[0] == CMD_PREFIX) {
            if (buffer[1] == CMD_REG) {
                if (client_id != EMPTY_ID) {
                    printf("[WARNING] Leave the chatroom with !q first if you want to register again.\n");
                    continue;
                }
                client_id = register_client();
                pthread_create(&receive_thread, NULL, receive_routine, NULL);
                continue;
            }
            else if (buffer[1] == CMD_LEAVE) {
                client_id = EMPTY_ID;
                pthread_cancel(receive_thread);
            }
        }
        char message[MAXSEND];
        sprintf(message, "%c%s", client_id, buffer);
        send_message(&servaddr, message);
    }
}

void on_app_close(int signum) {
    pthread_cancel(receive_thread);
    cleanup_socket();
}
