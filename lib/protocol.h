#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

//  HDR : BODY
//  HDR ->  VERSION | TYPE | PSTATE | GSTATE | BLEN 
//     VERSION : int default version is 0
//     TYPE    : int see enum below
//     PSTATE  : <V0,V1,V2,V3>  
//                 V0:    int
//                 V1:    int 
//                 V2:    int   
//                 V3:    int
//     GSTATE   : <V0,V1,V2> 
//                 V0:   int
//                 V1:  int
//                 V2:  int
//     BLEN     : int length of body

#define PROTOCOL_BASE_VERSION 0

typedef enum  {

  // Requests
  PROTO_MT_REQ_BASE_RESERVED_FIRST,
  PROTO_MT_REQ_BASE_HELLO,
  PROTO_MT_REQ_BASE_MOVE,
  PROTO_MT_REQ_BASE_GOODBYE,
  // RESERVED LAST REQ MT PUT ALL NEW REQ MTS ABOVE
  PROTO_MT_REQ_BASE_RESERVED_LAST,
  
  // Replys
  PROTO_MT_REP_BASE_RESERVED_FIRST,
  PROTO_MT_REP_BASE_HELLO,
  PROTO_MT_REP_BASE_MOVE,
  PROTO_MT_REP_BASE_GOODBYE,
  // RESERVED LAST REP MT PUT ALL NEW REP MTS ABOVE
  PROTO_MT_REP_BASE_RESERVED_LAST,

  // Events  
  PROTO_MT_EVENT_BASE_RESERVED_FIRST,
  PROTO_MT_EVENT_BASE_UPDATE,
  PROTO_MT_EVENT_BASE_RESERVED_LAST

} Proto_Msg_Types;

typedef enum  {

  PLAYER_S,
  PLAYER_X,
  PLAYER_O,
  PLAYER_EMPTY

} Player_Types;

typedef enum  {

  INVALID_MOVE,
  NOT_YOUR_TURN,
  SUCCESS

} Move_Return_Types;

typedef enum {

  WIN_O,
  WIN_X,
  TIE,
  PLAYING

} Game_Outcome;

typedef enum { PROTO_STATE_INVALID_VERSION=0, PROTO_STATE_INITIAL_VERSION=1} Proto_SVERS;

typedef union {
  unsigned long long raw;
} Proto_StateVersion;

typedef union {
  int raw;
} Proto_PV0;

typedef union {
  Player_Types raw;
} Proto_PV1;

typedef union {
  int raw;
} Proto_PV2;

typedef union {
  Game_Outcome raw;
} Proto_PV3;

typedef struct {
  Proto_PV1    playerIdentity;
  Proto_PV1    playerTurn;
  Proto_PV2    playerMove;
  Proto_PV3    v3;
} __attribute__((__packed__)) Proto_Player_State;

typedef union {
  int raw;
}  Proto_GV0;

typedef union {
  int raw;
} Proto_GV1;

typedef union {
  int raw;
} Proto_GV2;

typedef struct {
  Proto_GV0       pos1;
  Proto_GV0       pos2;
  Proto_GV0       pos3;
  Proto_GV0       pos4;
  Proto_GV0       pos5;
  Proto_GV0       pos6;
  Proto_GV0       pos7;
  Proto_GV0       pos8;
  Proto_GV0       pos9;
  Proto_GV0       gameResult;

} __attribute__((__packed__)) Proto_Game_State;

typedef struct {
  int                version;
  Proto_Msg_Types    type;
  Proto_StateVersion sver;
  Proto_Player_State pstate;
  Proto_Game_State   gstate;
  int                blen;
} __attribute__((__packed__)) Proto_Msg_Hdr;

// THE FOLLOWING IS TO MAKE SURE THAT YOU GET 
// 64bit ntohll and htonll defined from your
// host OS... only tested for OSX and LINUX
#ifdef __APPLE__
#  ifndef ntohll
#    include <libkern/OSByteOrder.h>
#    define ntohll(x) OSSwapBigToHostInt64(x)
#    define htonll(x) OSSwapHostToBigInt64(x)
#  endif
#else
#  ifndef ntohll
#    include <asm/byteorder.h>
#    define ntohll(x) __be64_to_cpu(x)
#    define htonll(x) __cpu_to_be64(x)
#  endif
#endif


#endif
