#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <ctype.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>


#define MAXLINE 255
#define SERVPORT 5501
#define NUM_OF_PLAYER 2

int listenfd;
int fd[NUM_OF_PLAYER][2]; // Used to store two (2 child) ends of pipe
int child_no = 0;

// ham tam thoi de tra ve child no gan voi child no nay
int findCoupleChildNo(int child_no){
    if(child_no == 0) return 1;
    return 0;
}

void sig_chld(int signo)
{
    pid_t pid;
    int stat;
    pid = waitpid(-1, &stat, WNOHANG);
    printf("child %d terminated\n", pid);
    child_no--;
}

void sig_int(int signo)
{
    close(listenfd);
    exit(-1);
}

char *upperize(char *str, char *dest)
{
    char *p = str;
    char *it = dest;

    for (; *p != 0; ++p, ++it)
        *it = toupper(*p);
    *it = 0;

    return dest;
}

void childProcqess(int clifd, struct sockaddr_in cliaddr, int child_no)
{
    int recvBytes, sentBytes;
    char mesg[MAXLINE];
    char res[MAXLINE];
    
    close(fd[child_no][0]);
    while (1)
    {
        printf("\n-- Receiving data ...\n");
        recvBytes = recv(clifd, mesg, MAXLINE, 0);

        if (recvBytes < 0)
        {
            perror("-- Error! ");
        }
        else if (recvBytes == 0)
        {
            printf("-- Client disconnect. Exiting...\n");
            break;
        }
        else
        {
            mesg[recvBytes] = 0;
            printf("-- %d Message from client [%s:%d]: %s\n", child_no, inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port), mesg);

            write(fd[child_no][1], mesg, strlen(mesg)+1);

            upperize(mesg, res);
            if (strcmp(res, "Q") == 0)
            {
                
                printf("-- Client request to exit. Exiting\n");
                // close(fd[child_no][1]);
                break;
            }
            sentBytes = send(clifd, res, strlen(res), 0);
            if (sentBytes < 0)
            {
                perror("-- Send result string error");
            }
        }
    }
    
    close(fd[child_no][1]);
    close(clifd);
}

void parentProcqess()
{
    // o day phai viet ham de doc lien tuc tu cac pipe noi den cac child
    for (int i = 0; i < child_no; i++)
    {
        char message[MAXLINE];
        close(fd[i][1]);
        // dang bi block o day.
        read(fd[i][0], message, MAXLINE);
        printf("- %d Parent process start waiting...\n", i);
        long strleng = strlen(message);
        if (strleng > 0)
        {
            message[strlen(message)] = '\0'; // string ends with '\0
            // printf("- Parent process waiting...\n");
            printf("- %d Data received from child process: length %ld: %s\n\n", i, strlen(message), message);
            // wait(NULL);
        }
    }
}

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }
int main(int argc, char **argv)
{
    
    
    int connfd;
    int recvBytes, sentBytes;
    socklen_t len;
    char mesg[MAXLINE];
    struct sockaddr_in servaddr, cliaddr;
    int nno, nch;
    char* username[NUM_OF_PLAYER];

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    int flags = guard(fcntl(listenfd, F_GETFL), "could not get flags on TCP listening socket");
    guard(fcntl(listenfd, F_SETFL, flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERVPORT);
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) >= 0)
    {
        printf("Server is running at port %d\n", SERVPORT);
    }
    else
    {
        perror("Bind failed\n");
        return 0;
    }

    if (listen(listenfd, 20))
    {
        perror("Error! ");
        return 0;
    }
   

    printf("Server started!\n");
    socklen_t clientAddrLen = sizeof(cliaddr);

    signal(SIGCHLD, sig_chld);
    pid_t pid;

    while (1)
    {
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clientAddrLen);
        if (connfd < 0)
        {
            if (errno == EINTR || errno == EWOULDBLOCK|| errno == EWOULDBLOCK)
            {
                parentProcqess(child_no);
                continue;
            }
            else
            {
                perror("Error");
            }
        }

        if (pipe(fd[child_no]) == -1)
        {
            perror("Pipe Failed");
            // return 1;
        }
        child_no++;
        if(child_no > NUM_OF_PLAYER){
            printf("Exceed max number of player: %d. Abord connection.\n", NUM_OF_PLAYER);
            close(connfd);
            continue;
        }
        pid = fork();
        if (pid < 0)
        {
            perror("Fork Failed");
            // return 1;
        }
        if (pid == 0)
        {
            close(listenfd);
            printf("- Connected to client [%s:%d] by process pid = %d\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port), getpid());
            childProcqess(connfd, cliaddr, child_no - 1);

            // sau khi exit, trong p con se in dong nay
            printf("- Exiting process pid = %d\n", getpid());

            exit(0);
        }
        // vi thang con da chay vao dong if tren, co exit ơ tren nen ko bao gio chay den dong duoi
        // thang cha xe clóe connfd nay de cho thang moi connect den
        close(connfd);

        // dua phan connect toi p con o day
        // lam ssao p cha identify cac p con duoc nhi??? dung child_no nhe
        
    }

// o day can dong tat ca pipe da mo trong parent
    // close(fd[0][0]);
    // close(fd[0][1]);
    close(listenfd);
    return 0;
}
