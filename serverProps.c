#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "serverProps.h"

// int strpos(char *string, char *match)
// {
//    char *p = strstr(string, match);
//    if (p)
//       return p - string;
//    return -1;
// }

char* serialize(SocketData *socketData) 
{
    // char str[MAXLINE];
    char *str =  (char *)malloc(MAXLINE * sizeof(char));
    char temstr[] = "{\"Command\":";
    strcat(str, temstr);
    sprintf(temstr, "%d", socketData->command);
    strcat(str, temstr);
    strcat(str, ",\"Point\":\"");

    sprintf(temstr, "%d", socketData->point->x);
    strcat(str, temstr);
    strcat(str, ", ");
    sprintf(temstr, "%d", socketData->point->y);
    strcat(str, temstr);
    strcat(str, "\",\"Message\":\"");
    strcat(str, socketData->message);
    strcat(str, "\"}");
    strcat(str, "\0");
    return str;
};

SocketData* deserialize(char *msg){
    // int st = strpos(msg, ":");
    // int end = strpos(msg, ",");
    // char *tmp;
    if(!strstr(msg, "Command") || !strstr(msg, "Point") || !strstr(msg,"Message")){
        perror("Message format is not correct!");
        return 0;
    }
    
    char *pch;
    pch = strtok(msg, "\",:");
    int index = 0;
    char command[5];
    char x[5];
    char y[5];
    // char mes[MAXLINE];
    while (pch != NULL)
    {
        switch (index)
        {
        case 2:
            strcpy(command, pch);
            break;
        case 4:
            strcpy(x, pch);
            break;
        case 5:
            strcpy(y, pch);
            break;
        case 7:
            strcpy(msg, pch);
            break;
        
        default:
            break;
        };
        // printf("\n%s", pch);
        pch = strtok(NULL, "\",:");
        index++;
    }
    SocketData *sd = makeSocketDataObject(atoi(command), msg, atoi(x), atoi(y));
    return sd;
}

SocketData* makeSocketDataObject(int command, char *msg, int x, int y){
    SocketData *data = (SocketData *)malloc(sizeof(SocketData));
    data->message = (char *)malloc(sizeof(char) * MAXLINE);

    data->command = command;
    strcpy(data->message, msg);

    Point *point = (Point *)malloc(sizeof(Point));
    point->x = x;
    point->y = y;

    data->point = point;
    return data;
}