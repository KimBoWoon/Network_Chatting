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

#define BUFSIZE 1000
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
    char name_message[NAMESIZE + BUFSIZE];
    char *userName = NULL;
    while (1) {
        fgets(message, BUFSIZE, stdin);
        if (!strcmp(message, "@@out\n")) {  /* '@@out' 입력 시 종료 */
//            sprintf(name_message, "[%s]님이 퇴장하셨습니다!\n", userName);
//            write(sock, name_message, sizeof(name_message));
            close(sock);
            exit(0);
        } else if (message[0] == '@' && message[1] == '@' && message[2] == 'j' && message[3] == 'o' && message[4] == 'i' && message[5] == 'n') {
            strtok(message, " ");
            userName = strtok(NULL, " ");
            userName[strlen(userName) - 1] = '\0';

            sprintf(name, "[%s]", userName);
            sprintf(name_message, "@@join %s\n", userName);
            write(sock, name_message, sizeof(name_message));
        } else if (message[0] == '@' && message[1] == '@' && message[2] == 'm' && message[3] == 'e' && message[4] == 'm' && message[5] == 'b' && message[6] == 'e' && message[7] == 'r') {
            sprintf(name_message, "@@member");
        } else if (message[0] == '@' && message[1] == '@' && message[2] == 't' && message[3] == 'a' && message[4] == 'l' && message[5] == 'k') {
            char whisperName[NAMESIZE] = {0}, whisperMessage[NAMESIZE] = {0};
            char *temp = NULL;

            strtok(message, " ");
            temp = strtok(NULL, " ");
            //temp[strlen(temp) - 1] = '\0';
            strcpy(whisperName, temp);

            temp = strtok(NULL, " ");
            //temp[strlen(temp) - 1] = '\0';
            strcpy(whisperMessage, temp);

            sprintf(name_message, "@@talk %s %s", whisperName, whisperMessage);
        } else {
            sprintf(name_message, "%s %s", name, message);
        }

        if (strcmp(name, "[Default]") == 0) {
            printf("@@join <name> 사용해 닉네임 설정을 해주시기 바랍니다.\n");
            continue;
        } else {
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
        memset(name_message, 0, sizeof(char) * (NAMESIZE + BUFSIZE));
    }
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
