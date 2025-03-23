#include "chatroom.h"

#define MAXMSG MAXSEND - 2

int sockfd;

char client_id = EMPTY_ID;

sockaddr_t servaddr;

pthread_t receive_thread;
pthread_mutex_t mutex;
pthread_cond_t cond_var;


void run_client();

int main(int argc, char* argv[]) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        cr_error("WSAStartup failed: %d", WSAGetLastError());
#endif
    if (argc < 2) 
        cr_error("Server IP and port in program arguments are needed");
    else if (argc > 3) 
        cr_error("Too many arguments");
    
    sockfd = cr_setup_socket(AF_INET, SOCK_STREAM, 0);
    servaddr = cr_setup_server(argv[1], atoi(argv[2])); //ip, port
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
        cr_error("Failed to connect to server");
    
    signal(SIGINT, &cr_on_app_close);
    signal(SIGTERM, &cr_on_app_close);
    
    run_client();
    return 0;
}

int receive(char *message) {
    int status = recv(sockfd, message, MAXSEND, 0);
    if (status < 0)
        cr_error("Failed to receive message");
    message[status] = '\0';
    return status;
}

void *receive_routine(void* arg) {
    char msgbuffer[MAXSEND];
    while (1) {
        pthread_mutex_lock(&mutex);
        while (client_id == EMPTY_ID) { // do not receive messages if client has no id
            pthread_cond_wait(&cond_var, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        receive(msgbuffer);

        if (msgbuffer[0] == USER_MESSAGE_START)
            printf("%s\n", msgbuffer + 1);
        else if (msgbuffer[0] == SERVER_MESSAGE_START)
            printf("[SERVER] %s\n", msgbuffer + 1);
        else
            printf("[BADCATCH] %s. User id: %c\n", msgbuffer, client_id); // should not receive these
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
    cr_send(&servaddr, register_request);
    
    char client_id_reply[MAXSEND];
    receive(client_id_reply);
    
    if (client_id_reply[0] != CMD_ID || '0' > client_id_reply[1] || '9' < client_id_reply[1]) {
        cr_warn("Bad server reply: %s", client_id_reply);
        return EMPTY_ID;
    }
    
    if (client_id_reply[1] == EMPTY_ID) 
        cr_warn("Chatroom is full. Unable to connect");
    
    return client_id_reply[1];
}

void leave_client(char *leave_command) {
    char leave_request[MAXSEND];
    snprintf(leave_request, MAXSEND, "%c%s", client_id, leave_command);
    cr_send(&servaddr, leave_request);
}

void run_client() {
    cr_log("Register with !r. See !h for other commands.");
    
    char input_buffer[MAXMSG];
    while (fgets(input_buffer, MAXMSG, stdin)) {
        input_buffer[strcspn(input_buffer, "\n")] = '\0';
        if (input_buffer[0] == CMD_PREFIX) {
            // handle register and leave commands separately from other commands
            if (input_buffer[1] == CMD_REG) {
                if (client_id != EMPTY_ID) {
                    cr_warn("Leave the chatroom with !q first if you want to register again");
                    continue;
                }
                pthread_mutex_lock(&mutex);
                client_id = register_client();
                pthread_cond_signal(&cond_var); // Signal the condition variable
                pthread_mutex_unlock(&mutex);

                pthread_create(&receive_thread, NULL, receive_routine, NULL);
            }
            else if (input_buffer[1] == CMD_LEAVE) {
                if (client_id == EMPTY_ID) {
                    cr_warn("Can't leave the chatroom as you're already not in it");
                    continue;
                }
                leave_client(input_buffer);
                client_id = EMPTY_ID;
                pthread_cancel(receive_thread);
            }
            continue;
        }
        char message[MAXSEND];
        sprintf(message, "%c%s", client_id, input_buffer);
        cr_send(&servaddr, message);
    }
}

void cr_on_app_close(int signum) {
    pthread_cancel(receive_thread);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_var); // Clean up condition variable

    cr_cleanup_socket();
}
