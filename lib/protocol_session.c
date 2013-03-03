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

extern void
proto_session_dump(Proto_Session *s)
{

  if (PROTO_PRINT_DUMPS==1) {

    fprintf(stderr, "Session s=%p:\n", s);
    fprintf(stderr, " fd=%d, extra=%p slen=%d, rlen=%d\n shdr:\n  ", 
      s->fd, s->extra,
      s->slen, s->rlen);
    proto_dump_msghdr(&(s->shdr));
    fprintf(stderr, " rhdr:\n  ");
    proto_dump_msghdr(&(s->rhdr));

  } 
}

extern void
proto_session_init(Proto_Session *s)
{
  if (s) bzero(s, sizeof(Proto_Session));
}

extern void
proto_session_reset_send(Proto_Session *s)
{
  bzero(&s->shdr, sizeof(s->shdr));
  s->slen = 0;
}

extern void
proto_session_reset_receive(Proto_Session *s)
{
  bzero(&s->rhdr, sizeof(s->rhdr));
  s->rlen = 0;
}

static void
proto_session_hdr_marshall_sver(Proto_Session *s, Proto_StateVersion v)
{
  s->shdr.sver.raw = htonll(v.raw);
}

static void
proto_session_hdr_unmarshall_sver(Proto_Session *s, Proto_StateVersion *v)
{
  s->rhdr.sver.raw = ntohll(s->rhdr.sver.raw);
}

static int
proto_session_hdr_unmarshall_blen(Proto_Session *s)
{
  s->rhdr.blen = ntohl(s->rhdr.blen);
}

static void
proto_session_hdr_marshall_type(Proto_Session *s, Proto_Msg_Types t)
{
  s->shdr.type = htonl(t);
}


extern int
proto_session_hdr_unmarshall_version(Proto_Session *s)
{
  s->rhdr.version = htonl(s->rhdr.version);
  return s->rhdr.version;
}

extern Proto_Msg_Types
proto_session_hdr_unmarshall_type(Proto_Session *s)
{
  s->rhdr.type = ntohl(s->rhdr.type);
  return s->rhdr.type;
}

extern void
proto_session_hdr_unmarshall(Proto_Session *s, Proto_Msg_Hdr *h)
{
  
  proto_session_hdr_unmarshall_version(s);
  proto_session_hdr_unmarshall_type(s);
  proto_session_hdr_unmarshall_sver(s, &h->sver);
  proto_session_hdr_unmarshall_blen(s);
}
   
extern void
proto_session_hdr_marshall(Proto_Session *s, Proto_Msg_Hdr *h)
{
  // ignore the version number and hard code to the version we support

  if (h->version>0)
    s->shdr.version = htonl(h->version);
  else  
    s->shdr.version = htonl(PROTOCOL_BASE_VERSION);
  proto_session_hdr_marshall_type(s, h->type);
  proto_session_hdr_marshall_sver(s, h->sver);
  // we ignore the body length as we will explicity set it
  // on the send path to the amount of body data that was
  // marshalled.
}

extern int 
proto_session_body_marshall_ll(Proto_Session *s, long long v)
{
  if (s && ((s->slen + sizeof(long long)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->sbuf + s->slen)) = htonll(v);
    s->slen+=sizeof(long long);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_ll(Proto_Session *s, int offset, long long *v)
{
  if (s && ((s->rlen - (offset + sizeof(long long))) >=0 )) {
    *v = *((long long *)(s->rbuf + offset));
    *v = htonl(*v);
    return offset + sizeof(long long);
  }
  return -1;
}

extern int 
proto_session_body_marshall_int(Proto_Session *s, int v)
{
  if (s && ((s->slen + sizeof(int)) < PROTO_SESSION_BUF_SIZE)) {
    *((int *)(s->sbuf + s->slen)) = htonl(v);
    s->slen+=sizeof(int);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_int(Proto_Session *s, int offset, int *v)
{
  if (s && ((s->rlen  - (offset + sizeof(int))) >=0 )) {
    *v = *((int *)(s->rbuf + offset));
    *v = htonl(*v);
    return offset + sizeof(int);
  }
  return -1;
}

extern int 
proto_session_body_marshall_char(Proto_Session *s, char v)
{
  if (s && ((s->slen + sizeof(char)) < PROTO_SESSION_BUF_SIZE)) {
    s->sbuf[s->slen] = v;
    s->slen+=sizeof(char);
    return 1;
  }
  return -1;
}

extern int 
proto_session_body_unmarshall_char(Proto_Session *s, int offset, char *v)
{
  if (s && ((s->rlen - (offset + sizeof(char))) >= 0)) {
    *v = s->rbuf[offset];
    return offset + sizeof(char);
  }
  return -1;
}

extern int
proto_session_body_reserve_space(Proto_Session *s, int num, char **space)
{
  if (s && ((s->slen + num) < PROTO_SESSION_BUF_SIZE)) {
    *space = &(s->sbuf[s->slen]);
    s->slen += num;
    return 1;
  }
  *space = NULL;
  return -1;
}

extern int
proto_session_body_ptr(Proto_Session *s, int offset, char **ptr)
{
  if (s && ((s->rlen - offset) > 0)) {
    *ptr = &(s->rbuf[offset]);
    return 1;
  }
  return -1;
}
      
extern int
proto_session_body_marshall_bytes(Proto_Session *s, int len, char *data)
{
  if (s && ((s->slen + len) < PROTO_SESSION_BUF_SIZE)) {
    memcpy(s->sbuf + s->slen, data, len);
    s->slen += len;
    return 1;
  }
  return -1;
}

extern int
proto_session_body_unmarshall_bytes(Proto_Session *s, int offset, int len, 
             char *data)
{
  if (s && ((s->rlen - (offset + len)) >= 0)) {
    memcpy(data, s->rbuf + offset, len);
    return offset + len;
  }
  return -1;
}

// rc < 0 on comm failures
// rc == 1 indicates comm success
extern  int
proto_session_send_msg(Proto_Session *s, int reset)
{
  s->shdr.blen = htonl(s->slen);

  // write request
  // ADD CODE
  net_writen(s->fd, &s->shdr, (int)sizeof(Proto_Msg_Hdr));

  if (s->slen>0) 
    net_writen(s->fd, &s->sbuf, (int)s->slen);
  
  if (proto_debug()) {
    fprintf(stderr, "%p: proto_session_send_msg: SENT:\n", pthread_self());
    proto_session_dump(s);
  }

  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Sending message...\n");

  // communication was successfull 
  if (reset) proto_session_reset_send(s);

  return 1;
}

extern int
proto_session_rcv_msg(Proto_Session *s)
{
  
  proto_session_reset_receive(s);

  // read reply
  ////////// ADD CODE //////////
  int bytesRead = net_readn(s->fd, &s->rhdr, sizeof(Proto_Msg_Hdr)); // Read the reply header from received message

  // Make sure we read the # of bytes we expect
  if (bytesRead<(int)sizeof(Proto_Msg_Hdr)) {
          fprintf(stderr, "%s: ERROR failed to read len: %d!=%d"
        " ... closing connection\n", __func__, bytesRead, (int)sizeof(Proto_Msg_Hdr));
          close(s->fd);
          return -1;
  }
  // Get the number of extra bytes in the blen field
  proto_session_hdr_unmarshall_blen(s);

  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Number of bytes read: %d extraBytes=%d\n", bytesRead, s-> rhdr.blen);

  // Now read the read into the reply buffer
  bytesRead = net_readn(s->fd, &s->rbuf, s->rhdr.blen);
  if ( bytesRead != s->rhdr.blen)  {
    fprintf(stderr, "%s: ERROR failed to read msg: %d!=%d"
      " .. closing connection\n" , __func__, bytesRead, s->rhdr.blen);
    close(s->fd);
    return -1;
  }

  /////////////////////

  if (proto_debug()) {
    if(PROTO_PRINT_DUMPS==1) fprintf(stderr, "%p: proto_session_rcv_msg: RCVED:\n", pthread_self());
    proto_session_dump(s);
  }

  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Successfully received message\n" );

  
  return 1;
}

extern int
proto_session_rpc(Proto_Session *s)
{
  int rc;  
  // HACK
  // fprintf(stderr, "Sent bytes:\n", );
  // print_mem(&s->shdr, sizeof(Proto_Msg_Hdr));

  rc = proto_session_send_msg(s, 0);
  rc = proto_session_rcv_msg(s);


  // fprintf(stderr, "Rcv msg returns : %d\n", rc);

  return rc;
}

