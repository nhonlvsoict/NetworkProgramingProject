/* 
 * @copyright (c) 2008, SoICT - HUST
 * @author NhonLV
 * @version 1.0
 */

#ifndef __SERVERPROPS_H__
#define __SERVERPROPS_H__
// #include "serverProps.h"


#define MAXLINE 512 //255
#define SERVPORT 5501
#define NUM_OF_PLAYER 4

//Command
#define CSEND_NAME 000
// #define SREP_NAME_VALID 010 //KO CAN VI KHI VALID THI SERVER DAY LIST LUON
#define SREP_NAME_INVALID 11
#define SSEND_LIST_PLAYER 100
#define CSEND_OPNAME 110
#define SREP_OPSTATUS_UNAVAI 120
// #define SREP_OPSTATUS_AVAI 121

// #define CCFED_OP 130
#define SSEND_CHALLENGE 140
#define CREP_CHALLENGE_ACCEPT 150 // khi op accepted thi gui order cho hai ben luon
#define CREP_CHALLENGE_DENY 151
#define SFORW_CHALLENGE_DENY 160

#define SNOTI_ORDER_FIRST 200
#define SNOTI_ORDER_SECOND 201

#define CSENT_TURN 210
#define SFORW_TURN 220

#define CREP_TURN_INVALID 250
#define SFORW_TURN_INVALID 260

// #define CSENT_TURN_FIRST 210
// #define CSENT_TURN_SECOND 211
// #define SFORW_TURN_FIRST 220
// #define SFORW_TURN_SECOND 221


#define CSENT_QUIT 230
#define SFORW_QUIT 270

// #define CSENT_QUIT_FIRST 230
// #define CSENT_QUIT_SECOND 231

// #define SSENT_END_FIRST 240
// #define SSENT_END_SECOND 241

#define CSENT_WIN 240
#define SFORW_WIN 280



typedef struct {
  int x;
  int y;
} Point;

typedef struct {
  int command;
  char *message;
  Point *point;
} SocketData;

SocketData* deserialize(char *msg);
SocketData* makeSocketDataObject(int command, char *msg, int x, int y);
char* serialize(SocketData *socketData);
#endif
