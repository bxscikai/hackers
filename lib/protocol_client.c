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
#include "protocol_client.h"


typedef struct {
  Proto_Session rpc_session;
  Proto_Session event_session;
  pthread_t EventHandlerTid;
  Proto_MT_Handler session_lost_handler;
  Proto_MT_Handler base_event_handlers[PROTO_MT_EVENT_BASE_RESERVED_LAST 
               - PROTO_MT_EVENT_BASE_RESERVED_FIRST
               - 1];

  Proto_Game_State gameState;

} Proto_Client;

extern Proto_Session *
proto_client_rpc_session(Proto_Client_Handle ch)
{
  Proto_Client *c = ch;
  return &(c->rpc_session);
}

extern Proto_Session *
proto_client_event_session(Proto_Client_Handle ch)
{
  Proto_Client *c = ch;
  return &(c->event_session);
}

extern Proto_Game_State *
proto_client_game_state(Proto_Client_Handle ch)
{
  Proto_Client *c = ch;
  return &(c->gameState);
}

extern int
proto_client_set_session_lost_handler(Proto_Client_Handle ch, Proto_MT_Handler h)
{
  Proto_Client *c = ch;
  c->session_lost_handler = h;
  return 1;
}


extern int
proto_client_set_event_handler(Proto_Client_Handle ch, Proto_Msg_Types mt,
             Proto_MT_Handler h)
{
  int i;
  Proto_Client *c = ch;

  // Here, we can setting the event handler for Proto_Client, which contains a seession lost handler
  // and an array for handlers for each messaget type that we have, we will need to write a 
  // handler for each message type
  if (mt>PROTO_MT_EVENT_BASE_RESERVED_FIRST && 
      mt<PROTO_MT_EVENT_BASE_RESERVED_LAST) {
    i= mt;
    // ADD CODE
    c->base_event_handlers[i] = h;

    return 1;
  } 

  else {
    return -1;
  }
}

static int 
proto_client_session_lost_default_hdlr(Proto_Session *s)
{
  fprintf(stderr, "Session lost...:\n");
  proto_session_dump(s);
  return -1;
}

static int 
proto_client_event_null_handler(Proto_Session *s)
{
  fprintf(stderr, 
    "proto_client_event_null_handler: invoked for session:\n");
  proto_session_dump(s);

  return 1;
}

static void *
proto_client_event_dispatcher(void * arg)
{
  Proto_Client *c;
  Proto_Session *s;
  Proto_Msg_Types mt;
  Proto_MT_Handler hdlr;
  int i;

  pthread_detach(pthread_self());

  c = (Proto_Client *) arg;
  s = &c->event_session;

  for (;;) {
    if (proto_session_rcv_msg(s)==1) {

        fprintf(stderr, "Client: Received event from server :)\n");

      mt = proto_session_hdr_unmarshall_type(s);
      int version = proto_session_hdr_unmarshall_version(s);

    fprintf(stderr, "Receiving event message: %d version: %d\n", mt, version);

      printMessageType(mt);

      if (mt > PROTO_MT_EVENT_BASE_RESERVED_FIRST && 
    mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {

    // We are getting the handler corresponding to our message type from our protocol_client 
     hdlr = c->base_event_handlers[mt];


          fprintf(stderr, "About to execute handler: %p\n", hdlr);
         if (hdlr(s)<0) {
          fprintf(stderr, "Handler executed\n");
          goto leave;
         }


      }
    } 
    else {
      // ADD CODE
      fprintf(stderr, "failed to receive event message from server\n");

      goto leave;
    }
  }
 leave:
  close(s->fd);
  return NULL;
}


extern int
proto_client_init(Proto_Client_Handle *ch)
{
  Proto_Msg_Types mt;
  Proto_Client *c;
 
  c = (Proto_Client *)malloc(sizeof(Proto_Client));
  if (c==NULL) return -1;
  bzero(c, sizeof(Proto_Client));

  proto_client_set_session_lost_handler(c, 
              proto_client_session_lost_default_hdlr);

  // Basically goes through event message type, creates a handler and adds it to the
  // client's array of message handlers
  // TODO - define a handler for each message type
  for (mt=PROTO_MT_EVENT_BASE_RESERVED_FIRST+1;
       mt<PROTO_MT_EVENT_BASE_RESERVED_LAST; mt++) {
      proto_client_set_event_handler(c, mt, proto_client_event_null_handler);
  }


  *ch = c;
  return 1;
}

int
proto_client_connect(Proto_Client_Handle ch, char *host, PortType port)
{
  Proto_Client *c = (Proto_Client *)ch;

  if (net_setup_connection(&(c->rpc_session.fd), host, port)<0) 
    return -1;

  if (net_setup_connection(&(c->event_session.fd), host, port+1)<0) 
    return -2; 

  if (pthread_create(&(c->EventHandlerTid), NULL, 
         &proto_client_event_dispatcher, c) !=0) {
    fprintf(stderr, 
      "proto_client_init: create EventHandler thread failed\n");
    perror("proto_client_init:");
    return -3;
  }

  // MY ADD CODE
  // Subscribe to the server for events
  marshall_mtonly(&(c->event_session), PROTO_MT_EVENT_BASE_UPDATE);  
  proto_session_send_msg(&(c->event_session), 0);

  return 0;
}


static void
marshall_mtonly(Proto_Session *s, Proto_Msg_Types mt) {
  Proto_Msg_Hdr h;
  
  bzero(&h, sizeof(h));
  h.type = mt;

  // ////
  // fprintf(stderr, "Client sending these bytes:\n");
  // h.version = 20;  
  // h.gstate.v0.raw = 9;
  // print_mem(&h, sizeof(Proto_Msg_Hdr));

  // ////

  proto_session_hdr_marshall(s, &h);
  // fprintf(stderr, "Client sending these converted:\n");
  // print_mem(&s->shdr, sizeof(Proto_Msg_Hdr));

};



// all rpc's are assume to only reply only with a return code in the body
// eg.  like the null_mes
static int 
do_generic_dummy_rpc(Proto_Client_Handle ch, Proto_Msg_Types mt)
{
  int rc;
  Proto_Session *s;
  Proto_Client *c = ch;

  s = &c->rpc_session;

  // marshall
  marshall_mtonly(s, mt);  
  //??? TO DO, Marshall more fields into this send header?


  rc = proto_session_rpc(s);


  if (rc==1) {
    proto_session_body_unmarshall_int(s, 0, &rc);
  } else {
    fprintf(stderr, "Rpc execution failed...\n");
  }
  
  return rc;
}

extern int 
proto_client_hello(Proto_Client_Handle ch)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_HELLO);  
}

extern int 
proto_client_move(Proto_Client_Handle ch, char data)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_MOVE);  
}

extern int 
proto_client_goodbye(Proto_Client_Handle ch)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_GOODBYE);  
}


// Connection terminated, actually close connection
extern void killConnection(Proto_Client_Handle *c) {

  Proto_Client *client = c;
  // Cancel server thread
  pthread_cancel(client->EventHandlerTid);
  // Close rpc and event session
  close(client->rpc_session.fd);
  close(client->event_session.fd);
  fprintf(stderr, "Connection terminated\n");
}
