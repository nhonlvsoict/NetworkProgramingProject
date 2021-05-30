#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#define MAXLINE 512
#define SERVPORT 5501

int guard(int n, char * err) { if (n < 0) { perror(err); exit(1); } return n; }

void clearBuffer () {
    char c;
    while ((c = getchar()) != '\n' && c != EOF);
}

void removeChar(char* s, char c)
{
    int j, n = strlen(s);
    for (int i = j = 0; i < n; i++)
        if (s[i] != c)
            s[j++] = s[i];
 
    s[j] = '\0';
}

void process (int clientfd, struct sockaddr_in* servaddr) {
    char sendline[MAXLINE];
    int sentBytes, recvBytes;
    long long totalSentBytes = 0;
    char res[MAXLINE];
    int nno, nch;

    while (1) {
        printf("\nInput message (enter Q or q to quit):\n");
        if (fgets(sendline, MAXLINE, stdin) == NULL) {
            break;
        }

        removeChar(sendline, '\n');

        if (strlen(sendline) > 0) {
            printf("<-- Send to server: %s\n", sendline);
            sentBytes = send(clientfd, sendline, strlen(sendline), 0);
            if (sentBytes < 0) {
                perror("Send message error");
                return;
            }

            totalSentBytes += sentBytes;
            if (strcmp(sendline, "q") == 0 || strcmp(sendline, "Q") == 0) {
                printf("Total bytes sent: %lld\n", totalSentBytes);
                printf("Exiting...\n");
                break;
            }
            recvBytes = recv(clientfd, res, MAXLINE, 0);
            if (recvBytes < 0) {
                perror("Receive message error!\n");
                return;
            }
            res[recvBytes] = 0;
            printf("--> From Server (%s:%d): %s\n", inet_ntoa(servaddr->sin_addr),
                htons(servaddr->sin_port), res);
        }
    }

    close(clientfd);
}

int main(int argc, char **argv) {
    int clientfd;
    struct sockaddr_in servaddr, from_socket;
    socklen_t addrlen = sizeof(from_socket);
    int connectFailed;
    char* servAddr = "127.0.0.1";

    printf("creat socket\n");
    // clientfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    // int flags = guard(fcntl(clientfd, F_GETFL), "could not get flags on TCP listening socket");
    // guard(fcntl(clientfd, F_SETFL, flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(servAddr);
    servaddr.sin_port = htons(SERVPORT);

    connectFailed = connect(clientfd, (struct sockaddr_in*) &servaddr, sizeof(servaddr));
    if (connectFailed) {
        perror("Error on connect!\n");
        return 0;
    }
    process(clientfd, &servaddr);

    return 0;
}
