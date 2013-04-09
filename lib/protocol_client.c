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

#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_client.h"
#include "../client/client.h"

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

extern GameStatus *
proto_client_game_status(Proto_Client_Handle ch)
{
  Proto_Client *c = ch;
  return &(c->game.status);
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

      if (mt > PROTO_MT_REQ_BASE_RESERVED_FIRST && 
    mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {

    // We are getting the handler corresponding to our message type from our protocol_client 
      mt = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
      hdlr = c->base_event_handlers[mt];

        if (hdlr(s)<0) goto leave;
         
        // Sync server game state with local game state        
        // c->game.status = s->rhdr.game.status;
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
      else if (mt==PROTO_MT_REP_BASE_PICKUP)
	proto_client_set_event_handler(c, mt, proto_server_mt_rpc_rep_pickup_handler); 
      else if (mt==PROTO_MT_REP_BASE_MAPQUERY)
        proto_client_set_event_handler(c, mt, proto_server_mt_rpc_rep_querymap_handler);      
      else if (mt==PROTO_MT_EVENT_LOBBY_UPDATE)
        proto_client_set_event_handler(c, mt, proto_server_mt_rpc_lobby_update_handler);      
      else if (mt==PROTO_MT_REP_BASE_START_GAME)
        proto_client_set_event_handler(c, mt, proto_server_mt_rep_start_game);      
      else if (mt==PROTO_MT_EVENT_GAME_UPDATE)
        proto_client_set_event_handler(c, mt, proto_server_mt_game_update_handler);              
      else
        proto_client_set_event_handler(c, mt, proto_client_event_null_handler);
  }

    // Print out handlers
    // for (mt=PROTO_MT_REQ_BASE_RESERVED_FIRST+1;
    //    mt<PROTO_MT_EVENT_BASE_RESERVED_LAST; mt++) {
    //   Proto_MT_Handler handler =  c->base_event_handlers[mt];
    // fprintf(stderr, "Handler at index: %d  is: %p\n", mt, handler);
    // }
  c->game.status = NOT_STARTED;

  *ch = c;


  return 1;
}

int
proto_client_connect(Proto_Client_Handle ch, char *host, PortType port)
{
  Proto_Client *c = (Proto_Client *)ch;
  c->rpc_session.client = c;
  c->event_session.client = c;

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

    // We got map data back, store it in our proto_client
    if (s->rhdr.type==PROTO_MT_REP_BASE_MAPQUERY) {
        c->game.map = s->rhdr.game.map;
    }

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

    return 1;

  return do_generic_dummy_rpc(ch,PROTO_MT_EVENT_REQ_UPDATE);  
}

extern int 
proto_client_move(Proto_Client_Handle ch, char data)
{
  Proto_Client *client = ch;
  // client->rpc_session.shdr.pstate.playerMove.raw = (int) (data - '0');

  if (client->game.status != IN_PROGRESS) {
      fprintf(stderr, "The game hasn't started yet!\n");
      return 1;
  }

  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_MOVE);  
}

extern int
proto_client_pickup(Proto_Client_Handle ch)
{
  Proto_Client *client = ch;
  

    if (client->game.status != IN_PROGRESS) {
      fprintf(stderr, "The game hasn't started yet!\n");
      return 1;
  }
  //ERIC
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_PICKUP);
}

extern int 
proto_client_goodbye(Proto_Client_Handle ch)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_GOODBYE);  
}

extern int 
proto_client_querymap(Proto_Client_Handle ch)
{
  Proto_Client *client = ch;

  // If we already cached the map, do not query the server again
  if (client->game.map.dimension.x>0 && client->game.map.dimension.y>0)
    return 1;

  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_MAPQUERY);  
}

extern int 
proto_client_startgame(Proto_Client_Handle ch)
{
  return do_generic_dummy_rpc(ch,PROTO_MT_REQ_BASE_START_GAME);  
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

// Send ACK to client
static int sendACK(Proto_Session *s, Proto_Msg_Types mt) {
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr));
  marshall_mtonly(s, mt);
  proto_session_send_msg(s, 0);  
}

static int 
proto_server_mt_rpc_rep_hello_handler(Proto_Session *s)
{
  Proto_Client *c = s->client;
  c->playerID = s->rhdr.version;

  if (s->rhdr.returnCode==RPC_HELLO_ALREADYJOINED) {
    fprintf(stderr, "You have already joined the game!\n");
    return 1;
  }

  fprintf(stderr, "Connected to server with fd=%d\n", c->playerID);

  // Make server send lobby updates
  sendACK(s, PROTO_MT_EVENT_LOBBY_UPDATE);

  return 1;
}

static int 
proto_server_mt_rpc_lobby_update_handler(Proto_Session *s)
{

  fprintf(stderr, "Received lobby update\n");

  Proto_Client *c = s->client;
  Player clientPlayer;
  int numOfPlayersTeam1 = 0;
  int numOfPlayersTeam2 = 0;
  int i;

  // Get players on each team
  for (i=0; i<MAX_NUM_PLAYERS; i++) {
    Player current = s->rhdr.game.Team1_Players[i];
    if (current.playerID>0)
      numOfPlayersTeam1++;
    if (c->playerID == current.playerID)
      clientPlayer = current;
  }
  for (i=0; i<MAX_NUM_PLAYERS; i++) {
    Player current = s->rhdr.game.Team2_Players[i];
    if (current.playerID>0)
      numOfPlayersTeam2++;
    if (c->playerID == current.playerID)
      clientPlayer = current;    
  }

  fprintf(stderr, "\n---Lobby Update---\n");
  fprintf(stderr, "Number of players on Team 1: %d\n", numOfPlayersTeam1);
  fprintf(stderr, "Number of players on Team 2: %d\n", numOfPlayersTeam2);
  if (clientPlayer.isHost==1)
    fprintf(stderr, "You are the host, start the game with 'Start' command when ready\n");
  fprintf(stderr, "---End of Lobby Update---\n");

  // Let the server know it received the update
  sendACK(s, PROTO_MT_EVENT_LOBBY_UPDATE);

  return 1;
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
proto_server_mt_game_update_handler(Proto_Session *s)
{
  // Update game states
    Proto_Client *c = s->client;

  // Copy player states to local state
  int i;
  for (i=0; i<MAX_NUM_PLAYERS; i++)
    c->game.Team1_Players[i] = s->rhdr.game.Team1_Players[i];
  for (i=0; i<MAX_NUM_PLAYERS; i++)
    c->game.Team2_Players[i] = s->rhdr.game.Team2_Players[i];  
  for (i=0; i<NUMOFOBJECTS;i++)
    c->game.map.objects[i] = s->rhdr.game.map.objects[i];
  c->game.status = s->rhdr.game.status;

  Player *player = getPlayer(&c->game, c->playerID);

  // Right now simply prints out all new player locations
  printUpdate(&c->game);

  Update_UI(player, &c->game);
  //UI CODE
  // Player *player = getPlayer(&c->game, c->playerID);
  // ui_paintmap(ui, &c->game, player);
  // SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);

  // TIFFANY, THIS IS WHERE U UPDATE GAME UI FOR USER ///////////
  // YOU CAN ACCESS THE GAME STRUCT IN c->game
  // PLAYERS: c->game.Team1_Players, c->game.Team2_Players
  // ITEMS:   c->game.map.objects
  // Command instructions
  // rh = hello, register yourself
  // start = start game, must be host
  // w,a,s,d = move up,left,down,right respectively
  //////////////////////////////////////////////////////////////

  return 1;
}


static int 
proto_server_mt_rep_start_game(Proto_Session *s)
{

  Proto_Client *c = s->client;

    // Don't initalize if the game started already
  if (c->game.status==IN_PROGRESS)
    return 1;

  // Copy player states to local state
  int i;
  for (i=0; i<MAX_NUM_PLAYERS; i++)
    c->game.Team1_Players[i] = s->rhdr.game.Team1_Players[i];
  for (i=0; i<MAX_NUM_PLAYERS; i++)
    c->game.Team2_Players[i] = s->rhdr.game.Team2_Players[i];  
  for (i=0; i<NUMOFOBJECTS;i++)
    c->game.map.objects[i] = s->rhdr.game.map.objects[i];
  c->game.status = s->rhdr.game.status;


  // Handle error cases
  if (s->rhdr.returnCode==RPC_STARTGAME_NOT_HOST)
    fprintf(stderr, "Failed to start game, you are not the host\n");
  else if (s->rhdr.returnCode==RPC_STARTGAME_UNEVEN_PLAYERS)
    fprintf(stderr, "Failed to start game, uneven number of players on each side\n");

  // Handle game start
  else 
  {
    Player *player = getPlayer(&c->game, c->playerID);

    if (player==NULL){
      fprintf(stderr, "player not found\n");
    }else{ 
      fprintf(stderr, "Player ID: %d  location is (%d,%d)\n", player->playerID, player->cellposition.x, player->cellposition.y);
      fprintf(stderr, "The Game has started, hit enter to begin.\n");
      fprintf(stderr, "May the odds be ever in your favor.\n");
    }
  }

  return 1;
}

static int 
proto_server_mt_rpc_rep_move_handler(Proto_Session *s)
{
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr));
  
  // RESPOND TO PLAYER MOVE
  if (s->rhdr.returnCode==SUCCESS)
    fprintf(stderr, "Move success!\n");
  else if (s->rhdr.returnCode==RPC_MOVE_MOVING_INTO_WALL)
    fprintf(stderr, "Move failed, cannot move into wall\n");
  else if (s->rhdr.returnCode==RPC_MOVE_MOVING_INTO_PLAYER)
    fprintf(stderr, "Move failed, cannot move into another player\n");  

  return 1;
}

static int
proto_server_mt_rpc_rep_pickup_handler(Proto_Session *s)
{
  Proto_Msg_Hdr h;
  bzero(&h, sizeof(Proto_Msg_Hdr));

  if (s->rhdr.returnCode==RPC_PICKUP_SUCCESS)
    fprintf(stderr, "Pickup success!\n");
  else if (s->rhdr.returnCode==RPC_DROP)
    fprintf(stderr, "Item Dropped!\n");
  else if (s->rhdr.returnCode==RPC_PICKUP_NOTHING)
    fprintf(stderr, "Pickup Fail: No item to pickup\n");
  else if (s->rhdr.returnCode==RPC_PICKUP_FLAGONSIDE)
    fprintf(stderr, "Pickup Fail: Cannot pick up your own flag when on your own side\n");
  return 1;
}

static int 
proto_server_mt_rpc_rep_querymap_handler(Proto_Session *s)
{
  // fprintf(stderr, "NumHome1: %d  NumHome2: %d  NumJail1: %d  NumJail2: %d  NumWall: %d  NumFloor: %d \n", s->rhdr.game.map.numHome1, s->rhdr.game.map.numHome2, s->rhdr.game.map.numJail1, s->rhdr.game.map.numJail2, s->rhdr.game.map.numWall, s->rhdr.game.map.numFloor);
  char string[(s->rhdr.game.map.dimension.x+1)*s->rhdr.game.map.dimension.y];
  proto_session_hdr_unmarshall_mapBody(s, string);
  parseMapFromString(string, &s->rhdr.game.map);
  printMap(&s->rhdr.game.map);
  // fprintf(stderr, "The map: %s\n", string);

  return 1; // rc just needs to be >1
}

/////////// End of Custom Event Handlers ///////////////


///////////////////////// HELPER METHODS //////////////////////////////



static void parseMapFromString(char *mapString, Maze *map) {

  int row;
  int column;
  int i;
  int count = 0;
  // Allocate memory for map
  map->mapBody = malloc(map->dimension.y * sizeof(Cell *));
    
  for(i = 0; i < map->dimension.y; i++) {
    map->mapBody[i] = malloc(map->dimension.x * sizeof(Cell));
  }

  // fprintf(stderr, "Dim %dx%d\n", map->dimension.x, map->dimension.y);
  // Parse map and store it
  for (row=0; row<map->dimension.x;row++) {
    for (column=0; column<map->dimension.y; column++) {

      char c = mapString[(row * (map->dimension.x+1)) + column];
      // fprintf(stderr, "%c", c);
      Cell cell;
      cell.type = cellTypeFromChar(c);
      cell.occupied = 0;
      map->mapBody[row][column] = cell;
      count++;
    }
    // fprintf(stderr, "\n");
  }
}

/////////// END OF HELPER METHODS ///////////////
