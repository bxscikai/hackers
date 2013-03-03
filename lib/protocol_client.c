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
               - PROTO_MT_REQ_BASE_RESERVED_FIRST
               - 1];
  Game game;

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

extern GameState *
proto_client_game_state(Proto_Client_Handle ch)
{
  Proto_Client *c = ch;
  return &(c->game.state);
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
  if (mt>PROTO_MT_REQ_BASE_RESERVED_FIRST && 
      mt<PROTO_MT_EVENT_BASE_RESERVED_LAST) {
    i= mt - PROTO_MT_REQ_BASE_RESERVED_FIRST -1;

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
  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Session lost...:\n");
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
  Proto_Msg_Hdr h;
  int i;

  pthread_detach(pthread_self());

  c = (Proto_Client *) arg;
  s = &c->event_session;

  for (;;) {
    if (proto_session_rcv_msg(s)==1) {

      mt = proto_session_hdr_unmarshall_type(s);  
      proto_session_hdr_unmarshall(s, &h);

      if (mt > PROTO_MT_EVENT_BASE_RESERVED_FIRST && 
    mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {

    // We are getting the handler corresponding to our message type from our protocol_client 
      mt = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
      hdlr = c->base_event_handlers[mt];

        if (hdlr(s)<0) goto leave;
         
        // Sync server game state with local game state        
        c->game.state = s->rhdr.game.state;
        // c->playerState.playerTurn.raw = s->rhdr.pstate.playerTurn.raw;

        // CHECK TO SEE IF GAME OVER

        // Reprint prompt
        // if (c->playerState.playerIdentity.raw==PLAYER_X)
        //   fprintf(stderr, "X>");
        // else if (c->playerState.playerIdentity.raw==PLAYER_O)
        //   fprintf(stderr, "O>");
        // else
        //   fprintf(stderr, "S>");                  

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
  for (mt=PROTO_MT_REQ_BASE_RESERVED_FIRST+1;
       mt<PROTO_MT_EVENT_BASE_RESERVED_LAST; mt++) {
      if (mt==PROTO_MT_REP_BASE_GOODBYE)
        proto_client_set_event_handler(c, mt, proto_server_mt_rpc_rep_goodbye_handler);
      else if (mt==PROTO_MT_REP_BASE_HELLO)
        proto_client_set_event_handler(c, mt, proto_server_mt_rpc_rep_hello_handler);
      else if (mt==PROTO_MT_EVENT_BASE_UPDATE)
        proto_client_set_event_handler(c, mt, proto_server_mt_event_update_handler);
      else if (mt==PROTO_MT_REP_BASE_MOVE)
        proto_client_set_event_handler(c, mt, proto_server_mt_rpc_rep_move_handler);
      else
        proto_client_set_event_handler(c, mt, proto_client_event_null_handler);
  }

    // Print out handlers
    // for (mt=PROTO_MT_REQ_BASE_RESERVED_FIRST+1;
    //    mt<PROTO_MT_EVENT_BASE_RESERVED_LAST; mt++) {
    //   Proto_MT_Handler handler =  c->base_event_handlers[mt];
    // fprintf(stderr, "Handler at index: %d  is: %p\n", mt, handler);
    // }
  c->game.state.status = NOT_STARTED;

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

// all rpc's are assume to only reply only with a return code in the body
// eg.  like the null_mes
static int 
do_generic_dummy_rpc(Proto_Client_Handle ch, Proto_Msg_Types mt)
{

  int rc;
  Proto_Session *s;
  Proto_Client *c = ch;

  Proto_Msg_Hdr h;
  Proto_Msg_Types reply_mt;
  Proto_MT_Handler hdlr;

  s = &c->rpc_session;
  c->rpc_session.shdr.type = mt;

  // marshall message type and send
  if (PROTO_PRINT_DUMPS==1) printHeader(&c->rpc_session.shdr);

  proto_session_hdr_marshall(s, &s->shdr);

  // execute rpc
  rc = proto_session_rpc(s);
  proto_session_hdr_unmarshall(s, &h);


  // Execute handler associated with the rpc reply
  reply_mt = s->rhdr.type;

  if (reply_mt > PROTO_MT_REQ_BASE_RESERVED_FIRST && reply_mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {

    // We are getting the handler corresponding to our message type from our protocol_client 
    reply_mt = reply_mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
    hdlr = c->base_event_handlers[reply_mt];
    rc = hdlr(s);

    // SET PLAYER IDENTITY

    
  }



  if (rc==1) {

    // print_mem(&s->rhdr, sizeof(Proto_Msg_Hdr)+4);
    proto_session_body_unmarshall_int(s, 0, &rc);
    // print_mem(&s->rhdr, sizeof(Proto_Msg_Hdr)+4);


    if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Return code: %x\n", rc);

  } 
  else {
    if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Rpc execution failed...\n");
    c->session_lost_handler(s);
    close(s->fd);    
  }

  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Ending RPC with rc = %d\n", rc);
  
  return rc;
}

extern int 
proto_client_hello(Proto_Client_Handle ch)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_HELLO);  
}

extern int 
proto_client_update(Proto_Client_Handle ch)
{
  // If the game hasn't started, don't bother
  Proto_Client *client = ch;
  if (client->game.state.status!=PLAYER1_TURN || client->game.state.status!=PLAYER2_TURN)
    return 1;

  return do_generic_dummy_rpc(ch,PROTO_MT_EVENT_REQ_UPDATE);  
}

extern int 
proto_client_move(Proto_Client_Handle ch, char data)
{
  // Proto_Client *client = ch;
  // client->rpc_session.shdr.pstate.playerMove.raw = (int) (data - '0');

  // if (client->gameState.gameResult.raw!=PLAYING) {

  //   if (client->gameState.gameResult.raw==NOT_STARTED)
  //     fprintf(stderr, "Game hasn't started yet, waiting for another player!\n");
  //   else
  //     fprintf(stderr, "Game is already over!\n");
  //   return 1;
  // }

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
  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Connection terminated\n");
}

/////////// Custom Event Handlers ///////////////
static int 
proto_server_mt_rpc_rep_goodbye_handler(Proto_Session *s)
{
  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "RPC REPLY GOODBYE: disconnecting from server okay\n");
  return -1;
}

static int 
proto_server_mt_rpc_rep_hello_handler(Proto_Session *s)
{
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr));

  // if (s->rhdr.pstate.playerIdentity.raw==PLAYER_X) {
  //   fprintf(stderr, "You are X’s\n");
  //   return 1;
  // }
  // else if (s->rhdr.pstate.playerIdentity.raw==PLAYER_O)  {
  //   fprintf(stderr, "You are O’s\n");
  //   return 2;
  // }
  // else{
  //   fprintf(stderr, "Game full, joined as spectator\n");
  // }

  return 3; // rc just needs to be >1
}

static int 
proto_server_mt_event_update_handler(Proto_Session *s)
{
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr));
  // printGameBoard(&s->rhdr.gstate);

  marshall_mtonly(s, PROTO_MT_EVENT_BASE_UPDATE);
  proto_session_send_msg(s, 0);

  return 1;
}

static int 
proto_server_mt_rpc_rep_move_handler(Proto_Session *s)
{
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr));

  // RESPOND TO PLAYER MOVE
  
  // if (s->rhdr.pstate.playerMove.raw==NOT_YOUR_TURN)
  //   fprintf(stderr, "Not your turn!\n");
  // else if (s->rhdr.pstate.playerMove.raw==INVALID_MOVE)
  //   fprintf(stderr, "Invalid move!\n");
  // else if (s->rhdr.pstate.playerMove.raw==SUCCESS)
  //   if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Move successfully!\n"); 

  return 1;
}
/////////// End of Custom Event Handlers ///////////////
