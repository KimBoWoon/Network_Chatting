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
    char userName[100] = {0};

    printf("닉네임을 입력해주세요! >> ");
    scanf("%s", userName);
    sprintf(name, "[%s]", userName);
    sprintf(name_message, "@@join %s\n", userName);
    write(sock, name_message, sizeof(name_message));

    while (1) {
        fgets(message, BUFSIZE, stdin);

        if (!strcmp(message, "&quit\n")) {  /* '@@out' 입력 시 종료 */
            sprintf(name_message, "[%s]님이 퇴장하셨습니다!\n", userName);
            write(sock, name_message, sizeof(name_message));
            close(sock);
            exit(0);
        } else if (message[0] == '&' && message[1] == 'l' && message[2] == 'i' && message[3] == 's' && message[4] == 't') {
//        } else if (message[0] == '@' && message[1] == '@' && message[2] == 'm' && message[3] == 'e' && message[4] == 'm' && message[5] == 'b' && message[6] == 'e' && message[7] == 'r') {
            sprintf(name_message, "@@member");
            write(sock, name_message, strlen(name_message));
        } else if (message[0] == '&' &&message[1] == 'p' && message[2] == '2' && message[3] == 'p') {
//        } else if (message[0] == '@' && message[1] == '@' && message[2] == 't' && message[3] == 'a' && message[4] == 'l' && message[5] == 'k') {
            char whisperName[NAMESIZE] = {0}, whisperMessage[NAMESIZE] = {0};
            char *temp = NULL;

            strtok(message, " ");
            temp = strtok(NULL, " ");
            strcpy(whisperName, temp);

            temp = strtok(NULL, " ");
            while (temp != NULL) {
                strcat(whisperMessage, temp);
                strcat(whisperMessage, " ");
                temp = strtok(NULL, " ");
            }

            sprintf(name_message, "@@talk %s %s %s", name, whisperName, whisperMessage);
            write(sock, name_message, strlen(name_message));
        } else if (message[0] == '&') {
//        } else if (message[0] == '@' && message[1] == '@') {
            printf("&는 command 전용 메시지 입니다!\n");
            printf("<<command list>>\n");
            printf("접속중인 사용자 : &list\n");
            printf("귓속말 보내기 : &p2p <to> <message>\n");
            printf("채팅방 퇴장 : &quit\n");
        }  else {
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
        memset(name_message, 0, sizeof(char) * (NAMESIZE + BUFSIZE));
    }
}

void error_handling(char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
