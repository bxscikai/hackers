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
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"

#define STRLEN 81

struct Globals {
  char host[STRLEN];
  PortType port;
} globals;


typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
  int isX;
} Client;

static int
clientInit(Client *C)
{
  bzero(C, sizeof(Client));

  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  }
  return 1;
}


static int
update_event_handler(Proto_Session *s)
{
  Client *C = proto_session_get_data(s);

  fprintf(stderr, "%s: called", __func__);
  return 1;
}


int 
startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h)
{
  if (globals.host[0]!=0 && globals.port!=0) {
    if (proto_client_connect(C->ph, host, port)!=0) {
      fprintf(stderr, "failed to connect\n");
      return -1;
    }
    proto_session_set_data(proto_client_event_session(C->ph), C);
#if 0
    if (h != NULL) {
      proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, 
             h);
    }
#endif
    return 1;
  }
  return 0;
}


int
prompt(int menu, int isX) 
{
  char *MenuString = "";
  if (isX == 1){MenuString = "\nX> ";}
  else {MenuString = "\nO> ";}
  
  int ret;
  int c=0;

  if (menu) printf("%s", MenuString);
  fflush(stdout);
  c = getchar();
  return c;
}


// FIXME:  this is ugly maybe the speration of the proto_client code and
//         the game code is dumb
int
game_process_reply(Client *C)
{
  Proto_Session *s;

  s = proto_client_rpc_session(C->ph);

  fprintf(stderr, "%s: do something %p\n", __func__, s);

  return 1;
}


int 
doRPCCmd(Client *C, char c) 
{
  int rc=-1;

  switch (c) {
  case 'h':  
    {
      printf("hello: rc=%x\n", rc);
      rc = proto_client_hello(C->ph);
      if (rc > 0) game_process_reply(C);
    }
    break;
  case 'm':
    scanf("%c", &c);
    rc = proto_client_move(C->ph, c);
    break;
  case 'g':
    printf("goodbye: rc=%x\n", rc);
    rc = proto_client_goodbye(C->ph);
    break;
  default:
    printf("%s: unknown command %c\n", __func__, c);
  }
  // NULL MT OVERRIDE ;-)
  if (PROTO_PRINT_DUMPS==1) printf("%s: rc=0x%x\n", __func__, rc);
  if (rc == 0xdeadbeef) rc=1;
  return rc;
}

int
doRPC(Client *C)
{
  int rc;
  char c;

  // Enter command, h=hello, m<c> = move c steps, g = goodbye
  printf("enter (h|m<c>|g): \n");
  scanf("%c", &c);
  rc=doRPCCmd(C,c);

  if (PROTO_PRINT_DUMPS==1) printf("doRPC: rc=0x%x\n", rc);

  return rc;
}


int 
docmd(Client *C, char cmd)
{
  int rc = 1;

  switch (cmd) {
  case 'd':
    proto_debug_on();
    break;
  case 'D':
    proto_debug_off();
    break;
  case 'r':
    rc = doRPC(C);
    break;
  case 'q':
    rc=-1;
    break;
  case '\n':
    rc=1;
    break;
  default:
    printf("Unkown Command\n");
  }
  return rc;
}

void *
shell(void *arg)
{
  Client *C = arg;
  char c;
  int rc;
  int menu=1;
  //the following is done in order to change the prompt for the user to X or O
  int promptType = C->isX;
  
  while (1) {
    if ((c=prompt(menu, promptType))!=0) rc=docmd(C, c);
    if (rc<0) {
      killConnection(C->ph);
      break;
    }
    if (rc==1) 
    {
       menu=1;
       
       Proto_Game_State *gs = proto_client_game_state(C->ph);
       printBoard(gs);
       //This is where the screen gets replaced
       //printf("\n1|2|3\n- - -\n4|5|6\n- - -\n7|8|9\n");
    

    
    } 
    else menu=0;
  }

  fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}

void 
usage(char *pgm)
{
  fprintf(stderr, "USAGE: %s <port|<<host port> [shell] [gui]>>\n"
           "  port     : rpc port of a game server if this is only argument\n"
           "             specified then host will default to localhost and\n"
     "             only the graphical user interface will be started\n"
           "  host port: if both host and port are specifed then the game\n"
     "examples:\n" 
           " %s 12345 : starts client connecting to localhost:12345\n"
    " %s localhost 12345 : starts client connecting to locaalhost:12345\n",
     pgm, pgm, pgm);
 
}

void
initGlobals(int argc, char **argv)
{
  bzero(&globals, sizeof(globals));

  if (argc==1) {
    usage(argv[0]);
    exit(-1);
  }

  if (argc==2) {
    strncpy(globals.host, "localhost", STRLEN);
    globals.port = atoi(argv[1]);
  }

  if (argc>=3) {
    strncpy(globals.host, argv[1], STRLEN);
    globals.port = atoi(argv[2]);
  }

}

int 
main(int argc, char **argv)
{
  Client c;

  initGlobals(argc, argv);

  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

  // ok startup our connection to the server
  if (startConnection(&c, globals.host, globals.port, update_event_handler)<0) {
    fprintf(stderr, "ERROR: startConnection failed\n");
    return -1;
  }
  
  printf("\n1|2|3\n- - -\n4|5|6\n- - -\n7|8|9\n");
  
  shell(&c);

  return 0;
}

void
printBoard(Proto_Game_State *myGS)
{
  int pos1, pos2, pos3, pos4, pos5, pos6, pos7, pos8, pos9;
  char cpos1, cpos2, cpos3, cpos4, cpos5, cpos6, cpos7, cpos8, cpos9;

  pos1 = ntohl(myGS->pos1.raw);
  pos2 = ntohl(myGS->pos2.raw);
  pos3 = ntohl(myGS->pos3.raw);
  pos4 = ntohl(myGS->pos4.raw);
  pos5 = ntohl(myGS->pos5.raw);
  pos6 = ntohl(myGS->pos6.raw);
  pos7 = ntohl(myGS->pos7.raw);
  pos8 = ntohl(myGS->pos8.raw);
  pos9 = ntohl(myGS->pos9.raw);
  
  if (pos1 == 0){cpos1 = '1';}
  else if (pos1 == 1){cpos1 = 'X';} else {cpos1 = 'O';}
  if (pos2 == 0){cpos2 = '2';}
  else if (pos2 == 1){cpos2 = 'X';} else {cpos2 = 'O';}
  if (pos3 == 0){cpos3 = '3';}
  else if (pos3 == 1){cpos3 = 'X';} else {cpos3 = 'O';}
  if (pos4 == 0){cpos4 = '4';}
  else if (pos4 == 1){cpos4 = 'X';} else {cpos4 = 'O';}
  if (pos5 == 0){cpos5 = '5';}
  else if (pos5 == 1){cpos5 = 'X';} else {cpos5 = 'O';}
  if (pos6 == 0){cpos6 = '6';}
  else if (pos6 == 1){cpos6 = 'X';} else {cpos6 = 'O';}
  if (pos7 == 0){cpos7 = '7';}
  else if (pos7 == 1){cpos7 = 'X';} else {cpos7 = 'O';}
  if (pos8 == 0){cpos8 = '8';}
  else if (pos8 == 1){cpos8 = 'X';} else {cpos8 = 'O';}
  if (pos9 == 0){cpos9 = '9';}
  else if (pos9 == 1){cpos9 = 'X';} else {cpos9 = 'O';}
  printf("\n%c|%c|%c\n- - -\n%c|%c|%c\n- - -\n%c|%c|%c\n", cpos1, cpos2, cpos3, cpos4, cpos5, cpos6, cpos7, cpos8, cpos9);
}
