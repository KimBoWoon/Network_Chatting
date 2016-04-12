/*
 * chat_client.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUFSIZE 100
#define NAMESIZE 20

void *send_message(void *arg);

void *recv_message(void *arg);

void error_handling(char *message);

char name[NAMESIZE] = "[Default]";
char message[BUFSIZE];

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_result;

    if (argc != 3) {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    //sprintf(name, "[%s]", argv[3]);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    pthread_create(&snd_thread, NULL, send_message, (void *) sock);
    pthread_create(&rcv_thread, NULL, recv_message, (void *) sock);

    pthread_join(snd_thread, &thread_result);
    pthread_join(rcv_thread, &thread_result);

    close(sock);
    return 0;
}

void *send_message(void *arg) /* 메시지 전송 쓰레드 실행 함수 */
{
    int sock = (int) arg;
    int i;
    char name_message[NAMESIZE + BUFSIZE], temp[NAMESIZE];
    while (1) {
        fgets(message, BUFSIZE, stdin);
        if (!strcmp(message, "@@out\n")) {  /* 'q' 입력 시 종료 */
            close(sock);
            exit(0);
        } else if (message[0] == '@' && message[1] == '@' && message[2] == 'j' && message[3] == 'o' && message[4] == 'i' && message[5] == 'n') {
            memset(temp, 0, sizeof(char) * NAMESIZE);

            for (i = 7; message[i] != '\n'; i++)
                temp[i - 7] = message[i];

            sprintf(name, "[%s]", temp);
            sprintf(name_message, "%s", message);
            write(sock, name_message, strlen(name_message));
        } else {
            sprintf(name_message, "%s %s", name, message);
            write(sock, name_message, strlen(name_message));
        }
    }
}

void *recv_message(void *arg) /* 메시지 수신 쓰레드 실행 함수 */
{
    int sock = (int) arg;
    char name_message[NAMESIZE + BUFSIZE];
    int str_len;
    while (1) {
        str_len = read(sock, name_message, NAMESIZE + BUFSIZE - 1);
        if (str_len == -1)
            return 1;
        name_message[str_len] = 0;
        fputs(name_message, stdout);
    }
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
