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
#define MAX_CHAR_NUM_IN_MAP 50000

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
  Proto_MT_Handler   base_req_handlers[PROTO_MT_EVENT_BASE_RESERVED_LAST - 
                     PROTO_MT_REQ_BASE_RESERVED_FIRST];
  Cell               updateCell;  

  // Game logic
  pthread_mutex_t    GameLock;
  Game game;

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
      mt<=PROTO_MT_EVENT_BASE_RESERVED_LAST) {
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
        goto leave;
      }
    }
  }

leave:
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
    if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Subscriber trying to subscribe...\n");

    if (connfd < 0) {
      if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Error: EventListen accept failed (%d)\n", errno);
    } else {
      int i;
      if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "EventListen: connfd=%d -> ", connfd);

      // If our subscribe queue has been maxed out, don't accept another subscriber
      if (PROTO_SERVER_MAX_EVENT_SUBSCRIBERS - Proto_Server.EventNumSubscribers <0) {
        if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "oops no space for any more event subscribers\n");
        close(connfd);
      } else {

          // ADD CODE
          int LastSubscriber;
          proto_server_record_event_subscriber(connfd, &LastSubscriber);          
          Proto_Server.EventLastSubscriber = LastSubscriber;

          // Proto_Server.EventSubscribers[Proto_Server.EventNumSubscribers] = connfd;
          // Proto_Server.EventNumSubscribers++;

          fprintf(stderr, "New client connected, total subscriber num %d connfd=%d\n", Proto_Server.EventNumSubscribers, connfd);
      }
    } 
  }
} 

// Post event update to specific client
static int 
postEventToFd(FDType fd, Proto_Msg_Types mt) {
  int rc;

  Proto_Server.EventSession.fd = fd;
  setPostMessage(&Proto_Server.EventSession, mt);
  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Server Trying to send event to specific client with fd = %d\n", Proto_Server.EventSession.fd);
  rc = proto_session_send_msg(&Proto_Server.EventSession, 0);  

  if (rc<0)
    if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Failed to post event\n");
  else
    rc = proto_session_rcv_msg(&Proto_Server.EventSession);

  if (rc<0)
    if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Failed to receive ACK\n" );

  proto_session_reset_send(&Proto_Server.EventSession);

    return rc;
}

void
proto_server_post_event(Proto_Msg_Types mt) 
{
  int i;
  int num;

  pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

  i = 0;
  num = Proto_Server.EventNumSubscribers;
  
  fprintf(stderr, "Broadcasting update event to %d subscribers\n", Proto_Server.EventNumSubscribers);

  // printGameState();


  while (num) {

    Proto_Server.EventSession.fd = Proto_Server.EventSubscribers[i];
    if (Proto_Server.EventSession.fd != -1) {
      num--;

      // HACK
      if (Proto_Server.EventListenFD<0) {
          // must have lost an event connection
          close(Proto_Server.EventSession.fd);
          Proto_Server.EventSubscribers[i]=-1;
          Proto_Server.EventNumSubscribers--;
          Proto_Server.session_lost_handler(&Proto_Server.EventSession);
      } 

      // FIXME: add ack message here to ensure that game is updated 
      // correctly everywhere... at the risk of making server dependent
      // on client behaviour  (use time out to limit impact... drop
      // clients that misbehave but be carefull of introducing deadlocks
      // HACK
        setPostMessage(&Proto_Server.EventSession, mt);
        if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Server Trying to send event update to %d\n", Proto_Server.EventSession.fd);
        int rc = proto_session_send_msg(&Proto_Server.EventSession, 0);

        if (rc<0)
          if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Failed to post event\n");
        else
          rc = proto_session_rcv_msg(&Proto_Server.EventSession);

        if (rc<0)
          if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Failed to receive ACK\n" );

        if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Received ACK from client\n");
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

  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "proto_rpc_dispatcher: %p: Started: fd=%d\n", 
    
  pthread_self(), s.fd);

  for (;;) {

    if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Server waiting for rpc from fd=%d\n", s.fd);

    if (proto_session_rcv_msg(&s)==1) {


      // ADD CODE /////////////

      proto_session_hdr_unmarshall(&s, &s.rhdr);
      mt = s.rhdr.type;      

      fprintf(stderr, "Client %d sent rpc with event: ", s.fd);
      printMessageType(s.rhdr.type);

      if (mt > PROTO_MT_REQ_BASE_RESERVED_FIRST && mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {
       
       // We are getting the handler corresponding to our message type from our protocol_client 
       if (PROTO_PRINT_DUMPS==1)  fprintf(stderr, "Server received rpc request, going inside server handler!\n");
       if (PROTO_PRINT_DUMPS==1) printHeader(&s.rhdr);

        mt = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
        hdlr = Proto_Server.base_req_handlers[mt];

        if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "The handler: %p\n", hdlr);

      /////////////////
        if (hdlr(&s)<0) goto leave;
      }
    } 

    else {

      if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Server: Valid message not received! Killing client thread...\n" );
      goto leave;
    }
  }
 leave:
  // ADD CODE
  Proto_Server.session_lost_handler(&s);
  close(s.fd);
  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Thread finished\n");

  return NULL;
}




static
void *
proto_server_rpc_listen(void *arg)
{
  int fd = Proto_Server.RPCListenFD;
  unsigned long connfd;
  pthread_t tid;
  
  if (net_listen(fd) < 0) {
    if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Error: proto_server_rpc_listen listen failed (%d)\n", errno);
    exit(-1);
  }

  for (;;) {
    // ADD CODE
    connfd = net_accept(fd);
    if (connfd < 0) {
      if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Error: proto_server_rpc_listen accept failed (%d)\n", errno);
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
    if (PROTO_PRINT_DUMPS==1) fprintf(stderr, 
      "proto_server_rpc_listen: pthread_create: create RPCListen thread failed\n");
    perror("pthread_create:");
    return -3;
  }
  return 1;
}

static int 
proto_session_lost_default_handler(Proto_Session *s)
{
  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Session lost...:\n");
  proto_session_dump(s);
  return -1;
}

static int 
proto_server_mt_null_handler(Proto_Session *s)
{
  int rc=1;
  Proto_Msg_Hdr h;
  
  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "proto_server_mt_null_handler: invoked for session:\n");
  proto_session_dump(s);

  // setup dummy reply header : set correct reply message type and 
  // everything else empty
  bzero(&h, sizeof(h));
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
       i<PROTO_MT_EVENT_BASE_RESERVED_LAST; i++) {

    if (i==PROTO_MT_REQ_BASE_GOODBYE)
      proto_server_set_req_handler(i, proto_server_mt_rpc_goodbye_handler);
    else if (i==PROTO_MT_REQ_BASE_HELLO)
      proto_server_set_req_handler(i, proto_server_mt_rpc_hello_handler);
    else if (i==PROTO_MT_REQ_BASE_MOVE)
      proto_server_set_req_handler(i, proto_server_mt_rpc_move_handler);
    else if (i==PROTO_MT_REQ_BASE_PICKUP)
      proto_server_set_req_handler(i, proto_server_mt_rpc_pickup_handler);
    else if (i==PROTO_MT_EVENT_REQ_UPDATE) 
      proto_server_set_req_handler(i, proto_server_mt_rpc_update_handler);    
    else if (i==PROTO_MT_REQ_BASE_MAPQUERY) 
      proto_server_set_req_handler(i, proto_server_mt_rpc_querymap_handler);
    else if (i==PROTO_MT_EVENT_LOBBY_UPDATE) 
      proto_server_set_req_handler(i, proto_server_mt_rpc_lobby_update_handler);    
    else if (i==PROTO_MT_REQ_BASE_START_GAME) 
      proto_server_set_req_handler(i, proto_server_mt_rpc_start_game);    
    else 
      proto_server_set_req_handler(i, proto_server_mt_null_handler);
    
        
  }

    //   // Print out handlers
    // for (i=PROTO_MT_REQ_BASE_RESERVED_FIRST;
    //    i<=PROTO_MT_EVENT_BASE_RESERVED_LAST; i++) {
    //   Proto_MT_Handler handler =  Proto_Server.base_req_handlers[i];
    // fprintf(stderr, "Handler at index: %d  is: %p\n", i, handler);
    // }

  // Initial states
  Proto_Server.game.status = NOT_STARTED;

  // Init mapPosition update
  Proto_Server.updateCell.position.x = -1;
  Proto_Server.updateCell.position.y = -1;

  for (i=0; i<PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
    Proto_Server.EventSubscribers[i]=-1;
  }
  Proto_Server.EventNumSubscribers=0;
  Proto_Server.EventLastSubscriber=0;
  pthread_mutex_init(&Proto_Server.EventSubscribersLock, 0);
  pthread_mutex_init(&Proto_Server.GameLock, 0);


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

  return 0;
}


extern void printGameState() {
    NOT_YET_IMPLEMENTED
}

//////// Event Posting /////////
extern void setPostMessage(Proto_Session *event, Proto_Msg_Types mt) {

  Proto_Msg_Hdr h;  
  bzero(&h, sizeof(h));
  h.type = mt;

  if (mt==PROTO_MT_EVENT_LOBBY_UPDATE) {
    h.game = Proto_Server.game;
  }
  else if (mt==PROTO_MT_REP_BASE_START_GAME) {
   h.game = Proto_Server.game; 
  }
  else if (mt==PROTO_MT_EVENT_GAME_UPDATE) {
    h.game = Proto_Server.game;
  }
  else if (mt==PROTO_MT_EVENT_MAP_UPDATE) {
    h.updateCell = Proto_Server.updateCell;
  }
  else if (mt==PROTO_MT_EVENT_GAME_OVER_UPDATE) {
    h.game = Proto_Server.game;
  }

  proto_session_hdr_marshall(event, &h);
}


/////////// Custom Event Handlers ///////////////

// Reinit game state
static void reinitialize_State() {
  NOT_YET_IMPLEMENTED
}

// Disconnects from the calling client and sends ack to confirm disconnection
static int 
proto_server_mt_rpc_goodbye_handler(Proto_Session *s)
{
  int rc=-1;
  int i;

  // Close connection with client
  for (i=0; i<PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
    if (Proto_Server.EventSubscribers[i]==s->fd) {
      Proto_Server.EventSubscribers[i] = -1;
      if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "Removing subscriber\n");
    }
  }
  Proto_Server.EventNumSubscribers--;

    // Sending a goodbye reply
    marshall_mtonly(s, PROTO_MT_REP_BASE_GOODBYE);
    proto_session_send_msg(s, 1);

    // Telling other client the player quit
    // Proto_Server.gameState.gameResult.raw=RESIGNED;
    // proto_server_post_event();

    // Reinitialize server state
    reinitialize_State();

    fprintf(stderr, "Client disconnecting....\n");


  return rc;
}

static int getNumberOfPlayersForTeam(int team) {

    int numOfPlayers = 0;
    int i;

    if (team!=1 && team!=2)
      return 0;

    for (i=0; i<MAX_NUM_PLAYERS;i++) {
      Player currentPlayer;
      if (team==1)
        currentPlayer = Proto_Server.game.Team1_Players[i];
      else if (team==2)
        currentPlayer = Proto_Server.game.Team2_Players[i];

      if (currentPlayer.playerID>0)
        numOfPlayers++;
    }

    return numOfPlayers;
}

static void insertPlayerForTeam(int team, Player *player) {

    int i;
    for (i=0; i<MAX_NUM_PLAYERS;i++) {
      Player currentPlayer;
      if (team==1)
        currentPlayer = Proto_Server.game.Team1_Players[i];
      else if (team==2)
        currentPlayer = Proto_Server.game.Team2_Players[i];

      if (currentPlayer.playerID<=0) {
        if (team==1)
          Proto_Server.game.Team1_Players[i] = *player;
        else if (team==2)
          Proto_Server.game.Team2_Players[i] = *player;
        return;
      }
        
    }
}

static int 
proto_server_mt_rpc_start_game(Proto_Session *s)
{
  Player *player = getPlayer(&Proto_Server.game, s->fd);
  Proto_Msg_Hdr h;  
  bzero(&h, sizeof(h));
  h.type = PROTO_MT_REP_BASE_START_GAME;

  // You are not the host, reject request
  if (player->isHost==0) 
    h.returnCode = RPC_STARTGAME_NOT_HOST;
    
  // Check if we have even number of players on each side
  int numOfPlayersTeam1 = getNumberOfPlayersForTeam(1);
  int numOfPlayersTeam2 = getNumberOfPlayersForTeam(2); 
  if (numOfPlayersTeam1!=numOfPlayersTeam2) 
    h.returnCode = RPC_STARTGAME_UNEVEN_PLAYERS;  

  // Terminate early because we cannot start the game yet
  if (h.returnCode>0) {
    proto_session_hdr_marshall(s, &h);
    proto_session_send_msg(s, 1);
    return 1;
  }


  pthread_mutex_lock(&Proto_Server.GameLock); // Lock before update

  // Randomly assign each player a random location in their home region
  while (numOfPlayersTeam1!=0) {
      Cell *cell = Proto_Server.game.map.homeCells_1[getRandNum(0, Proto_Server.game.map.numHome1)];
      // We found an occupied cell, assign user this cell
      if (cell->occupied==0) {
        Player *player = &Proto_Server.game.Team1_Players[numOfPlayersTeam1-1];
        cell->occupied = 1;
        player->cellposition = cell->position;
        numOfPlayersTeam1--;
      }
  }

  while (numOfPlayersTeam2!=0) {
      Cell *cell = Proto_Server.game.map.homeCells_2[getRandNum(0, Proto_Server.game.map.numHome2)];
      // We found an occupied cell, assign user this cell
      if (cell->occupied==0) {
        Player *player = &Proto_Server.game.Team2_Players[numOfPlayersTeam2-1];
        cell->occupied = 1;
        player->cellposition = cell->position;
        numOfPlayersTeam2--;
      }
  }

  // Spawn objects
  spawnObject(JACKHAMMER1);
  spawnObject(JACKHAMMER2);
  spawnObject(FLAG_1);
  spawnObject(FLAG_2);

  Proto_Server.game.status = IN_PROGRESS;

  pthread_mutex_unlock(&Proto_Server.GameLock); // Unlock before update

  // Send reply to requesting client
  h.game = Proto_Server.game;
  proto_session_hdr_marshall(s, &h);
  proto_session_send_msg(s, 1);

  // Update all players to start game
  proto_server_post_event(PROTO_MT_REP_BASE_START_GAME);
}




static int 
proto_server_mt_rpc_hello_handler(Proto_Session *s)
{
  // Send a reply with the identity of the user
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(h));
  h.type = PROTO_MT_REP_BASE_HELLO;

  pthread_mutex_lock(&Proto_Server.GameLock); // Lock before update


  // Make sure this player hasn't joined already
  // If the user has joined already, returned corresponding error code
  if (getPlayer(&Proto_Server.game, s->fd)!=NULL) {
    h.returnCode = RPC_HELLO_ALREADYJOINED;
    proto_session_hdr_marshall(s, &h);
    proto_session_send_msg(s, 1);    
    return 1;
  }

  int numOfPlayersTeam1 = getNumberOfPlayersForTeam(1);
  int numOfPlayersTeam2 = getNumberOfPlayersForTeam(2);  

  Player newplayer;
  newplayer.isHost=0;
  newplayer.canMove=1;
  newplayer.playerID = s->fd;
  // TEMP GIVEN PLAYER JACKHAMMER
  // newplayer.inventory.type = JACKHAMMER1;
  newplayer.inventory.type = NONE;

  if (numOfPlayersTeam1==0 && numOfPlayersTeam2==0) {
    newplayer.team = TEAM_1;
    newplayer.isHost = 1;
    insertPlayerForTeam(1, &newplayer);
  }
  else if (numOfPlayersTeam1>numOfPlayersTeam2) {
    newplayer.team = TEAM_2;
    insertPlayerForTeam(2, &newplayer);
  }
  else if (numOfPlayersTeam1<numOfPlayersTeam2) {
    newplayer.team = TEAM_1;
    insertPlayerForTeam(1, &newplayer);
  }
  else if (numOfPlayersTeam1==numOfPlayersTeam2) {
    newplayer.team = TEAM_1;
    insertPlayerForTeam(1, &newplayer);
  }

  pthread_mutex_unlock(&Proto_Server.GameLock); // Unlock after update

  h.game = Proto_Server.game;
  h.version = newplayer.playerID;
  proto_session_hdr_marshall(s, &h);
  proto_session_send_msg(s, 1);

  return 1;
}

static int 
proto_server_mt_rpc_lobby_update_handler(Proto_Session *s)
{  
  proto_server_post_event(PROTO_MT_EVENT_LOBBY_UPDATE);
  return 1;
}

static int
proto_server_mt_rpc_pickup_handler(Proto_Session *s)
{
  //Reply Header
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(h));
  h.type = PROTO_MT_REP_BASE_PICKUP;

  pthread_mutex_lock(&Proto_Server.GameLock);
  
  Player *player = getPlayer(&Proto_Server.game, s->fd);

  if (player->inventory.type == NONE)//Makes sure nothing is in inventory, else a drop occurs
  {
      fprintf(stderr, "Pickup Attempt\n");
      int i = 0;
      while (i<NUMOFOBJECTS)
      {
         if (player->cellposition.x == Proto_Server.game.map.objects[i].cellposition.x && player->cellposition.y == Proto_Server.game.map.objects[i].cellposition.y)
	 {
	   if (Proto_Server.game.map.objects[i].type == FLAG_1)
           {
		if (player->team == TEAM_1)
		{
			CellType ct = Proto_Server.game.map.mapBody[player->cellposition.y][player->cellposition.x].type;
			if (ct == FLOOR_1 || ct == HOME_1 || ct == JAIL_1)
			{
				h.returnCode = RPC_PICKUP_FLAGONSIDE;
				fprintf(stderr, "Pickup Fail: Player attempted to pick up own falg on their own side");
				goto done;
			}
		}
		fprintf(stderr, "Pickup Success Item: FLAG_1\n");
	   }
 	   else if (Proto_Server.game.map.objects[i].type == FLAG_2)
           {
		if (player->team == TEAM_2)
                {
                        CellType ct = Proto_Server.game.map.mapBody[player->cellposition.y][player->cellposition.x].type;
			if (ct == FLOOR_2 || ct == HOME_2 || ct == JAIL_2)
                        {
                                h.returnCode = RPC_PICKUP_FLAGONSIDE;
                                fprintf(stderr, "Pickup Fail: Player attempted to pick up own falg on their own side");
                                goto done;
                        }
		}
		fprintf(stderr, "Pickup Success Item: FLAG_2\n");
           }
           else if (Proto_Server.game.map.objects[i].type == JACKHAMMER1)
           {
                fprintf(stderr, "Pickup Success Item: JACKHAMMER1\n");
           }
           else if (Proto_Server.game.map.objects[i].type == JACKHAMMER2)
           {
                fprintf(stderr, "Pickup Success Item: JACKHAMMER2\n");
           }

	   player->inventory.type = Proto_Server.game.map.objects[i].type;
	   h.returnCode = RPC_PICKUP_SUCCESS;

	   goto done;
	 }
	 i++;
      }
      fprintf(stderr, "Pickup Failed: Nothing to Pickup\n");
      h.returnCode = RPC_PICKUP_NOTHING;
  }
  else 
  {
      if (player->inventory.type == JACKHAMMER1)
      { 
         fprintf(stderr, "Dropped JACKHAMMER1\n");
      }
      else if(player->inventory.type == JACKHAMMER2)
      {
         fprintf(stderr, "Dropped JACKHAMMER2\n");
      } 
      else if(player->inventory.type == FLAG_1)
      {
         fprintf(stderr, "Dropped FLAG_1\n");
      }
      else if(player->inventory.type == FLAG_2)
      {
         fprintf(stderr, "Dropped FLAG_2\n");
      }

      player->inventory.type = NONE;
      h.returnCode = RPC_DROP;
  }

  done:
  pthread_mutex_unlock(&Proto_Server.GameLock); // Unlock after update


  proto_session_hdr_marshall(s, &h);
  proto_session_send_msg(s, 1);

  proto_server_post_event(PROTO_MT_EVENT_GAME_UPDATE);
      // Reset mapPosition update
    Proto_Server.updateCell.position.x = -1;
    Proto_Server.updateCell.position.y = -1;

  return 1;
}

static int 
proto_server_mt_rpc_move_handler(Proto_Session *s) {

  // Make reply header
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(h));
  h.type = PROTO_MT_REP_BASE_MOVE;

  // printUpdate(&Proto_Server.game);

  pthread_mutex_lock(&Proto_Server.GameLock); // Lock before update

  Direction dir = s->rhdr.returnCode;
  Cell *collisionCell;
  Player *player = getPlayer(&Proto_Server.game, s->fd);

  if(player->canMove == 1)
  {

  if (dir==UP) {
    fprintf(stderr, "Client %d moves UP\n", s->fd);
    collisionCell = &Proto_Server.game.map.mapBody[player->cellposition.y-1][player->cellposition.x];
  }
  else if (dir==DOWN) {
    fprintf(stderr, "Client %d moves DOWN\n", s->fd);
    collisionCell = &Proto_Server.game.map.mapBody[player->cellposition.y+1][player->cellposition.x];
  }
  else if (dir==LEFT) {
    fprintf(stderr, "Client %d moves LEFT\n", s->fd);
    collisionCell = &Proto_Server.game.map.mapBody[player->cellposition.y][player->cellposition.x-1];
  }
  else if (dir==RIGHT) {
    fprintf(stderr, "Client %d moves RIGHT\n", s->fd);          
    collisionCell = &Proto_Server.game.map.mapBody[player->cellposition.y][player->cellposition.x+1];
  }
  
  printCell(collisionCell);

  // Check for collision  
  if (collisionCell->occupied==1) {
    h.returnCode = RPC_MOVE_MOVING_INTO_PLAYER;    
    fprintf(stderr, "MOVE INTO PLAYER\n");
  }
#if CANMOVE_THROUGH_WALL
  else if (0) {
#else
  else if (collisionCell->type==WALL_FIXED || (collisionCell->type==WALL_UNFIXED && (player->inventory.type!=JACKHAMMER1 && player->inventory.type!=JACKHAMMER2))) {
#endif    
    fprintf(stderr, "MOVE INTO WALL\n");
    h.returnCode = RPC_MOVE_MOVING_INTO_WALL;
  }

  //////// JACKHAMMER USAGE /////////////
  // The user is moving into a unfixed wall with a jackhammer, break it down
  else if (collisionCell->type==WALL_UNFIXED && (player->inventory.type==JACKHAMMER1 || player->inventory.type==JACKHAMMER2)) {

    fprintf(stderr, "BREAKING DOWN WALL AT (%d,%d)\n", collisionCell->position.x, collisionCell->position.y);
    // Spawn jackhammer back to a random location in base    
    spawnObject(player->inventory.type);
    player->inventory.type = NONE;

    // Decide which floor cell to turn the wall cell to depending on the adjacent cells
    if (Proto_Server.game.map.mapBody[collisionCell->position.y-1][collisionCell->position.x].type==FLOOR_1 ||
        Proto_Server.game.map.mapBody[collisionCell->position.y+1][collisionCell->position.x].type==FLOOR_1 ||
        Proto_Server.game.map.mapBody[collisionCell->position.y][collisionCell->position.x-1].type==FLOOR_1 ||
        Proto_Server.game.map.mapBody[collisionCell->position.y][collisionCell->position.x+1].type==FLOOR_1) {
        collisionCell->type = FLOOR_1;
    }
    else {
      collisionCell->type = FLOOR_2;
    }
    Proto_Server.updateCell = *collisionCell;
    fprintf(stderr, "Cell breaking down: \n");
    printCell(&Proto_Server.updateCell);

    // Move the player into the cell that was originally the wall
    Cell *prevPos = &Proto_Server.game.map.mapBody[player->cellposition.y][player->cellposition.x];
    prevPos->occupied = 0;
    collisionCell->occupied = 1;    
    player->cellposition = collisionCell->position;

    h.returnCode = SUCCESS;
    // Broadcast map change to everyone
    proto_server_post_event(PROTO_MT_EVENT_MAP_UPDATE);    

  }

  ///////////// END OF JACKHAMMER USAGE //////////
  else {
    // Move out of current cell
    fprintf(stderr, "MOVE SUCCESSFUL\n");
    Cell *prevPos = &Proto_Server.game.map.mapBody[player->cellposition.y][player->cellposition.x];
    prevPos->occupied = 0;
    collisionCell->occupied = 1;
    
    player->cellposition = collisionCell->position;
    h.returnCode = SUCCESS;

  //Free Jail Logic
  Cell *curPos = &Proto_Server.game.map.mapBody[player->cellposition.y][player->cellposition.x]; 
  CellType pType = curPos->type;

  if (player->team == TEAM_1)
  {
	if (pType == JAIL_2)
	{
		int i = 0;
		while (i < MAX_NUM_PLAYERS)
		{
			if (Proto_Server.game.Team1_Players[i].canMove == 0)
			{
				Proto_Server.game.Team1_Players[i].canMove = 1;
				Proto_Server.game.map.mapBody[Proto_Server.game.Team1_Players[i].cellposition.y][Proto_Server.game.Team1_Players[i].cellposition.x].occupied = 0;
				while (1) 
				{
      					Cell *randCell = Proto_Server.game.map.homeCells_1[getRandNum(0, Proto_Server.game.map.numHome1)];
      				        if (randCell->occupied==0) 
					{
						Proto_Server.game.Team1_Players[i].cellposition = randCell->position;
						Proto_Server.game.map.mapBody[Proto_Server.game.Team1_Players[i].cellposition.y][Proto_Server.game.Team1_Players[i].cellposition.x].occupied = 1;
						break;
					}
				}
			}
			i++;
		}
	}	
  }
  else
  {
        if (pType == JAIL_1)
        {
                int i = 0;
                while (i < MAX_NUM_PLAYERS)
                {
                        if (Proto_Server.game.Team2_Players[i].canMove == 0)
                        {
                                Proto_Server.game.Team2_Players[i].canMove = 1;
                                Proto_Server.game.map.mapBody[Proto_Server.game.Team2_Players[i].cellposition.y][Proto_Server.game.Team2_Players[i].cellposition.x].occupied = 0;
                                while (1)
                                {
                                        Cell *randCell = Proto_Server.game.map.homeCells_2[getRandNum(0, Proto_Server.game.map.numHome2)];
                                        if (randCell->occupied==0)
                                        {
                                                Proto_Server.game.Team2_Players[i].cellposition = randCell->position;
                                                Proto_Server.game.map.mapBody[Proto_Server.game.Team2_Players[i].cellposition.y][Proto_Server.game.Team2_Players[i].cellposition.x].occupied = 1;
                                                break;
                                        }
                                }
                        }
                        i++;
                }
        }
  }

  //Tagging Logic
  if (player->team == TEAM_1)
  {
        if (pType == FLOOR_1 || pType == HOME_1 || pType == JAIL_1)
  	{
		int i = 0;
		while (i < MAX_NUM_PLAYERS)
		{
			//Up
			if (Proto_Server.game.Team2_Players[i].cellposition.x == player->cellposition.x && Proto_Server.game.Team2_Players[i].cellposition.y == (player->cellposition.y - 1))
			{
				Proto_Server.game.Team2_Players[i].canMove = 0;
				int xStart = 90;
				int yStart = 90;
				while (1)
				{
					Cell *cPos = &Proto_Server.game.map.mapBody[yStart][xStart];
					if (cPos->occupied == 0)
					{
						Proto_Server.game.map.mapBody[Proto_Server.game.Team2_Players[i].cellposition.y][Proto_Server.game.Team2_Players[i].cellposition.x].occupied = 0;
						Proto_Server.game.Team2_Players[i].cellposition = cPos->position;
						Proto_Server.game.map.mapBody[yStart][xStart].occupied = 1;
						break;
					}
					if (xStart == 97 && yStart == 108){break;} 
					if (xStart == 97)
					{
						xStart = 90;
						yStart ++;
					}
					xStart++;
				}
			}
			//Right
			if (Proto_Server.game.Team2_Players[i].cellposition.x == (player->cellposition.x + 1) && Proto_Server.game.Team2_Players[i].cellposition.y == player->cellposition.y)
                        {
                        	Proto_Server.game.Team2_Players[i].canMove = 0;
                                int xStart = 90;
                                int yStart = 90;
                                while (1)
                                {
                                        Cell *cPos = &Proto_Server.game.map.mapBody[yStart][xStart];
                                        if (cPos->occupied == 0)
                                        {
						Proto_Server.game.map.mapBody[Proto_Server.game.Team2_Players[i].cellposition.y][Proto_Server.game.Team2_Players[i].cellposition.x].occupied = 0;
                                                Proto_Server.game.Team2_Players[i].cellposition = cPos->position;
                                                Proto_Server.game.map.mapBody[yStart][xStart].occupied = 1;
                                                break;
                                        }
                                        if (xStart == 97 && yStart == 108){break;}
                                        if (xStart == 97)
                                        {
                                                xStart = 90;
                                                yStart ++;
                                        }
					xStart++;
                                }
			}
			//Down
			if (Proto_Server.game.Team2_Players[i].cellposition.x == player->cellposition.x && Proto_Server.game.Team2_Players[i].cellposition.y == (player->cellposition.y + 1))
                        {
                        	Proto_Server.game.Team2_Players[i].canMove = 0;
                                int xStart = 90;
                                int yStart = 90;
                                while (1)
                                {
                                        Cell *cPos = &Proto_Server.game.map.mapBody[yStart][xStart];
                                        if (cPos->occupied == 0)
                                        {
						Proto_Server.game.map.mapBody[Proto_Server.game.Team2_Players[i].cellposition.y][Proto_Server.game.Team2_Players[i].cellposition.x].occupied = 0;
                                                Proto_Server.game.Team2_Players[i].cellposition = cPos->position;
                                                Proto_Server.game.map.mapBody[yStart][xStart].occupied = 1;
                                                break;
                                        }
                                        if (xStart == 97 && yStart == 108){break;}
                                        if (xStart == 97)
                                        {
                                                xStart = 90;
                                                yStart ++;
                                        }
					xStart++;
                                }
			}
			//Left
			if (Proto_Server.game.Team2_Players[i].cellposition.x == (player->cellposition.x-1) && Proto_Server.game.Team2_Players[i].cellposition.y == player->cellposition.y)
                        {
                        	Proto_Server.game.Team2_Players[i].canMove = 0;
                                int xStart = 90;
                                int yStart = 90;
                                while (1)
                                {
                                        Cell *cPos = &Proto_Server.game.map.mapBody[yStart][xStart];
                                        if (cPos->occupied == 0)
                                        {
                                                Proto_Server.game.map.mapBody[Proto_Server.game.Team2_Players[i].cellposition.y][Proto_Server.game.Team2_Players[i].cellposition.x].occupied = 0;
						Proto_Server.game.Team2_Players[i].cellposition = cPos->position;
                                                Proto_Server.game.map.mapBody[yStart][xStart].occupied = 1;
                                                break;
                                        }
                                        if (xStart == 97 && yStart == 108){break;}
                                        if (xStart == 97)
                                        {
                                                xStart = 90;
                                                yStart ++;
                                        }
					xStart++;
                                }

			}	
			i++;
		}
	}
  }
  else
  {
        if (pType == FLOOR_2 || pType == HOME_2 || pType == JAIL_2)
        {
                int i = 0;
                while (i < MAX_NUM_PLAYERS)
                {
			//Up
                        if (Proto_Server.game.Team1_Players[i].cellposition.x == player->cellposition.x && Proto_Server.game.Team1_Players[i].cellposition.y == (player->cellposition.y - 1))
                        {
                                Proto_Server.game.Team1_Players[i].canMove = 0;
                                int xStart = 102;
                                int yStart = 90;
                                while (1)
                                {
                                        Cell *cPos = &Proto_Server.game.map.mapBody[yStart][xStart];
                                        if (cPos->occupied == 0)
                                        {
                                                Proto_Server.game.map.mapBody[Proto_Server.game.Team1_Players[i].cellposition.y][Proto_Server.game.Team1_Players[i].cellposition.x].occupied = 0;
                                                Proto_Server.game.Team1_Players[i].cellposition = cPos->position;
                                                Proto_Server.game.map.mapBody[yStart][xStart].occupied = 1;
                                                break;
                                        }
                                        if (xStart == 109 && yStart == 108){break;}
                                        if (xStart == 97)
                                        {
                                                xStart = 102;
                                                yStart ++;
                                        }
                                        xStart++;
                                }
                        }
			//Right
			if (Proto_Server.game.Team1_Players[i].cellposition.x == (player->cellposition.x + 1) && Proto_Server.game.Team1_Players[i].cellposition.y == player->cellposition.y)
                        {
                                Proto_Server.game.Team1_Players[i].canMove = 0;
                                int xStart = 102;
                                int yStart = 90;
                                while (1)
                                {
                                        Cell *cPos = &Proto_Server.game.map.mapBody[yStart][xStart];
                                        if (cPos->occupied == 0)
                                        {
                                                Proto_Server.game.map.mapBody[Proto_Server.game.Team1_Players[i].cellposition.y][Proto_Server.game.Team1_Players[i].cellposition.x].occupied = 0;
                                                Proto_Server.game.Team1_Players[i].cellposition = cPos->position;
                                                Proto_Server.game.map.mapBody[yStart][xStart].occupied = 1;
                                                break;
                                        }
                                        if (xStart == 109 && yStart == 108){break;}
                                        if (xStart == 97)
                                        {
                                                xStart = 102;
                                                yStart ++;
                                        }
                                        xStart++;
                                }
                        }
			//Down
			if (Proto_Server.game.Team1_Players[i].cellposition.x == player->cellposition.x && Proto_Server.game.Team1_Players[i].cellposition.y == (player->cellposition.y + 1))
                        {
                                Proto_Server.game.Team1_Players[i].canMove = 0;
                                int xStart = 102;
                                int yStart = 90;
                                while (1)
                                {
                                        Cell *cPos = &Proto_Server.game.map.mapBody[yStart][xStart];
                                        if (cPos->occupied == 0)
                                        {
                                                Proto_Server.game.map.mapBody[Proto_Server.game.Team1_Players[i].cellposition.y][Proto_Server.game.Team1_Players[i].cellposition.x].occupied = 0;
                                                Proto_Server.game.Team1_Players[i].cellposition = cPos->position;
                                                Proto_Server.game.map.mapBody[yStart][xStart].occupied = 1;
                                                break;
                                        }
                                        if (xStart == 109 && yStart == 108){break;}
                                        if (xStart == 97)
                                        {
                                                xStart = 102;
                                                yStart ++;
                                        }
                                        xStart++;
                                }
                        }
			//Left
			if (Proto_Server.game.Team1_Players[i].cellposition.x == (player->cellposition.x-1) && Proto_Server.game.Team1_Players[i].cellposition.y == player->cellposition.y)
                        {
                                Proto_Server.game.Team1_Players[i].canMove = 0;
                                int xStart = 102;
                                int yStart = 90;
                                while (1)
                                {
                                        Cell *cPos = &Proto_Server.game.map.mapBody[yStart][xStart];
                                        if (cPos->occupied == 0)
                                        {
                                                Proto_Server.game.map.mapBody[Proto_Server.game.Team1_Players[i].cellposition.y][Proto_Server.game.Team1_Players[i].cellposition.x].occupied = 0;
                                                Proto_Server.game.Team1_Players[i].cellposition = cPos->position;
                                                Proto_Server.game.map.mapBody[yStart][xStart].occupied = 1;
                                                break;
                                        }
                                        if (xStart == 109 && yStart == 108){break;}
                                        if (xStart == 97)
                                        {
                                                xStart = 102;
                                                yStart ++;
                                        }
                                        xStart++;
                                }
                        }
		i++;
		}
	}
  }

  //Object movement
  //Important note: when I decided to find out what index in objects[] the object is, I looked to find what each was hardcoded to.
  if (player->inventory.type != NONE)
  {
     if (player->inventory.type == FLAG_1)
     {
	     Proto_Server.game.map.objects[0].cellposition = player->cellposition;
     	  if (player->team == TEAM_1)
      	{
          // If the player has stepped back onto its own side, we will randomly teleport this item on their side
      		CellType ct = Proto_Server.game.map.mapBody[player->cellposition.y][player->cellposition.x].type;
      		if (ct == FLOOR_1)
      		{
            spawnObject(FLAG_1);
      			player->inventory.type = NONE;
      		}	
      	}
     }

     // If the player has flag 2
     else if (player->inventory.type == FLAG_2)
     {
	     Proto_Server.game.map.objects[1].cellposition = player->cellposition;
  
  // Teleport team 2's flag back into a random spot in its home   
	if (player->team == TEAM_2)
      {
      CellType ct = Proto_Server.game.map.mapBody[player->cellposition.y][player->cellposition.x].type;
        if (ct == FLOOR_2)
        {
          spawnObject(FLAG_2);
          player->inventory.type = NONE;
        }
      }
     }
     else if (player->inventory.type == JACKHAMMER1)     
	     Proto_Server.game.map.objects[2].cellposition = player->cellposition;     
     else if (player->inventory.type == JACKHAMMER2)
	     Proto_Server.game.map.objects[3].cellposition = player->cellposition;
  }
  }

  // Check if game over or not, if so, broadcast notification to all players
  GameStatus gameOver = checkOverGame(&Proto_Server.game);

  if (gameOver==TEAM_1_WON || gameOver==TEAM_2_WON) {
    Proto_Server.game.status = gameOver;
      proto_server_post_event(PROTO_MT_EVENT_GAME_OVER_UPDATE);
  }
  }
  else
  {
	h.returnCode = RPC_IN_JAIL;
  }

  pthread_mutex_unlock(&Proto_Server.GameLock); // Unlock after update

    
  proto_session_hdr_marshall(s, &h);
  proto_session_send_msg(s, 1);

  proto_server_post_event(PROTO_MT_EVENT_GAME_UPDATE);

  return 1;
}

// Returns the team that won
extern GameStatus checkOverGame(Game *game) {

  // Get position of both flags
  Object *flag1 = &Proto_Server.game.map.objects[0];
  Object *flag2 = &Proto_Server.game.map.objects[1];
  Cell *flag1Cell = &Proto_Server.game.map.mapBody[flag1->cellposition.y][flag1->cellposition.x];
  Cell *flag2Cell = &Proto_Server.game.map.mapBody[flag2->cellposition.y][flag2->cellposition.x];
  int i;

  // If both flags on one team's side, possible winning condition
  if ( (flag1Cell->type==FLOOR_1 || flag1Cell->type==HOME_1) && (flag2Cell->type==FLOOR_1 || flag2Cell->type==HOME_1)) {

      // Make sure no player is still holding the flag
      for (i=0; i<MAX_NUM_PLAYERS; i++) {
        Player *player = &Proto_Server.game.Team1_Players[i];
        if (player->inventory.type==FLAG_1 || player->inventory.type==FLAG_2)
          return IN_PROGRESS;
      }

      return TEAM_1_WON;
  }
  else if ( (flag1Cell->type==FLOOR_2 || flag1Cell->type==HOME_2) && (flag2Cell->type==FLOOR_2 || flag2Cell->type==HOME_2)) {

      // Make sure no player is still holding the flag
      for (i=0; i<MAX_NUM_PLAYERS; i++) {
        Player *player = &Proto_Server.game.Team2_Players[i];
        if (player->inventory.type==FLAG_1 || player->inventory.type==FLAG_2)
          return IN_PROGRESS;
      }
      return TEAM_2_WON;
  }

  return IN_PROGRESS;
}

// Spawn object random position predefined for object
static void 
spawnObject(ObjectType obj) {


    // Assign jackhammers
  if (obj==JACKHAMMER1) {

    int assignedjackhammer1 = 0;

    // Assign hammer 1
    while (assignedjackhammer1==0 ) {
      Cell *cell = Proto_Server.game.map.homeCells_1[getRandNum(0, Proto_Server.game.map.numHome1)];
      // Check to make sure no one is in this cell
      if (cell->occupied==0) {
        // Make sure this item is not overlapping another item
        int cellcontainsObj = cellContainsObject(&Proto_Server.game, cell);

        if (cellcontainsObj==NONE) {
          Object newObj;
          newObj.cellposition = cell->position;
          newObj.type = JACKHAMMER1;              
          assignedjackhammer1 = 1;
          Proto_Server.game.map.objects[2] = newObj;
        }
      }
    }
  }
  else if (obj==JACKHAMMER2) {
  
    int assignedjackhammer2 = 0;
    // Assign hammer 2
    while (assignedjackhammer2==0 ) {
      Cell *cell = Proto_Server.game.map.homeCells_2[getRandNum(0, Proto_Server.game.map.numHome2)];
      // Check to make sure no one is in this cell
      if (cell->occupied==0) {
        // Make sure this item is not overlapping another item
        int cellcontainsObj = cellContainsObject(&Proto_Server.game, cell);

        if (cellcontainsObj==NONE) {
          Object newObj;
          newObj.cellposition = cell->position;
          newObj.type = JACKHAMMER2;              
          assignedjackhammer2 = 1;
          Proto_Server.game.map.objects[3] = newObj;
        }
      }
    }
  }
  else if (obj==FLAG_1) {

    int assignedFlag1 = 0;
    int i;
    // Assign flag 1
    while (assignedFlag1==0 ) {
        Cell *cell = Proto_Server.game.map.floorCells_1[getRandNum(0, Proto_Server.game.map.numFloor1)];
        // Check to make sure no one is in this cell
        if (cell->occupied==0) {
          // Make sure this item is not overlapping another item
          int cellcontainsObj = cellContainsObject(&Proto_Server.game, cell);

          if (cellcontainsObj==NONE) {
            Object newObj;
            newObj.cellposition = cell->position;
            newObj.type = FLAG_1;              
            assignedFlag1 = 1;
            Proto_Server.game.map.objects[0] = newObj;
          }
        }
    }

  }

  else if (obj==FLAG_2) {

    int assignedFlag2 = 0;
    int i;
    // Assign flag 2
    while (assignedFlag2==0 ) {
        Cell *cell = Proto_Server.game.map.floorCells_2[getRandNum(0, Proto_Server.game.map.numFloor2)];
          // Cell *cell = Proto_Server.game.map.homeCells_1[getRandNum(0, Proto_Server.game.map.numHome1)];
 
        // Check to make sure no one is in this cell
        if (cell->occupied==0) {        
          // Make sure this item is not overlapping another item
          int cellcontainsObj = cellContainsObject(&Proto_Server.game, cell);

          if (cellcontainsObj==NONE) {
            Object newObj;
            newObj.cellposition = cell->position;
            newObj.type = FLAG_2;              
            assignedFlag2 = 1;
            Proto_Server.game.map.objects[1] = newObj;
          }
        }
    }   
  }

}



// Client requesting board update
static int 
proto_server_mt_rpc_update_handler(Proto_Session *s)
{
  int rc=1;
  postEventToFd(s->fd, PROTO_MT_EVENT_BASE_UPDATE);

  return rc;
}

static int 
proto_server_mt_rpc_querymap_handler(Proto_Session *s)
{
  // Send a reply with the map
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(h));
  h.type = PROTO_MT_REP_BASE_MAPQUERY;  

  h.game.map.dimension.x=4;
  h.game.map.dimension.y=2;
  h.game.map = Proto_Server.game.map;

  proto_session_hdr_marshall(s, &h);

  // Turn the map into a ascii string and send it to the client
  char mapStr[(Proto_Server.game.map.dimension.x+1) * Proto_Server.game.map.dimension.y];
  convertToString(&Proto_Server.game.map, mapStr);
  proto_session_hdr_marshall_mapBody(s, mapStr);
  proto_session_send_msg(s, 1);

  return 1;
}
/////////// End of Custom Event Handlers ///////////////


////////////// Newly added Capture the flag code ///////////////

extern int
proto_server_parse_map(char *filename) 
{

  FILE *fr; //file pointer    
  bzero(&Proto_Server.game.map, sizeof(Maze));

  fr = fopen (filename, "r");

   char line[MAX_LINE_LEN];
   size_t len = 0;

   if (fr == NULL)
       fprintf(stderr, "FAILED\n");

    // PASS ONE, we are trying to find the map dimensions to know how much memory to allocate
    int numOfLines = 0;
    while (fgets(line, MAX_LINE_LEN, fr) !=NULL) {
      numOfLines++;
      Proto_Server.game.map.dimension.x = (int)strlen(line)-1;      
      // fprintf(stderr, "%s", line);
    }
    Proto_Server.game.map.dimension.y = (int)numOfLines;

    // Allocate memory for the multidimensional array
    Proto_Server.game.map.mapBody = malloc(Proto_Server.game.map.dimension.y * sizeof(Cell *));
    int i;
    for(i = 0; i < Proto_Server.game.map.dimension.y; i++)
    {
      Proto_Server.game.map.mapBody[i] = malloc(Proto_Server.game.map.dimension.x * sizeof(Cell));
    }

    // fprintf(stderr, "Map dimensions: %dx%d\n", Proto_Server.game.map.dimension.x, Proto_Server.game.map.dimension.y);

   // PASS TWO, We are iterating through each line in the map and parsing the cells
   rewind(fr); 

   // File iteration loop
    numOfLines=0;
    while (fgets(line, MAX_LINE_LEN, fr) !=NULL) {              

      // We are iterating through each character in the map
      int i,j;
      for (i=0; i<Proto_Server.game.map.dimension.x; i++) {
        Cell newcell;
        newcell.occupied=0;
        newcell.position.x = i;
        newcell.position.y = numOfLines;
        char currentCell = line[i];
        newcell.type=cellTypeFromChar(currentCell);

        // Record map stats so we don't need to recompute them when queried
        if (newcell.type==HOME_1)
          Proto_Server.game.map.numHome1++;
        else if (newcell.type==HOME_2)
          Proto_Server.game.map.numHome2++;
        else if (newcell.type==JAIL_1)
          Proto_Server.game.map.numJail1++;
        else if (newcell.type==JAIL_2)
          Proto_Server.game.map.numJail2++;
        else if (newcell.type==WALL_FIXED)
          Proto_Server.game.map.numFixedWall++;
        else if (newcell.type==WALL_UNFIXED)
          Proto_Server.game.map.numNonfixedWall++;        
        // Decide if floor tile is Floor1 or Floor2
        else if (newcell.type==FLOOR_1) 
        {
          if (i<=Proto_Server.game.map.dimension.x * 0.5) { 
            Proto_Server.game.map.numFloor1++;
            newcell.type = FLOOR_1;
          }
          else if (i>Proto_Server.game.map.dimension.x*0.5) {
            Proto_Server.game.map.numFloor2++;
            newcell.type = FLOOR_2;
          }
        }


        if (currentCell!='\n')
          Proto_Server.game.map.mapBody[numOfLines][i] = newcell;
                                   

      }
      numOfLines++;
   }

  fclose(fr);

  ///// Now we have the entire map, cache certain cell sets for faster access /////////

  Proto_Server.game.map.floorCells_1 = malloc(Proto_Server.game.map.numFloor1 * sizeof(Cell*));
  Proto_Server.game.map.floorCells_2 = malloc(Proto_Server.game.map.numFloor2 * sizeof(Cell*));
  Proto_Server.game.map.homeCells_1 = malloc(Proto_Server.game.map.numHome1 * sizeof(Cell*));
  Proto_Server.game.map.homeCells_2 = malloc(Proto_Server.game.map.numHome2 * sizeof(Cell*));
  int floorCell1Index = 0;
  int floorCell2Index = 0;
  int homeCell1Index = 0;
  int homeCell2Index = 0;  

  for (i=0; i<Proto_Server.game.map.dimension.x; i++) {
    int j;
    for (j=0; j<Proto_Server.game.map.dimension.y; j++) {
  
      Cell *cell = &Proto_Server.game.map.mapBody[i][j];
      // Cache floor1 and floor2 cells in its own arrays
      if (cell->type == FLOOR_1) {
        Proto_Server.game.map.floorCells_1[floorCell1Index] = cell;
        floorCell1Index++;
      }
      else if (cell->type==FLOOR_2) {
        Proto_Server.game.map.floorCells_2[floorCell2Index] = cell;
        floorCell2Index++;        
      }
      else if (cell->type==HOME_1) {
        Proto_Server.game.map.homeCells_1[homeCell1Index] = cell;
        homeCell1Index++;        
      }
      else if (cell->type==HOME_2) {
        Proto_Server.game.map.homeCells_2[homeCell2Index] = cell;
        homeCell2Index++;        
      }      
      // Count the number of unfixed walls now we have all the cells in the map
      else if (cell->type==WALL_UNFIXED) {
          Cell *leftCell;
          Cell *rightCell;
          Cell *topCell;
          Cell *bottomCell;

          if (i>0)
            leftCell = &Proto_Server.game.map.mapBody[i-1][j];
          if (i<Proto_Server.game.map.dimension.x-1)
            rightCell = &Proto_Server.game.map.mapBody[i+1][j];
          if (j>0)
            topCell = &Proto_Server.game.map.mapBody[i][j-1];
          if (j<Proto_Server.game.map.dimension.y-1)
            bottomCell = &Proto_Server.game.map.mapBody[i][j+1];

          // Condition for exterior wall
          if (i==0 || j==0 || i==Proto_Server.game.map.dimension.x-1 || j==Proto_Server.game.map.dimension.y-1
            // Condition of wall near jail or home
            || leftCell->type==JAIL_1 || leftCell->type==JAIL_2 || leftCell->type==HOME_1 || leftCell->type==HOME_2
            || rightCell->type==JAIL_1 || rightCell->type==JAIL_2 || rightCell->type==HOME_1 || rightCell->type==HOME_2
            || topCell->type==JAIL_1 || topCell->type==JAIL_2 || topCell->type==HOME_1 || topCell->type==HOME_2
            || bottomCell->type==JAIL_1 || bottomCell->type==JAIL_2 || bottomCell->type==HOME_1 || bottomCell->type==HOME_2
            ) 
          {
            cell->type = WALL_FIXED;
            Proto_Server.game.map.numNonfixedWall--;
            Proto_Server.game.map.numFixedWall++;
          }
      }

    }
  }

  ///// End of caching certain cell sets for faster access /////////

  printMap(&Proto_Server.game.map);

  // // Allocate memory for ascii map representation
  char returnstr[(Proto_Server.game.map.dimension.x+1) * Proto_Server.game.map.dimension.y];
  // // Convert string
  convertToString(&Proto_Server.game.map, returnstr);
  // // Print
  fprintf(stderr, "%s\n", returnstr);

  return 1;

}

extern void convertToString(void *map, char *str) {
  Maze *maze = (Maze *)map;
  int width = maze->dimension.x;
  int height = maze->dimension.y;

  // char str[(width+1) * height];

  int i;
  int count = 0;
  for(i=0; i<height; i++){
    int j;
    for(j=0; j<width; j++){
      str[count] = getCellChar(maze->mapBody[i][j].type);
      count++;
    }
    str[count] = '\n';
    count++;
  }

  str[(count-1)] = '\0';
}




////////////// End of Newly added Capture the flag code ///////////////
