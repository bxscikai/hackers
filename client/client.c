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

#define STRLEN    81
#define INPUTSIZE 50 

struct Globals {
  char host[STRLEN];
  PortType port;
} globals;


typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
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


void
prompt(int menu, int isX, char *result) 
{

  char *MenuString = "";
  if (isX == PLAYER_X){MenuString = "\nX> ";}
  else if (isX == PLAYER_O){MenuString = "\nO> ";}
  else {MenuString = "\n?> ";}
  
  int ret;
  int c=0;

  if (menu) printf("%s", MenuString);

  // OLD PROMPT
  // fflush(stdout);
  // c = getchar();

  // NEW PROMPT
  fgets(result, INPUTSIZE, stdin);  // Get string from user
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
      if (PROTO_PRINT_DUMPS==1) printf("hello: rc=%x\n", rc);
      rc = proto_client_hello(C->ph);
      if (rc > 0) game_process_reply(C);
    }
    break;
  case 'm':
    scanf("%c", &c);
    rc = proto_client_move(C->ph, c);
    break;
  case 'g':
    if (PROTO_PRINT_DUMPS==1) printf("goodbye: rc=%x\n", rc);
    rc = proto_client_goodbye(C->ph);
    rc = -1;
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
  if (PROTO_PRINT_DUMPS==1) printf("enter (h|m<c>|g): \n");
  scanf("%c", &c);
  rc=doRPCCmd(C,c);

  if (PROTO_PRINT_DUMPS==1) printf("doRPC: rc=0x%x\n", rc);

  return rc;
}

void where() {
  if (globals.port<=0)
    fprintf(stderr, "Not Connected\n");
  else
    fprintf(stderr, "Connection: %s:%d\n", globals.host, globals.port);
}

// See if we have a connect code
int
check_if_connect(char *mystring){

  char *first_part;
  char *sec_part;
  char *third_part;
  char * pch;

  // printf ("Splitting string \"%s\" into tokens:\n", mystring);
  pch = strtok (mystring," :");
  int i =0;
  while (pch != NULL)
  {
    if (i==0 && (strcmp(pch, "connect") != 0)) 
        return -1;
    else if (i==1) 
      strncpy(globals.host, pch, strlen(pch));    
    else if (i==2) 
      globals.port = atoi(pch);        
    i++;
    pch = strtok (NULL, " :");
  }

  // first_part = strtok(mystring, " ");
  return 1;
}

int 
docmd(Client *C, char *cmd)
{
  int rc = 1;

  // If this is a connect attempt
  int connectAttempt = check_if_connect(cmd);
  if (connectAttempt==1) {
  // Ok startup our connection to the server
      if (startConnection(C, globals.host, globals.port, update_event_handler)<0) {
        fprintf(stderr, "ERROR: startConnection failed\n");
        return -1;
      }
      else  {
        fprintf(stderr, "Successfully connected to <%s:%d>\n", globals.host, globals.port);      
        return proto_client_hello(C->ph);
      }
      return 1;
  }

  if (strcmp(cmd, "disconnect\n")==0) {
    doRPCCmd(C, 'g');
    rc=-1;
  }
  else if (strcmp(cmd, "where\n")==0) 
    where();
  else if (strcmp(cmd, "1\n")==0) 
    rc = proto_client_move(C->ph, '1');
  else if (strcmp(cmd, "2\n")==0) 
    rc = proto_client_move(C->ph, '2');  
  else if (strcmp(cmd, "3\n")==0) 
    rc = proto_client_move(C->ph, '3');
  else if (strcmp(cmd, "4\n")==0) 
    rc = proto_client_move(C->ph, '4');  
  else if (strcmp(cmd, "5\n")==0) 
    rc = proto_client_move(C->ph, '5');  
  else if (strcmp(cmd, "6\n")==0) 
    rc = proto_client_move(C->ph, '6');  
  else if (strcmp(cmd, "7\n")==0) 
    rc = proto_client_move(C->ph, '7');  
  else if (strcmp(cmd, "8\n")==0) 
    rc = proto_client_move(C->ph, '8');  
  else if (strcmp(cmd, "9\n")==0) 
    rc = proto_client_move(C->ph, '9');                 
  else if (strcmp(cmd, "d\n")==0) 
    proto_debug_on();  
  else if (strcmp(cmd, "D\n")==0) 
    proto_debug_off();
  else if (strcmp(cmd, "rh\n")==0) 
    rc = proto_client_hello(C->ph);
  else if (strcmp(cmd, "q\n")==0) {
    fprintf(stderr, "Game Over: You Quit\n");    
    doRPCCmd(C, 'g');
    rc=-1;
  }  
  else if (strcmp(cmd, "quit\n")==0) {
    doRPCCmd(C, 'g');
    fprintf(stderr, "Game Over: You Quit\n");
    rc=-1;
  }    
  else if (strcmp(cmd, "\n")==0) {
    rc = proto_client_update(C->ph);
    rc=1;
  }
  else
    fprintf(stderr, "Unknown command\n");

  return rc;
}

void *
shell(void *arg)
{
  Client *C = arg;
  char input[INPUTSIZE];
  int rc;
  int menu=1;
  //the following is done in order to change the prompt for the user to X or O
  int promptType;

  
  while (1) {

    // Clear input each time
    bzero(&input, INPUTSIZE);

    promptType = proto_client_isX(C->ph);
    prompt(menu, promptType, input);

    if (strlen(input)>0) {
      rc=docmd(C, input);
      menu = 1;
    }
    if (rc<0) {
      killConnection(C->ph);
      break;
    }
    if (rc==1) 
    {
       menu=1;       
       Proto_Game_State *gs = proto_client_game_state(C->ph);
    } 
    else menu=1;
  }

  if (PROTO_PRINT_DUMPS==1) fprintf(stderr, "terminating\n");
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

  fprintf(stderr, "Type 'connect <host:port>' to connect to a game.\n");

  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    


  shell(&c);

  return 0;
}

