/*
 * chat_server.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFSIZE 100

typedef struct User {
    char* name;
    char* ipAddress;
} user;

void *clnt_connection(void *arg);

void send_message(char *message, int len);

void addUser(char *message);

void error_handling(char *message);

int clnt_number = 0;
int clnt_socks[10];
pthread_mutex_t mutx;

user userList[10];

int main(int argc, char **argv) {
    int serv_sock;
    int clnt_sock;
    struct sockaddr_in serv_addr;
    struct sockaddr_in clnt_addr;
    int clnt_addr_size;
    pthread_t thread;

    user u;

    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    if (pthread_mutex_init(&mutx, NULL))
        error_handling("mutex init error");

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1) {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *) &clnt_addr, &clnt_addr_size);

        pthread_mutex_lock(&mutx);
        u.name = "name";
        u.ipAddress = inet_ntoa(clnt_addr.sin_addr);
        userList[clnt_number] = u;
        printf("%s %s\n", u.name, u.ipAddress);
        clnt_socks[clnt_number++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        pthread_create(&thread, NULL, clnt_connection, (void *) clnt_sock);
        printf("새로운 연결, 클라이언트 IP : %s \n", inet_ntoa(clnt_addr.sin_addr));
    }

    return 0;
}

void *clnt_connection(void *arg) {
    int clnt_sock = (int) arg;
    int str_len = 0;
    char message[BUFSIZE];
    int i;

    while ((str_len = read(clnt_sock, message, sizeof(message))) != 0) {
        //printf("%s", message);
        if (message[0] == '@' && message[1] == '@' && message[2] == 'j' && message[3] == 'o' && message[4] == 'i' && message[5] == 'n') {
            printf("before addUser\n");
            addUser(message);
            printf("after addUser\n");
        } else {
            send_message(message, str_len);
        }
    }

    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_number; i++) {   /* 클라이언트 연결 종료 시 */
        if (clnt_sock == clnt_socks[i]) {
            //printf("접속 종료 %s", clnt_socks[i]);
            for (; i < clnt_number - 1; i++)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_number--;
    pthread_mutex_unlock(&mutx);

    close(clnt_sock);
    return 0;
}

void addUser(char *message) {
    char temp[BUFSIZE] = { 0 };
    char *addtemp = NULL;

    strcpy(temp, message);
    strtok(temp, " ");

    addtemp = strtok(NULL, " ");
    addtemp[strlen(addtemp) - 1] = '\0';
    printf("%s님이 입장했습니다!\n", addtemp);
    userList[clnt_number - 1].name = addtemp;
    sprintf(message, "%s님이 입장하셨습니다!\n", userList[clnt_number - 1].name);
    send_message(message, strlen(message));
    memset(message, 0, sizeof(char) * BUFSIZE);
//    addtemp = strtok(NULL, " ");
//    strcpy(userList[num].ipAddress, addtemp);
}

void send_message(char *message, int len) {
    int i;
    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_number; i++)
        write(clnt_socks[i], message, len);
    pthread_mutex_unlock(&mutx);
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
