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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>

#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_session.h"

int PROTO_DEBUG=0;

extern void
proto_dump_mt(Proto_Msg_Types type)
{
  switch (type) {
  case PROTO_MT_REQ_BASE_RESERVED_FIRST: 
    fprintf(stderr, "PROTO_MT_REQ_BASE_RESERVED_FIRST");
    break;
  case PROTO_MT_REQ_BASE_HELLO: 
    fprintf(stderr, "PROTO_MT_REQ_BASE_HELLO");
    break;
  case PROTO_MT_REQ_BASE_MOVE: 
    fprintf(stderr, "PROTO_MT_REQ_BASE_MOVE");
    break;
  case PROTO_MT_REQ_BASE_GOODBYE: 
    fprintf(stderr, "PROTO_MT_REQ_BASE_GOODBYE");
    break;
  case PROTO_MT_REQ_BASE_RESERVED_LAST: 
    fprintf(stderr, "PROTO_MT_REQ_BASE_RESERVED_LAST");
    break;
  case PROTO_MT_REP_BASE_RESERVED_FIRST: 
    fprintf(stderr, "PROTO_MT_REP_BASE_RESERVED_FIRST");
    break;
  case PROTO_MT_REP_BASE_HELLO: 
    fprintf(stderr, "PROTO_MT_REP_BASE_HELLO");
    break;
  case PROTO_MT_REP_BASE_MOVE:
    fprintf(stderr, "PROTO_MT_REP_BASE_MOVE");
    break;
  case PROTO_MT_REP_BASE_GOODBYE:
    fprintf(stderr, "PROTO_MT_REP_BASE_GOODBYE");
    break;
  case PROTO_MT_REP_BASE_RESERVED_LAST: 
    fprintf(stderr, "PROTO_MT_REP_BASE_RESERVED_LAST");
    break;
  case PROTO_MT_EVENT_BASE_RESERVED_FIRST: 
    fprintf(stderr, "PROTO_MT_EVENT_BASE_RESERVED_LAST");
    break;
  case PROTO_MT_EVENT_BASE_UPDATE: 
    fprintf(stderr, "PROTO_MT_EVENT_BASE_UPDATE");
    break;
  case PROTO_MT_EVENT_BASE_RESERVED_LAST: 
    fprintf(stderr, "PROTO_MT_EVENT_BASE_RESERVED_LAST");
    break;
  default:
    fprintf(stderr, "UNKNOWN=%d", type);
  }
}
 
extern void
proto_dump_pstate(Proto_Player_State *ps)
{
  int v0, v1, v2, v3;
  
  v0 = ntohl(ps->playerIdentity.raw);
  v1 = ntohl(ps->playerTurn.raw);
  v2 = ntohl(ps->playerMove.raw);
  v3 = ntohl(ps->v3.raw);

  fprintf(stderr, "v0=x0%x v1=0x%x v2=0x%x v3=0x%x\n",
	  v0, v1, v2, v3);
}

extern void
proto_dump_gstate(Proto_Game_State *gs)
{
  int pos1, pos2, pos3, pos4, pos5, pos6, pos7, pos8, pos9;

  pos1 = ntohl(gs->pos1.raw);
  pos2 = ntohl(gs->pos2.raw);
  pos3 = ntohl(gs->pos3.raw);
  pos4 = ntohl(gs->pos4.raw);
  pos5 = ntohl(gs->pos5.raw);
  pos6 = ntohl(gs->pos6.raw);
  pos7 = ntohl(gs->pos7.raw);
  pos8 = ntohl(gs->pos8.raw);
  pos9 = ntohl(gs->pos9.raw);

  fprintf(stderr, "pos1=0x%x pos2=0x%x pos3=0x%x pos4=0x%x pos5=0x%x pos6=0x%x pos7=0x%x pos8=0x%x pos9=0x%x\n",
	  pos1, pos2, pos3, pos4, pos5, pos6, pos7, pos8, pos9);
}

extern void
proto_dump_msghdr(Proto_Msg_Hdr *hdr)
{
  // fprintf(stderr, "ver=%d type=", ntohl(hdr->version));
  // proto_dump_mt(ntohl(hdr->type));
  // fprintf(stderr, " sver=%llx", ntohll(hdr->sver.raw));
  // fprintf(stderr, " pstate:");
  // proto_dump_pstate(&(hdr->pstate));
  // fprintf(stderr, " gstate:"); 
  // proto_dump_gstate(&(hdr->gstate));
  // fprintf(stderr, " blen=%d\n", ntohl(hdr->blen));

  if (PROTO_PRINT_DUMPS==1) {

    fprintf(stderr, "ver=%d type=", hdr->version);
    proto_dump_mt(hdr->type);
    fprintf(stderr, " sver=%llx", hdr->sver.raw);
    fprintf(stderr, " pstate:");
    proto_dump_pstate(&(hdr->pstate));
    fprintf(stderr, " gstate:"); 
    proto_dump_gstate(&(hdr->gstate));
    fprintf(stderr, " blen=%d\n", hdr->blen);

  }
}

extern void
marshall_mtonly(void *session, Proto_Msg_Types mt) {
  
  Proto_Session *s = (Proto_Session*)session;
  Proto_Msg_Hdr h; 

  

  bzero(&h, sizeof(h));
  h.type = mt;
  proto_session_hdr_marshall(s, &h);

};

