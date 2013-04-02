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
  case PROTO_MT_EVENT_LOBBY_UPDATE: 
    fprintf(stderr, "PROTO_MT_EVENT_LOBBY_UPDATE");
    break;    
  case PROTO_MT_EVENT_BASE_RESERVED_LAST: 
    fprintf(stderr, "PROTO_MT_EVENT_BASE_RESERVED_LAST");
    break;
  default:
    fprintf(stderr, "UNKNOWN=%d", type);
  }
}

extern char getCellChar(int type) {
    if (type==FLOOR_1)
      return ' ';
    if (type==FLOOR_2)
      return ' ';
    else if (type==WALL_FIXED)
      return '@';
    else if (type==WALL_UNFIXED)
      return '#';    
    else if (type==HOME_1)
      return 'h';
    else if (type==HOME_2)
      return 'H';
    else if (type==JAIL_1)
      return 'j';
    else if (type==JAIL_2)
      return 'J';
    else if (type==FLAG_1)
      return 'f';
    else if (type==FLAG_2)
      return 'F';

    return '?';
}

extern int cellTypeFromChar(char cell) {
    if (cell=='#')
    return WALL_UNFIXED;
  else if (cell==' ')
    return FLOOR_1;
  else if (cell=='h')
    return HOME_1;
  else if (cell=='H')
    return HOME_2;
  else if (cell=='j')
    return JAIL_1;
  else if (cell=='J')
    return JAIL_2;

  return INVALID;
}

extern char* cellTypeNameFromType(int type) {
    if (type==FLOOR_1)
      return "FLOOR_1";
    else if (type==FLOOR_2)
      return "FLOOR_2";    
    else if (type==WALL_FIXED)
      return "WALL_FIXED";
    else if (type==WALL_UNFIXED)
      return "WALL_UNFIXED";    
    else if (type==HOME_1)
      return "HOME_1";
    else if (type==HOME_2)
      return "HOME_2";
    else if (type==JAIL_1)
      return "JAIL_1";
    else if (type==JAIL_2)
      return "JAIL_2";
    else if (type==FLAG_1)
      return "FLAG_1";
    else if (type==FLAG_2)
      return "FLAG_2";

    return "INVALID";  
}

extern void printMap(void *map) {
  Maze *maze = (Maze *)map;
    fprintf(stderr, "PRINTING MAP\n");
  int i;
  for (i=0; i<maze->dimension.x; i++) {
    int j;
    for (j=0; j<maze->dimension.y; j++) {
  
      fprintf(stderr, "%c", getCellChar(maze->mapBody[i][j].type));
    }
    fprintf(stderr, "\n");
  }
   fprintf(stderr, "NumHome1: %d  NumHome2: %d  NumJail1: %d  NumJail2: %d  NumWall: %d  NumFloor: %d \n", maze->numHome1, maze->numHome2, maze->numJail1, maze->numJail2, maze->numFixedWall + maze->numNonfixedWall, maze->numFloor1 + maze->numFloor2);
  
}
 

extern Player* getPlayer(Game *game, int playerID) {
  int i;
  Player *player;

  for (i=0; i<MAX_NUM_PLAYERS; i++) {
    player = &(game->Team1_Players[i]);
    if (player->playerID==playerID)
      return player;
    player = &(game->Team2_Players[i]);
    if (player->playerID==playerID)
      return player;    
  }
  return NULL;
}


extern void
proto_dump_game_players(Game *g)
{
  int i;
  int array_len = sizeof(g->Team1_Players) / sizeof(g->Team1_Players[0]);
  fprintf(stderr, "Team 1 Players:\n");
  for(i=0; i<array_len; i++){
    fprintf(stderr, "PLAYER %d position: (%d,%d) \n", i+1, g->Team1_Players[i].cellposition.x, g->Team1_Players[i].cellposition.y);
    fprintf(stderr, "team: %d\n", g->Team1_Players[i].team);
    fprintf(stderr, "can move?: %d\n", g->Team1_Players[i].canMove);
  }

  fprintf(stderr, "Team 2 Players:\n");

  int j;
  for(j=0; j<array_len; j++){
    fprintf(stderr, "PLAYER %d position: (%d,%d) \n", j+1, g->Team2_Players[i].cellposition.x, g->Team2_Players[i].cellposition.y);
    fprintf(stderr, "team: %d\n", g->Team2_Players[i].team);
    fprintf(stderr, "can move?: %d\n", g->Team2_Players[i].canMove);
  }
}

extern void
proto_dump_gstate(GameStatus gs)
{
  if (gs==TEAM_1_WON) 
    fprintf(stderr, "TEAM_1_WON\n");
  else if (gs==TEAM_2_WON)
    fprintf(stderr, "TEAM_2_WON\n");
  else if (gs==NOT_STARTED)
    fprintf(stderr, "NOT_STARTED\n");  
  else if (gs==IN_PROGRESS)
    fprintf(stderr, "IN_PROGRESS\n");    
}

extern void
proto_dump_msghdr(Proto_Msg_Hdr *hdr)
{

  if (PROTO_PRINT_DUMPS==1) {

    fprintf(stderr, "ver=%d type=", hdr->version);
    proto_dump_mt(hdr->type);
    fprintf(stderr, " sver=%llx \n", hdr->sver.raw);
    fprintf(stderr, " game: \n");
    proto_dump_game_players(&(hdr->game));
    fprintf(stderr, " gstate: \n"); 
    proto_dump_gstate(hdr->game.status);
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

