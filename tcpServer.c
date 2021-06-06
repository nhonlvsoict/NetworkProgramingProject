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

int child_num = 0;

int fd[NUM_OF_PLAYER][2][2];    // Used to store two (2 child_no) ends of pipe
int pinit[NUM_OF_PLAYER] = {0}; // parent init state of pipe to child_no
int child_pid[NUM_OF_PLAYER] = {-1};
int child_order[NUM_OF_PLAYER] = {-1}; // 0 is first, 1 is second
char *child_username[NUM_OF_PLAYER];
int ops_no[NUM_OF_PLAYER] = {-1}; // opponent no list
// them bien o day thi cung can them xu ly o sig_chld va init()

void sentSocketData(int sent_no, SocketData *socketData);
int guard(int n, char *err)
{
    if (n < 0)
    {
        perror(err);
        exit(1);
    }
    return n;
}
// void setOpponent(int _child1, int _child2){
//     ops_no[_child1] = _child2;
//     ops_no[_child2] = _child1;
// }
int findChildNoByPId(int pid)
{
    int index = 0;

    while (index < NUM_OF_PLAYER && child_pid[index] != pid)
        ++index;

    return (index == NUM_OF_PLAYER ? -1 : index);
}
int findChildNoByUsername(char *username)
{
    for (int i = 0; i < NUM_OF_PLAYER; i++)
    {
        if (child_username[i])
            if (strcmp(username, child_username[i]) == 0)
                return i;
    }
    return -1;
}
// ham tam thoi de tra ve child_no no gan voi child_no no nay
int findOpponentNo(int _child_no)
{
    // return -1 mean no couple child_no
    if (child_num < 2)
        return -1;
    if (ops_no[_child_no] == -1)
        perror("Error! No opponent found");
    return ops_no[_child_no];
}

void sig_chld(int signo)
{
    pid_t pid;
    int stat;
    pid = waitpid(-1, &stat, WNOHANG);
    printf("child_no %d terminated\n", pid);
     // giam so child_no
    
    int cno = findChildNoByPId(pid);
    int op_no = findOpponentNo(cno);
    // set ops no neu co = -1
    ops_no[cno] = -1;
    if(op_no != -1) {
        ops_no[op_no] = -1;
        child_order[op_no] = -1;
        // o day can gui tin hieu nhac thang kia thoat cmr
        char msg[1] = "0"; 
        SocketData *sd = makeSocketDataObject(SFORW_QUIT, msg, 0, 0);
        sentSocketData(op_no, sd);
    }
    // set child_pid = -1
    child_pid[cno] = -1;
    // child_order = -1
    child_order[cno] = -1;
    // child_username = null
    child_username[cno] = 0;

    // pinti
    pinit[cno] = 0;
    //fd
    close(fd[cno][0][0]);
    close(fd[cno][0][1]);
    close(fd[cno][1][0]);
    close(fd[cno][1][1]);
    
    fd[cno][0][0] = 0;
    fd[cno][0][1] = 0;
    fd[cno][1][0] = 0;
    fd[cno][1][1] = 0;
    child_num--;
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
    guard(fcntl(fd[_child_no][1][1], F_SETFL, fcntl(fd[_child_no][1][1], F_GETFL) | O_NONBLOCK), "-- Error on set write non-blocking on child_no");
    guard(fcntl(fd[_child_no][0][0], F_SETFL, fcntl(fd[_child_no][0][0], F_GETFL) | O_NONBLOCK), "-- Error on set read non-blocking on child_no");
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

            // printf("-- %d child_no written to parent\n", _child_no);

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
int existUsername(char *username, int child_no){
    for (int i = 0; i < NUM_OF_PLAYER; i++)
    {
        if (i != child_no && child_username[i])
            if ((strcmp(username, child_username[i]) == 0))
                return 1;
    }
    return 0;
}
void startGame(int _child, int op_no){
    int rand = child_pid[_child] % 2;
    child_order[_child] = rand;
    child_order[op_no] = !rand;
    ops_no[_child] = op_no;
    ops_no[op_no] = _child;
}
void sentSocketData(int sent_no, SocketData *socketData)
{
    char *newmsg = serialize(socketData);
    // send to apropriate child_no

    if (sent_no >= 0)
    {
        // char temStr[20];
        // sprintf(temStr, "%d", child_no);

        printf("- Parent foward to %d: %s\n", sent_no, newmsg);
        write(fd[sent_no][0][1], newmsg, strlen(newmsg) + 1);
    }
}
void endGame(int _child){
    child_order[_child] = -1;
    int op_no = findOpponentNo(_child);
    child_order[op_no] = -1;
    ops_no[_child] = -1;
    ops_no[op_no] = -1;
}

void processSocketData(SocketData *socketData, int child_no)
{
    // luu client se forard data nay den. nho set bien sent_no cho ai
    // // int sent_no = child_no;
    // int couple_no = findOpponentNo(child_no);
    
    switch (socketData->command)
    {
    case CSEND_NAME:
        if (!existUsername(socketData->message, child_no))
        {
            child_username[child_no] = socketData->message;
            // SSEND_LIST_PLAYER
            char *userList = (char *)malloc(MAXLINE * sizeof(char));
            // strcat(userList, child_username[0]);
            for (int i = 0; i < NUM_OF_PLAYER; i++)
            {
                if(i != child_no && child_username[i] && ops_no[i] == -1){
                    strcat(userList, child_username[i]);
                    strcat(userList, " ");
                }
            };
            socketData->message = userList;
            socketData->command = SSEND_LIST_PLAYER;
            sentSocketData(child_no, socketData);
        }
        else
        {
            socketData->command = SREP_NAME_INVALID;
       
            // socketData->message = emptyMsg;
            sentSocketData(child_no, socketData);
        }

        break;
    case CSEND_OPNAME:
    {
        int op_no = findChildNoByUsername(socketData->message);
        if (op_no != -1)
        {
            socketData->command = SSEND_CHALLENGE;
            socketData->message = child_username[child_no];

            // doan nay phai gui cho nguoi co username trong message
            sentSocketData(op_no, socketData);
        }
        else
        {
            socketData->command = SREP_OPSTATUS_UNAVAI;
            sentSocketData(child_no, socketData);
        }
        break;
    }
    case CREP_CHALLENGE_ACCEPT:
        // gui lai order cho dua vua accept
        // cho nay thang nao gui accept phai gui kem ten dua minh accepted
        // tim set op_no
        {
            int op_no = findChildNoByUsername(socketData->message);
            // setOpponent(child_no, op_no);
            startGame(child_no, op_no);
            int opCommand;
            if (child_order[child_no] == 0)
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
            sentSocketData(child_no, socketData);

            SocketData *opSocketData = makeSocketDataObject(opCommand, socketData->message, 0, 0);
            sentSocketData(op_no, opSocketData);
            break;
        }
    case CREP_CHALLENGE_DENY:
    {
        // foward cho dua kia
        int op_no = findChildNoByUsername(socketData->message);
        socketData->command = SFORW_CHALLENGE_DENY;
        // socketData->message = emptyMsg;
        sentSocketData(op_no, socketData);
        break;
    }
    case CSENT_TURN:
    {
        // foward cho dua kia
        int op_no = findOpponentNo(child_no);
        socketData->command = SFORW_TURN;
        // socketData->message = emptyMsg;
        sentSocketData(op_no, socketData);
        break;
    }
    case CREP_TURN_INVALID:
    {
        // foward cho dua kia
        int op_no = findOpponentNo(child_no);
        socketData->command = SFORW_TURN_INVALID;
        // socketData->message = emptyMsg;
        sentSocketData(op_no, socketData);
        break;
    }
    case CSENT_WIN:
    {
        // foward cho dua kia
        int op_no = ops_no[child_no];
        socketData->command = SFORW_WIN;
        // socketData->message = emptyMsg;
        sentSocketData(op_no, socketData);
        endGame(child_no);
        break;
    }

    default:
        // sent_no = couple_no;
        break;
    };
}
void parentProcqess()
{
    // o day phai viet ham de doc lien tuc tu cac pipe noi den cac child_no
    for (int i = 0; i < NUM_OF_PLAYER && child_pid[i] != -1; i++)
    {
        // nhung gi ham cha chi chay mot lan voi moi ham con thi nhet vao group duois
        // if (!pinit[i] && child_pid[i] != -1)
        // {
            
        // }

        //kiem tra lien tuc
        char message[MAXLINE];
        ssize_t r = read(fd[i][1][0], message, MAXLINE);
        // printf("- %d Parent process start waiting...\n", i);

        if (r > 0)
        {
            // long strleng = strlen(message);
            message[strlen(message)] = '\0'; // string ends with '\0

            printf("- Data received from child_no %d - length %ld: %s\n", i, strlen(message), message);
            SocketData *sd = deserialize(message);

            if (sd != 0)
            {
                printf("- Receive command %d, point (%d, %d) and msg: %s\n", sd->command, sd->point->x, sd->point->y, sd->message);
                
                processSocketData(sd, i);
                
            };
        }
    }
}
int findAvailableChildNo(){
    for(int i = 0; i < NUM_OF_PLAYER; i++){
        if(child_pid[i] == -1) return i;
    }
    return -1;
}
void init(){
    for(int i = 0; i < NUM_OF_PLAYER; i++){
        child_pid[i] = -1;
        child_order[i] = -1;
        child_username[i] = 0;
        ops_no[i] = -1;
        pinit[i] = 0;
    }
}

int main(int argc, char **argv)
{
    init();
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
                parentProcqess();
                continue;
            }
            else
            {
                perror("Error");
            }
        }
        

        if (child_num == NUM_OF_PLAYER)
        {
            printf("Exceed max number of player: %d. Abord connection.\n", NUM_OF_PLAYER);
            close(connfd);
            continue;
        }
        int _child_no = findAvailableChildNo();
        if(_child_no == -1) perror("Error with child_no no");

        if (pipe(fd[_child_no][0]) == -1)
        {
            perror("Pipe p to c Failed");
            // return 1;
        }
        if (pipe(fd[_child_no][1]) == -1)
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
            childProcqess(connfd, cliaddr, _child_no);

            // sau khi exit, trong p con se in dong nay
            printf("- Exiting process pid = %d\n", getpid());

            // close(listenfd);
            exit(0);
        }

        // neu chua khoi tao cho child_no nay thi khoi tao mot lan duy nhat
            pinit[_child_no]++;
            printf("- Parent set nb for child_no %d, total child_no: %d\n", _child_no, child_num);

            close(fd[_child_no][1][1]);
            close(fd[_child_no][0][0]);

            int fcr1 = fcntl(fd[_child_no][1][0], F_SETFL, fcntl(fd[_child_no][1][0], F_GETFL) | O_NONBLOCK);
            if (fcr1 < 0)
            {
                printf("- Child %d error set read nb\n", _child_no);
                perror("- Error on set read non-blocking on parent\n");
                exit(0);
            };
            int fcr2 = fcntl(fd[_child_no][0][1], F_SETFL, fcntl(fd[_child_no][0][1], F_GETFL) | O_NONBLOCK);
            if (fcr2 < 0)
            {
                printf("- Child %d error set write nb\n", _child_no);
                perror("- Error on set write non-blocking on parent\n");
                exit(0);
            };

        // vi thang con da chay vao dong if tren, co exit ơ tren nen ko bao gio chay den dong duoi
        // thang cha xe clóe connfd nay de cho thang moi connect den



        close(connfd);
        printf("- Child no: %d\n", _child_no);
        child_pid[_child_no] = pid;
        child_num++;

        // dua phan connect toi p con o day
        // lam ssao p cha identify cac p con duoc nhi??? dung child_num nhe
    }

    // o day can dong tat ca pipe da mo trong parent
    // close(fd[0][0]);
    // close(fd[0][1]);
    close(listenfd);
    return 0;
}
