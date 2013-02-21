#ifndef __DAGAME_PROTOCOL_UTILS_H__
#define __DAGAME_PROTOCOL_UTILS_H__
/******************************************************************************
* Copyright (C) 2011 by Jonathan Appavoo, Boston University
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
*****************************************************************************/

extern int PROTO_DEBUG;
#define PROTO_PRINT_DUMPS 0

extern void proto_dump_mt(Proto_Msg_Types type);
extern void proto_dump_pstate(Proto_Player_State *ps);
extern void proto_dump_gstate(Proto_Game_State *gs);
extern void proto_dump_msghdr(Proto_Msg_Hdr *hdr);

static inline void proto_debug_on(void) { PROTO_DEBUG = 1; }
static inline void proto_debug_off(void) { PROTO_DEBUG = 0; }
static inline int  proto_debug(void) {return PROTO_DEBUG; }

static inline void printMessageType(Proto_Msg_Types type) {

  if (type==PROTO_MT_REQ_BASE_RESERVED_FIRST)
    fprintf(stderr, "PROTO_MT_REQ_BASE_RESERVED_FIRST\n");
  else if (type==PROTO_MT_REQ_BASE_HELLO)
    fprintf(stderr, "PROTO_MT_REQ_BASE_HELLO\n");
  else if (type==PROTO_MT_REQ_BASE_MOVE)
    fprintf(stderr, "PROTO_MT_REQ_BASE_MOVE\n");
  else if (type==PROTO_MT_REQ_BASE_GOODBYE)
    fprintf(stderr, "PROTO_MT_REQ_BASE_GOODBYE\n");
  else if (type==PROTO_MT_REQ_BASE_RESERVED_LAST)
    fprintf(stderr, "PROTO_MT_REQ_BASE_RESERVED_LAST\n");


  else if (type==PROTO_MT_REP_BASE_RESERVED_FIRST)
    fprintf(stderr, "PROTO_MT_REP_BASE_RESERVED_FIRST\n");
  else if (type==PROTO_MT_REP_BASE_HELLO)
    fprintf(stderr, "PROTO_MT_REP_BASE_HELLO\n");
  else if (type==PROTO_MT_REP_BASE_MOVE)
    fprintf(stderr, "PROTO_MT_REP_BASE_MOVE\n");
  else if (type==PROTO_MT_REP_BASE_GOODBYE)
    fprintf(stderr, "PROTO_MT_REP_BASE_GOODBYE\n");
  else if (type==PROTO_MT_REP_BASE_RESERVED_LAST)
    fprintf(stderr, "PROTO_MT_REP_BASE_RESERVED_LAST\n");


  else if (type==PROTO_MT_EVENT_BASE_RESERVED_FIRST)
    fprintf(stderr, "PROTO_MT_EVENT_BASE_RESERVED_FIRST\n");
  else if (type==PROTO_MT_EVENT_BASE_UPDATE)
    fprintf(stderr, "PROTO_MT_EVENT_BASE_UPDATE\n");
  else if (type==PROTO_MT_EVENT_BASE_RESERVED_LAST)
    fprintf(stderr, "PROTO_MT_EVENT_BASE_RESERVED_LAST\n");
  else 
  	fprintf(stderr, "No matching type, raw int: %d\n", type );
  

}

static inline void printHeader(Proto_Msg_Hdr *header) {
	fprintf(stderr, "---HEADER----\n");
	printMessageType(header->type);
	fprintf(stderr, "VERSION: %d  blen: %d \n", header->version, header->blen);
  fprintf(stderr, "Game State: %d %d %d %d %d %d %d %d %d\n", header->gstate.pos1.raw, header->gstate.pos2.raw, header->gstate.pos3.raw, header->gstate.pos4.raw, header->gstate.pos5.raw, header->gstate.pos6.raw, header->gstate.pos7.raw ,header->gstate.pos8.raw ,header->gstate.pos9.raw);
	fprintf(stderr, "---END OF HEADER----\n");
}

static inline char printGamePiece(int piece, int position) {
  if (piece==-1)
    return ('0' + position);
  else if (piece==1)
    return 'X';
  else if (piece==2)
    return 'O';
}

static inline void printGameBoard(Proto_Msg_Hdr *header) {

  fprintf(stderr, "\n");
  fprintf(stderr, "%c|%c|%c\n", printGamePiece(header->gstate.pos1.raw, 1), printGamePiece(header->gstate.pos2.raw, 2), printGamePiece(header->gstate.pos3.raw, 3));
  fprintf(stderr, "-----\n");
  fprintf(stderr, "%c|%c|%c\n", printGamePiece(header->gstate.pos4.raw, 4), printGamePiece(header->gstate.pos5.raw, 5), printGamePiece(header->gstate.pos6.raw, 6));
  fprintf(stderr, "-----\n");
  fprintf(stderr, "%c|%c|%c\n", printGamePiece(header->gstate.pos7.raw, 7), printGamePiece(header->gstate.pos8.raw, 8), printGamePiece(header->gstate.pos9.raw, 9));
  fprintf(stderr, "\n");

}

static inline void print_mem(void const *vp, size_t n)
{
    unsigned char const *p = vp;
    size_t i;
    for (i=0; i<n; i++)
        printf("%02x", p[i]);
    printf("\n");
};

extern void
marshall_mtonly(void *session, Proto_Msg_Types mt);

#endif
