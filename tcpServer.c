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
#include "serverProps.h"
#include "childProcess.h"
int listenfd;
int fd[NUM_OF_PLAYER][2][2];    // Used to store two (2 child) ends of pipe
int pinit[NUM_OF_PLAYER] = {0}; // parent init state of pipe to child
int child_no = 0;
int child_pid[NUM_OF_PLAYER];
int child_order[NUM_OF_PLAYER] = {-1}; // 0 is first, 1 is second
char *child_username[NUM_OF_PLAYER];


int guard(int n, char *err)
{
    if (n < 0)
    {
        perror(err);
        exit(1);
    }
    return n;
}

// ham tam thoi de tra ve child no gan voi child no nay
int findCoupleChildNo(int _child_no)
{
    // return -1 mean no couple child
    if (child_no < 2)
        return -1;

    if (_child_no == 0)
        return 1;
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

void checkParentMsg(int clifd, int _child_no)
{
    //check mess from parent
    // char message[MAXLINE];
    // char *message = (char *)malloc(MAXLINE);
    char message[MAXLINE];
    ssize_t r = read(fd[_child_no][0][0], message, MAXLINE);

    if (r > 0)
    {
        // long strleng = strlen(message);
        message[strlen(message)] = '\0'; // string ends with '\0
        printf("-- Child %d received from parent: %s\n", _child_no, message);
        int sentBytes = send(clifd, message, strlen(message), 0);
        if (sentBytes < 0)
        {
            perror("-- Send result string error");
        }
    }
}
void childProcqess(int clifd, struct sockaddr_in cliaddr, int _child_no)
{
    int recvBytes;
    // int sentBytes;
    char mesg[MAXLINE];
    // char res[MAXLINE];

    close(fd[_child_no][1][0]); // close parent's read
    close(fd[_child_no][0][1]); // close parent's write
    guard(fcntl(fd[_child_no][1][1], F_SETFL, fcntl(fd[_child_no][1][1], F_GETFL) | O_NONBLOCK), "-- Error on set write non-blocking on child");
    guard(fcntl(fd[_child_no][0][0], F_SETFL, fcntl(fd[_child_no][0][0], F_GETFL) | O_NONBLOCK), "-- Error on set read non-blocking on child");
    while (1)
    {
        // printf("-- %d Receiving data ...\n", _child_no);
        // children van bi block o day
        recvBytes = recv(clifd, mesg, MAXLINE, MSG_DONTWAIT);

        if (recvBytes < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
            {
                checkParentMsg(clifd, _child_no);
                continue;
            }
            perror("-- Error when recv()! ");
        }
        else if (recvBytes == 0)
        {
            printf("-- Client disconnect. Exiting...\n");
            break;
        }
        else
        {

            mesg[recvBytes] = 0;
            printf("\n\n-- %d Message from client [%s:%d]: %s\n", _child_no, inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port), mesg);

            write(fd[_child_no][1][1], mesg, strlen(mesg) + 1);

            // printf("-- %d child written to parent\n", _child_no);

            // upperize(mesg, res);

            // if (strcmp(res, "Q") == 0)
            // {

            //     printf("-- Client request to exit. Exiting\n");
            //     // close(fd[_child_no][1]);
            //     break;
            // }
            // sentBytes = send(clifd, res, strlen(res), 0);
            // if (sentBytes < 0)
            // {
            //     perror("-- Send result string error");
            // }
        }
    }

    close(fd[_child_no][1][1]);
    close(fd[_child_no][0][0]);
    close(clifd);
}
int existUsername(char *username, int child){
    for (int i = 0; i < child_no; i++)
    {
        if (i != child)
            if (strcmp(username, child_username[i]) == 0)
                return 1;
    }
    return 0;
}
void setChildOrder(int _child){
    int rand = child_pid[_child] % 2;
    child_order[_child] = rand;
    int op = findCoupleChildNo(_child);
    child_order[op] = !rand;
}
void sentSocketData(int sent_no, SocketData *socketData)
{
    char *newmsg = serialize(socketData);
    // send to apropriate child

    if (sent_no >= 0)
    {
        char temStr[20];
        // sprintf(temStr, "%d", child);

        printf("- Parent foward to %d: %s\n", sent_no, newmsg);
        write(fd[sent_no][0][1], newmsg, strlen(newmsg) + 1);
    }
}
void processSocketData(SocketData *socketData, int child)
{
    // luu client se forard data nay den. nho set bien sent_no cho ai
    // int sent_no = child;
    int couple_no = findCoupleChildNo(child);
    
    switch (socketData->command)
    {
    case CSEND_NAME:
        if (!existUsername(socketData->message, child))
        {
            child_username[child] = socketData->message;
            // SSEND_LIST_PLAYER
            char *userList = (char *)malloc(MAXLINE * sizeof(char));
            // strcat(userList, child_username[0]);
            for (int i = 0; i < child_no && i != child; i++)
            {
                strcat(userList, child_username[i]);
                strcat(userList, " ");
            };
            socketData->message = userList;
            socketData->command = SSEND_LIST_PLAYER;
            sentSocketData(child, socketData);
        }
        else
        {
            socketData->command = SREP_NAME_INVALID;
       
            // socketData->message = emptyMsg;
            sentSocketData(child, socketData);
        }

        break;
    case CSEND_OPNAME:
        socketData->command = SSEND_CHALLENGE;
        socketData->message = child_username[child];

        sentSocketData(couple_no, socketData);
        break;
    case CREP_CHALLENGE_ACCEPT:
        // gui lai order cho dua vua accept
        setChildOrder(child);
        int opCommand;
        if (child_order[child] == 0)
        {
            socketData->command = SNOTI_ORDER_FIRST;
            opCommand = SNOTI_ORDER_SECOND;
        }
        else
        {
            socketData->command = SNOTI_ORDER_SECOND;
            opCommand = SNOTI_ORDER_FIRST;
        }
        
        
        // socketData->message = emptyMsg;
        sentSocketData(child, socketData);
        
        SocketData *opSocketData = makeSocketDataObject(opCommand, socketData->message, 0, 0);
        sentSocketData(couple_no, opSocketData);
        break;
    case CREP_CHALLENGE_DENY:
        // foward cho dua kia
        socketData->command = SFORW_CHALLENGE_DENY;
        // socketData->message = emptyMsg;
        sentSocketData(couple_no, socketData);
        break;
    // case CCFED_OP:
    
    //     break;
    case SSEND_CHALLENGE:
    
        break;
    
        break;
    case SNOTI_ORDER_FIRST:
        break;
    case SNOTI_ORDER_SECOND:
        break;
    case CSENT_TURN_FIRST:
        break;
    case CSENT_TURN_SECOND:
        break;
    case SFORW_TURN_FIRST:
        break;
    case SFORW_TURN_SECOND:
        break;
    case CSENT_QUIT_FIRST:
        break;
    case CSENT_QUIT_SECOND:
        break;
    case SSENT_END_FIRST:
        break;
    case SSENT_END_SECOND:
        break;
    default:
        // sent_no = couple_no;
        break;
    };
}
void parentProcqess(int _child_no)
{
    // o day phai viet ham de doc lien tuc tu cac pipe noi den cac child
    for (int i = 0; i < _child_no; i++)
    {
        // nhung gi ham cha chi chay mot lan voi moi ham con thi nhet vao group duois
        if (!pinit[i])
        {
            // neu chua khoi tao cho child nay thi khoi tao mot lan duy nhat
            pinit[i]++;
            printf("- Parent set nb for child %d, total child: %d\n", i, _child_no);

            close(fd[i][1][1]);
            close(fd[i][0][0]);

            int fcr1 = fcntl(fd[i][1][0], F_SETFL, fcntl(fd[i][1][0], F_GETFL) | O_NONBLOCK);
            if (fcr1 < 0)
            {
                printf("- Child %d error set read nb\n", i);
                perror("- Error on set read non-blocking on parent\n");
                exit(0);
            };
            int fcr2 = fcntl(fd[i][0][1], F_SETFL, fcntl(fd[i][0][1], F_GETFL) | O_NONBLOCK);
            if (fcr2 < 0)
            {
                printf("- Child %d error set write nb\n", i);
                perror("- Error on set write non-blocking on parent\n");
                exit(0);
            };
        }

        //kiem tra lien tuc
        char message[MAXLINE];
        ssize_t r = read(fd[i][1][0], message, MAXLINE);
        // printf("- %d Parent process start waiting...\n", i);

        if (r > 0)
        {
            // long strleng = strlen(message);
            message[strlen(message)] = '\0'; // string ends with '\0

            printf("- Data received from child %d - length %ld: %s\n", i, strlen(message), message);
            SocketData *sd = deserialize(message);

            if (sd != 0)
            {
                printf("- Receive command %d, point (%d, %d) and msg: %s\n", sd->command, sd->point->x, sd->point->y, sd->message);
                
                processSocketData(sd, i);
                
            };
        }
    }
}

int main(int argc, char **argv)
{
    int connfd;
    // int recvBytes, sentBytes;
    // socklen_t len;
    // char mesg[MAXLINE];
    struct sockaddr_in servaddr, cliaddr;
    // int nno, nch;
    // char *username[NUM_OF_PLAYER];

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
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EWOULDBLOCK)
            {
                parentProcqess(child_no);
                continue;
            }
            else
            {
                perror("Error");
            }
        }
        if (child_no == NUM_OF_PLAYER)
        {
            printf("Exceed max number of player: %d. Abord connection.\n", NUM_OF_PLAYER);
            close(connfd);
            continue;
        }

        if (pipe(fd[child_no][0]) == -1)
        {
            perror("Pipe p to c Failed");
            // return 1;
        }
        if (pipe(fd[child_no][1]) == -1)
        {
            perror("Pipe c to p Failed");
            // return 1;
        }

        pid = fork();
        if (pid < 0)
        {
            perror("Fork Failed");
            // return 1;
        }
        if (pid == 0)
        {
            close(listenfd); // enable this so disable below
            printf("- Connected to client [%s:%d] by process pid = %d\n", inet_ntoa(cliaddr.sin_addr), htons(cliaddr.sin_port), getpid());
            childProcqess(connfd, cliaddr, child_no);

            // sau khi exit, trong p con se in dong nay
            printf("- Exiting process pid = %d\n", getpid());

            // close(listenfd);
            exit(0);
        }
        // vi thang con da chay vao dong if tren, co exit ơ tren nen ko bao gio chay den dong duoi
        // thang cha xe clóe connfd nay de cho thang moi connect den
        close(connfd);
        printf("- Child no: %d\n", child_no);
        child_pid[child_no] = pid;
        child_no++;

        // dua phan connect toi p con o day
        // lam ssao p cha identify cac p con duoc nhi??? dung child_no nhe
    }

    // o day can dong tat ca pipe da mo trong parent
    // close(fd[0][0]);
    // close(fd[0][1]);
    close(listenfd);
    return 0;
}
