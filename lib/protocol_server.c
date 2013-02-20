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
#include "protocol_server.h"

#define PROTO_SERVER_MAX_EVENT_SUBSCRIBERS 1024

struct {
  FDType   RPCListenFD;
  PortType RPCPort;


  FDType             EventListenFD;
  PortType           EventPort;
  pthread_t          EventListenTid;
  pthread_mutex_t    EventSubscribersLock;
  int                EventLastSubscriber;
  int                EventNumSubscribers;
  FDType             EventSubscribers[PROTO_SERVER_MAX_EVENT_SUBSCRIBERS];
  Proto_Session      EventSession; 
  pthread_t          RPCListenTid;
  Proto_MT_Handler   session_lost_handler;
  Proto_MT_Handler   base_req_handlers[PROTO_MT_REQ_BASE_RESERVED_LAST - 
               PROTO_MT_REQ_BASE_RESERVED_FIRST-1];

  // Game logic
  Proto_Game_State   gameState;
  FDType             player_O;
  FDType             player_X;


} Proto_Server;

extern PortType proto_server_rpcport(void) { return Proto_Server.RPCPort; }
extern PortType proto_server_eventport(void) { return Proto_Server.EventPort; }
extern Proto_Session *
proto_server_event_session(void) 
{ 
  return &Proto_Server.EventSession; 
}

extern int
proto_server_set_session_lost_handler(Proto_MT_Handler h)
{
  Proto_Server.session_lost_handler = h;
  return 1;
}

extern int
proto_server_set_req_handler(Proto_Msg_Types mt, Proto_MT_Handler h)
{
  int i;

  if (mt>PROTO_MT_REQ_BASE_RESERVED_FIRST &&
      mt<PROTO_MT_REQ_BASE_RESERVED_LAST) {
    i = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;

  // Add handler for proto server
  Proto_Server.base_req_handlers[i] = h;

    return 1;
  } else {
    return -1;
  }
}


static int
proto_server_record_event_subscriber(int fd, int *num)
{
  int rc=-1;

  pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

  if (Proto_Server.EventLastSubscriber < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS
      && Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber]
      ==-1) {
    // ADD CODE
    // If the last eventSubscriber is empty, place the new fd in its index
    // it is to save the for loop
    Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber] = fd;
    Proto_Server.EventNumSubscribers++;
    *num = Proto_Server.EventLastSubscriber;

    rc = 1;
  } 

  else {
    int i;
    for (i=0; i< PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
      if (Proto_Server.EventSubscribers[i]==-1) {
        // ADD CODE
        Proto_Server.EventSubscribers[i] = fd;
        Proto_Server.EventNumSubscribers++;
        *num=i;
        rc=1;
      }
    }
  }

  pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);

  return rc;
}

static
void *
proto_server_event_listen(void *arg)
{
  int fd = Proto_Server.EventListenFD;
  int connfd;

  if (net_listen(fd)<0) {
    exit(-1);
  }

  for (;;) {
    // ADD CODE

    connfd = net_accept(fd);
    fprintf(stderr, "Subscriber trying to subscribe...\n");

    if (connfd < 0) {
      fprintf(stderr, "Error: EventListen accept failed (%d)\n", errno);
    } else {
      int i;
      fprintf(stderr, "EventListen: connfd=%d -> ", connfd);

      // If our subscribe queue has been maxed out, don't accept another subscriber
      if (PROTO_SERVER_MAX_EVENT_SUBSCRIBERS - Proto_Server.EventNumSubscribers <0) {
        fprintf(stderr, "oops no space for any more event subscribers\n");
        close(connfd);
      } else {

          // ADD CODE
          int LastSubscriber;
          proto_server_record_event_subscriber(connfd, &LastSubscriber);          
          Proto_Server.EventLastSubscriber = LastSubscriber;

          if (LastSubscriber == 1) {//Set Proto_Server.PlayerX to the FD
		}
	  else {//Set PlayerO}}
		}
	  
	  // Proto_Server.EventSubscribers[Proto_Server.EventNumSubscribers] = connfd;
          // Proto_Server.EventNumSubscribers++;

          fprintf(stderr, "New subscriber with subscriber num %d connfd=%d\n", Proto_Server.EventNumSubscribers, connfd);
      }
    } 
  }
} 

void
proto_server_post_event(void) 
{
  int i;
  int num;

  pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

  i = 0;
  num = Proto_Server.EventNumSubscribers;
  while (num) {

    fprintf(stderr, "Looping postEvent\n");

    Proto_Server.EventSession.fd = Proto_Server.EventSubscribers[i];
    if (Proto_Server.EventSession.fd != -1) {
      num--;

      // HACK
      if (Proto_Server.EventListenFD<0) {
          // must have lost an event connection
          close(Proto_Server.EventSession.fd);
          Proto_Server.EventSubscribers[i]=-1;
          Proto_Server.EventNumSubscribers--;
          Proto_Server.EventLastSubscriber = -1;
      } 

      // FIXME: add ack message here to ensure that game is updated 
      // correctly everywhere... at the risk of making server dependent
      // on client behaviour  (use time out to limit impact... drop
      // clients that misbehave but be carefull of introducing deadlocks
      // HACK
        setPostMessage(&Proto_Server.EventSession);
        fprintf(stderr, "Server Trying to send event update to %d\n", Proto_Server.EventSession.fd);
        int rc = proto_session_send_msg(&Proto_Server.EventSession, 0);

        if (rc<0)
          fprintf(stderr, "Failed to post event\n");
        else
          rc = proto_session_rcv_msg(&Proto_Server.EventSession);
        if (rc<0)
          fprintf(stderr, "Failed to receive ACK\n" );

        fprintf(stderr, "Reply message from post event: %s\n", Proto_Server.EventSession.rbuf);
        /////////////////////////////////////////

    }
    i++;
  }
  proto_session_reset_send(&Proto_Server.EventSession);
  pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
}

static void *
proto_server_req_dispatcher(void * arg)
{
  Proto_Session s;
  Proto_Msg_Types mt;
  Proto_MT_Handler hdlr;
  int i;
  unsigned long arg_value = (unsigned long) arg;
  
  pthread_detach(pthread_self());

  proto_session_init(&s);

  s.fd = (FDType) arg_value;

  fprintf(stderr, "proto_rpc_dispatcher: %p: Started: fd=%d\n", 
    pthread_self(), s.fd);

  for (;;) {

    fprintf(stderr, "Before server receives message\n");

    if (proto_session_rcv_msg(&s)==1) {


      // ADD CODE /////////////
      // mt = proto_session_hdr_unmarshall_type(&s);
      fprintf(stderr,"Receiving before unmarshall bytes.\n");
      //print_mem(&s.rhdr, sizeof(Proto_Msg_Hdr));

      proto_session_hdr_unmarshall(&s, &s.rhdr);

      mt = s.rhdr.type;
      fprintf(stderr,"Receiving after unmarshall bytes.\n");
      //print_mem(&s.rhdr, sizeof(Proto_Msg_Hdr));
      printHeader(&s.rhdr);
      

      if (mt > PROTO_MT_REQ_BASE_RESERVED_FIRST && mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {

       // We are getting the handler corresponding to our message type from our protocol_client 
        fprintf(stderr, "Server received rpc request, going inside server handler!\n");

        hdlr = Proto_Server.base_req_handlers[mt];

      /////////////////
        if (hdlr(&s)<0) goto leave;
      }
    } 

    else {

      fprintf(stderr, "Server: Valid message not received! Killing client thread...\n" );
      goto leave;
    }
  }
 leave:
  // ADD CODE
  Proto_Server.RPCListenTid = (pthread_t)-1;
  close(s.fd);

  return NULL;
}


  // PROTO_MT_REQ_BASE_RESERVED_FIRST,
  // PROTO_MT_REQ_BASE_HELLO,
  // PROTO_MT_REQ_BASE_MOVE,
  // PROTO_MT_REQ_BASE_GOODBYE,
  // // RESERVED LAST REQ MT PUT ALL NEW REQ MTS ABOVE
  // PROTO_MT_REQ_BASE_RESERVED_LAST,
  
  // // Replys
  // PROTO_MT_REP_BASE_RESERVED_FIRST,
  // PROTO_MT_REP_BASE_HELLO,
  // PROTO_MT_REP_BASE_MOVE,
  // PROTO_MT_REP_BASE_GOODBYE,
  // // RESERVED LAST REP MT PUT ALL NEW REP MTS ABOVE
  // PROTO_MT_REP_BASE_RESERVED_LAST,

  // // Events  
  // PROTO_MT_EVENT_BASE_RESERVED_FIRST,
  // PROTO_MT_EVENT_BASE_UPDATE,
  // PROTO_MT_EVENT_BASE_RESERVED_LAST

static
void *
proto_server_rpc_listen(void *arg)
{
  int fd = Proto_Server.RPCListenFD;
  unsigned long connfd;
  pthread_t tid;
  
  if (net_listen(fd) < 0) {
    fprintf(stderr, "Error: proto_server_rpc_listen listen failed (%d)\n", errno);
    exit(-1);
  }

  for (;;) {
    // ADD CODE
    connfd = net_accept(fd);
    if (connfd < 0) {
      fprintf(stderr, "Error: proto_server_rpc_listen accept failed (%d)\n", errno);
    } else {
      pthread_create(&tid, NULL, &proto_server_req_dispatcher,
         (void *)connfd);
    }
  }
}

extern int
proto_server_start_rpc_loop(void)
{
  if (pthread_create(&(Proto_Server.RPCListenTid), NULL, 
         &proto_server_rpc_listen, NULL) !=0) {
    fprintf(stderr, 
      "proto_server_rpc_listen: pthread_create: create RPCListen thread failed\n");
    perror("pthread_create:");
    return -3;
  }
  return 1;
}

static int 
proto_session_lost_default_handler(Proto_Session *s)
{
  fprintf(stderr, "Session lost...:\n");
  proto_session_dump(s);
  return -1;
}

static int 
proto_server_mt_null_handler(Proto_Session *s)
{
  int rc=1;
  Proto_Msg_Hdr h;
  
  fprintf(stderr, "proto_server_mt_null_handler: invoked for session:\n");
  proto_session_dump(s);

  // setup dummy reply header : set correct reply message type and 
  // everything else empty
  bzero(&h, sizeof(s));
  h.type = proto_session_hdr_unmarshall_type(s);
  h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
  proto_session_hdr_marshall(s, &h);

  // setup a dummy body that just has a return code 
  proto_session_body_marshall_int(s, 0xdeadbeef);

  rc=proto_session_send_msg(s,1);

  return rc;
}

extern int
proto_server_init(void)
{
  int i;
  int rc;

  proto_session_init(&Proto_Server.EventSession);

  // TO DO, define handler for each message type
  // Add handler for each server event
  proto_server_set_session_lost_handler(
             proto_session_lost_default_handler);
  for (i=PROTO_MT_REQ_BASE_RESERVED_FIRST+1; 
       i<PROTO_MT_REQ_BASE_RESERVED_LAST; i++) {
      proto_server_set_req_handler(i, proto_server_mt_null_handler);
        
  }


  for (i=0; i<PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
    Proto_Server.EventSubscribers[i]=-1;
  }
  Proto_Server.EventNumSubscribers=0;
  Proto_Server.EventLastSubscriber=0;
  pthread_mutex_init(&Proto_Server.EventSubscribersLock, 0);


  rc=net_setup_listen_socket(&(Proto_Server.RPCListenFD),
           &(Proto_Server.RPCPort));

  if (rc==0) { 
    fprintf(stderr, "prot_server_init: net_setup_listen_socket: FAILED for RPCPort\n");
    return -1;
  }

  Proto_Server.EventPort = Proto_Server.RPCPort + 1;

  rc=net_setup_listen_socket(&(Proto_Server.EventListenFD),
           &(Proto_Server.EventPort));

  if (rc==0) { 
    fprintf(stderr, "proto_server_init: net_setup_listen_socket: FAILED for EventPort=%d\n", 
      Proto_Server.EventPort);
    return -2;
  }

  // Create a thread which waits for clients to subscribe to
  if (pthread_create(&(Proto_Server.EventListenTid), NULL, 
         &proto_server_event_listen, NULL) !=0) {
    fprintf(stderr, 
      "proto_server_init: pthread_create: create EventListen thread failed\n");
    perror("pthread_createt:");
    return -3;
  }

  // Initialize game board
  Proto_Server.gameState.pos1.raw = -1;
  Proto_Server.gameState.pos2.raw = -1;
  Proto_Server.gameState.pos3.raw = -1;
  Proto_Server.gameState.pos4.raw = -1;
  Proto_Server.gameState.pos5.raw = -1;
  Proto_Server.gameState.pos6.raw = -1;
  Proto_Server.gameState.pos7.raw = -1;
  Proto_Server.gameState.pos8.raw = -1;
  Proto_Server.gameState.pos9.raw = -1;

  // Initialize players
  Proto_Server.player_O = -1;
  Proto_Server.player_X = -1;


  return 0;
}

//////// Event Posting /////////
extern void setPostMessage(Proto_Session *event) {

  Proto_Msg_Hdr h;  
  bzero(&h, sizeof(h));
  h.type = PROTO_MT_EVENT_BASE_UPDATE;
  h.version = 15;
  fprintf(stderr, "Sending event message: %d\n", h.type);
  proto_session_hdr_marshall(event, &h);
}

