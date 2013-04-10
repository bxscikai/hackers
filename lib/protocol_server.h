#ifndef __DAGAME_PROTOCOL_SERVER_H__
#define __DAGAME_PROTOCOL_SERVER_H__
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

#include "net.h"
#include "protocol.h"
#include "protocol_session.h"


extern int proto_server_init(void);

extern int proto_server_set_session_lost_handler(Proto_MT_Handler h);
extern int proto_server_set_req_handler(Proto_Msg_Types mt, Proto_MT_Handler h);

extern PortType proto_server_rpcport(void);
extern PortType proto_server_listenport(void);
extern Proto_Session *proto_server_event_session(void);
extern int    proto_server_start_rpc_loop(void);
extern void proto_server_post_event(Proto_Msg_Types mt);

// Game logic
extern void setPostMessage(Proto_Session *event, Proto_Msg_Types mt);
extern void printGameState();
static void reinitialize_State();
// Handlers
static int proto_server_mt_rpc_goodbye_handler(Proto_Session *s);
static int proto_server_mt_rpc_hello_handler(Proto_Session *s);
static int proto_server_mt_rpc_move_handler(Proto_Session *s);
static int proto_server_mt_rpc_pickup_handler(Proto_Session *s);
static int proto_server_mt_rpc_update_handler(Proto_Session *s);
static int proto_server_mt_rpc_querymap_handler(Proto_Session *s);
static int proto_server_mt_rpc_lobby_update_handler(Proto_Session *s);
static int proto_server_mt_rpc_start_game(Proto_Session *s);

// Capture the flag logic
extern int proto_server_parse_map(char *filename);
// extern char * convertToString(void *map);
extern void convertToString(void *map, char *str);
static int getNumberOfPlayersForTeam(int team);
static void insertPlayerForTeam(int team, Player *player);
static void spawnObject(ObjectType obj);
extern GameStatus checkOverGame(Game *game);

#endif
