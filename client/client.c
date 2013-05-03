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
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "../lib/types.h"
#include "client.h"
#include "../lib/protocol_utils.h"
#include "tty.h"
#include "uistandalone.h"

UI *ui;

#define STRLEN    81
#define INPUTSIZE 50 
#define FASTINPUTMODE 1

// For documenting speed of various game functions
struct timeval rpc_start;
struct timeval rpc_pickup_start;

struct Globals {
  char host[STRLEN];
  PortType port;
} globals;

extern void Update_UI(Player *myPlayer, void *game)
{
  struct timeval ui_start;
  struct timeval ui_end;
  gettimeofday(&ui_start, NULL);

  ui_repaint(ui, game, myPlayer);

  gettimeofday(&ui_end, NULL);

  double difference = (ui_end.tv_sec*1000 + ui_end.tv_usec*0.001) - (ui_start.tv_sec*1000 + ui_start.tv_usec*0.001);
  fprintf(stderr, "Time elapsed for UI: %.f milliseconds\n", difference);
}

static int
clientInit(Client *C)
{
  bzero(C, sizeof(Client));
  // Proto_Client *client = C->ph;


  // initialize the client protocol subsystem
  if (proto_client_init(&(C->ph))<0) {
    fprintf(stderr, "client: main: ERROR initializing proto system\n");
    return -1;
  }

  Proto_Client *client = C->ph;
  client->client = C;

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
prompt(int menu, char *result) 
{

  char *MenuString = "?>";
  
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
    gettimeofday(&rpc_start, NULL);
    if (PROTO_PRINT_DUMPS==1) printf("move: rc=%x\n", rc);
    rc = proto_client_move(C->ph, c);
    break;
  case 'f':
    gettimeofday(&rpc_pickup_start, NULL);  
    if (PROTO_PRINT_DUMPS==1) printf("pickup: rc=%x\n", rc);
    rc = proto_client_pickup(C->ph);
    break;
  case 'g':
    if (PROTO_PRINT_DUMPS==1) printf("goodbye: rc=%x\n", rc);
    rc = proto_client_goodbye(C->ph);
    // We are done, exit the client
    exit(1);
    rc = -1;

    break;
  case 's':
    if (PROTO_PRINT_DUMPS==1) printf("start: rc=%x\n", rc);
    rc = proto_client_startgame(C->ph);
    break;    
  case 'q':
    if (PROTO_PRINT_DUMPS==1) printf("query map: rc=%x\n", rc);
    rc = proto_client_querymap(C->ph);
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

int containsString(char *source, char *substr) {

  char str1[50];
  strcpy(str1, source);
  char * pch = strtok (str1," ");
  int i =0;
  while (pch != NULL)
  {
    if (strcmp(pch, substr)==0)  
      return 1;    

    pch = strtok (NULL, " ");

  }
  return -1;
}

void cinfo(char *string, Client *C) {

// cinfo <x,y>
  Proto_Client *client = (Proto_Client *) C->ph;
  char *pch;
  pch = strtok (string," ");  
  int segmentNumber=0;
  int innersegmentNumber=0;
  int row = 0;
  int column=0;

  // Parse input to get row and column for cinfo command
  while (pch != NULL)
  {
    if (segmentNumber==1)  {      
      pch = strtok(pch, ",");
      while (pch != NULL) {
        if (innersegmentNumber==0)
          row = atoi(pch);
        else if (innersegmentNumber==1) {
          column = atoi(pch);
          break;
        }
        pch = strtok (NULL, ",");
        innersegmentNumber++;
      }   
    }
    segmentNumber++;        
    pch = strtok (NULL, " ");

  }

  Cell mycell = client->game.map.mapBody[row][column];
  fprintf(stderr, "%s\n", cellTypeNameFromType(mycell.type));

}

int 
docmd(Client *C, char *cmd)
{
  int rc = 1;
  Proto_Client *client = (Proto_Client *) C->ph;

  // If this is a connect attempt
  char input[50];
  strcpy(input, cmd);
  int connectAttempt = check_if_connect(input);

  fprintf(stderr, "Connect: %d\n", connectAttempt);

  if (connectAttempt==1) {
  // Ok startup our connection to the server
      if (startConnection(C, globals.host, globals.port, update_event_handler)<0) {
        fprintf(stderr, "ERROR: startConnection failed\n");
        return -1;
      }
      else  {
        fprintf(stderr, "Successfully connected to <%s:%d>\n", globals.host, globals.port);  
        fprintf(stderr, "Before RH\n");    
        proto_client_hello(C->ph);
        fprintf(stderr, "Done with RH\n");

        doRPCCmd(C, 'q'); //query for the map
        fprintf(stderr, "Got Map\n");

        return 1;
      }
      return 1;
  }
  strcpy(input,cmd);
  if (strcmp(cmd, "disconnect\n")==0) {
    doRPCCmd(C, 'g');
    rc=-1;
  }
  else if (strcmp(cmd, "where\n")==0) 
    where();
  else if (strcmp(cmd, "numhome 1\n")==0) {
    doRPCCmd(C, 'q');  // query map
    if (client->game.map.numHome1!=0)
      fprintf(stderr, "%d\n", client->game.map.numHome1);
  }
  else if (strcmp(cmd, "numhome 2\n")==0) {
    doRPCCmd(C, 'q');  // query map
    if (client->game.map.numHome2!=0)
      fprintf(stderr, "%d\n", client->game.map.numHome2);    
  }
  else if (strcmp(cmd, "numjail 1\n")==0) {
    doRPCCmd(C, 'q');  // query map
    if (client->game.map.numJail1!=0)
      fprintf(stderr, "%d\n", client->game.map.numJail1);    
  }
  else if (strcmp(cmd, "numjail 2\n")==0) {
    doRPCCmd(C, 'q');  // query map
    if (client->game.map.numHome2!=0)
      fprintf(stderr, "%d\n", client->game.map.numJail2);    
  }
  else if (strcmp(cmd, "numwall\n")==0) {
    doRPCCmd(C, 'q');  // query map
    if (client->game.map.numFixedWall!=0 && client->game.map.numNonfixedWall!=0)
      fprintf(stderr, "%d\n", client->game.map.numFixedWall + client->game.map.numNonfixedWall);    
  }
  else if (strcmp(cmd, "numfloor\n")==0) {
    doRPCCmd(C, 'q');  // query map       
    if (client->game.map.numFloor1!=0 && client->game.map.numFloor2!=0)
      fprintf(stderr, "%d\n", client->game.map.numFloor1+client->game.map.numFloor2);    
  }       
  else if (strcmp(cmd, "dim\n")==0) {
    doRPCCmd(C, 'q');  // query map  
    if (client->game.map.dimension.x!=0 && client->game.map.dimension.y!=0)
      fprintf(stderr, "%dx%d\n", client->game.map.dimension.x, client->game.map.dimension.y);    
  }                  
  else if (strcmp(cmd, "dump\n")==0) {
    doRPCCmd(C, 'q');  // query map   
    printMap(&client->game.map);
    if (DISPLAYUI==1)
      ui_update(ui);
  }                 
  else if (containsString(input, "cinfo")>0) {
    doRPCCmd(C, 'q');  // query map     
    cinfo(cmd, C);
  }
  else if (strcmp(cmd, "start\n")==0) {
      doRPCCmd(C, 's');  // query map 
  }
  else if (strcmp(cmd, "test\n")==0)  {
      if (STRESS_TEST == 1)
      {
          pid_t myChild;
          myChild = fork();
	  if (myChild == 0)
          {
	      Wander(C, 0);
          }
          else{} 
      }
  }
  // MOVEMENT
  else if (strcmp(cmd, "w\n")==0) {
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = UP;
      doRPCCmd(C, 'm');  // query map   
  }
  else if (strcmp(cmd, "a\n")==0) {
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = LEFT;
      doRPCCmd(C, 'm');  // query map   
  }
  else if (strcmp(cmd, "d\n")==0) {
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = RIGHT;
      doRPCCmd(C, 'm');  // query map   
  }
  else if (strcmp(cmd, "s\n")==0) {
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = DOWN;
      doRPCCmd(C, 'm');  // query map   
  }
  // END OF MOVEMENT  
  
  //PICKUP
  else if (strcmp(cmd, "f\n")==0){
      Proto_Client *client = C->ph;
      doRPCCmd(C, 'f');
  }
  
  //END PICKUP
  else if (strcmp(cmd, "O\n")==0)     
    proto_debug_on();  
  else if (strcmp(cmd, "o\n")==0) 
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
  else {
    fprintf(stderr, "Unknown command\n");
  }
    
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
  
  while (1) {

    // Clear input each time
    bzero(&input, INPUTSIZE);
    
    prompt(menu, input);

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
       // Proto_Game_State *gs = proto_client_game_state(C->ph);
    }
    if (((Proto_Client *) C->ph)->game.status == IN_PROGRESS && DISPLAYUI == 1)
    {
      // launchUI(C);
      return NULL;
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

  if (FASTINPUTMODE==1) {
    if (argc==1) {
      usage(argv[0]);
      exit(-1);
    }
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
  
  if (!FASTINPUTMODE)
      fprintf(stderr, "Type 'connect <host:port>' to connect to a game.\n");

  if (clientInit(&c) < 0) {
    fprintf(stderr, "ERROR: clientInit failed\n");
    return -1;
  }    

  initGlobals(argc, argv);

  if (FASTINPUTMODE) {
    startConnection(&c, globals.host, globals.port, update_event_handler);
    doRPCCmd(&c, 'q'); //query for the map
    if (STRESS_TEST==1)
      proto_client_hello(c.ph);
  }    

  shell(&c);
  // Cannot put shell on a separate thread because fget() function doesn't work for some reason
  // pthread_t tid;
  // pthread_create(&tid, NULL, shell, &c);
  Proto_Client *proto_client = c.ph;
  Player *me = getPlayer(&proto_client->game, proto_client->playerID);

  if (DISPLAYUI==1) {

    // If I am not the host and we are stress testing, I should be wandering
    if (me->isHost==0 && STRESS_TEST==1) {
         // Wander(&c, 0);
        docmd(&c, "test\n");

    }
    // The host will be the only ones that has UI showing, other players just wonder
    else 
    {
      //window will be consistently 20x20
      // pthread_t tid;
      // pthread_create(&tid, NULL, shell, NULL);

      // Init for UI stuff
      tty_init(STDIN_FILENO);

      ui_init(&(ui));

      // WITH OSX ITS IS EASIEST TO KEEP UI ON MAIN THREAD
      // SO JUMP THROW HOOPS :-(
      Proto_Client *client = (Proto_Client *) c.ph;
      Player *me = getPlayer(&client->game, client->playerID);

      doRPCCmd(&c, 'q'); //query for the map
      ui_main_loop(ui, (32 * WINDOW_SIZE), (32 * WINDOW_SIZE), &client->game, me, &c);
    }

  }


  // launchUI(&c);
    
  return 0;
}

void
launchUI(Client *c) {

  // if (DISPLAYUI==1) {

  //   //window will be consistently 20x20
  //   pthread_t tid;
  //   pthread_create(&tid, NULL, shell, NULL);

  //   // Init for UI stuff
  //   tty_init(STDIN_FILENO);

  //   ui_init(&(ui));

  //   // WITH OSX ITS IS EASIEST TO KEEP UI ON MAIN THREAD
  //   // SO JUMP THROW HOOPS :-(
  //   Proto_Client *client = (Proto_Client *) c->ph;
  //   Player *me = getPlayer(&client->game, client->playerID);

  //   doRPCCmd(c, 'q'); //query for the map

  //   ui_main_loop(ui, (32 * client->game.map.dimension.x * 0.1), (32 * client->game.map.dimension.y * 0.1), &client->game, me, &c);

  // }

}


extern sval
ui_keypress(UI *ui, SDL_KeyboardEvent *e, Client *C)
{
  SDLKey sym = e->keysym.sym;
  SDLMod mod = e->keysym.mod;

  if (e->type == SDL_KEYDOWN) {
    if (sym == SDLK_LEFT && mod == KMOD_NONE) {
      fprintf(stderr, "%s: move left\n", __func__);
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = LEFT;

      doRPCCmd(C, 'm');
      return 2;
    }
    if (sym == SDLK_RIGHT && mod == KMOD_NONE) {
      fprintf(stderr, "%s: move right\n", __func__);
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = RIGHT;

      doRPCCmd(C, 'm');
      return 2;
    }
    if (sym == SDLK_UP && mod == KMOD_NONE)  {  
      fprintf(stderr, "%s: move up\n", __func__);
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = UP;
    
      doRPCCmd(C, 'm');
      return 2;
    }
    if (sym == SDLK_t && mod == KMOD_NONE)  {  
      Wander(C, 0);

      return 2;
    }
    if (sym == SDLK_DOWN && mod == KMOD_NONE)  {
      fprintf(stderr, "%s: move down\n", __func__);
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = DOWN;

      doRPCCmd(C, 'm');
      return 2;
    }
    if (sym == SDLK_f && mod == KMOD_NONE)  {   
      fprintf(stderr, "%s: pickup \n", __func__);
      doRPCCmd(C, 'f');
      return 2;
    }
    if (sym == SDLK_q && mod == KMOD_NONE)  {   
      doRPCCmd(C, 'g');
      return -1;
    }    
    if (sym == SDLK_z && mod == KMOD_NONE){
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = UP;
      Player *me = getPlayer(&client->game, client->playerID);

      return ui_zoom(ui, 1, &client->game, me);
    }
    if (sym == SDLK_z && mod & KMOD_SHIFT ){
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = UP;
      Player *me = getPlayer(&client->game, client->playerID);
      
      return ui_zoom(ui,-1, &client->game, me);
    }
    if (sym == SDLK_LEFT && mod & KMOD_SHIFT){
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = UP;
      Player *me = getPlayer(&client->game, client->playerID);

      return ui_pan(ui,-1,0, &client->game, me);
    }
    if (sym == SDLK_RIGHT && mod & KMOD_SHIFT){
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = UP;
      Player *me = getPlayer(&client->game, client->playerID);

      return ui_pan(ui,1,0, &client->game, me);
    }
    if (sym == SDLK_UP && mod & KMOD_SHIFT){
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = UP;
      Player *me = getPlayer(&client->game, client->playerID);

      return ui_pan(ui, 0,-1, &client->game, me);
    }
    if (sym == SDLK_DOWN && mod & KMOD_SHIFT){
      Proto_Client *client = C->ph;
      client->rpc_session.shdr.returnCode = UP;
      Player *me = getPlayer(&client->game, client->playerID);

      return ui_pan(ui, 0,1, &client->game, me);
    }
    else {
      fprintf(stderr, "%s: key pressed: %d\n", __func__, sym); 
    }
  } else {
    fprintf(stderr, "%s: key released: %d\n", __func__, sym);
  }
  return 1;
}

void Wander(Client *C, int direction)
{
    if (direction == 0)
    {
        Proto_Client *client = (Proto_Client *) C->ph;
        client->rpc_session.shdr.returnCode = RIGHT;
        doRPCCmd(C, 'm');  // query map    
    }
    else if (direction == 1)
    {
        Proto_Client *client = (Proto_Client *) C->ph;
        client->rpc_session.shdr.returnCode = DOWN;
        doRPCCmd(C, 'm');  // query map
    }
    else if (direction == 2)
    {
        Proto_Client *client = (Proto_Client *) C->ph;
        client->rpc_session.shdr.returnCode = LEFT;
        doRPCCmd(C, 'm');  // query map
    }
    else if (direction == 3)
    {
        Proto_Client *client = (Proto_Client *) C->ph;
        client->rpc_session.shdr.returnCode = UP;
        doRPCCmd(C, 'm');  // query map
        direction = -1; //Resets direction
    }

    struct timespec tim, tim2;
    tim.tv_sec = 0;
    tim.tv_nsec = 500000000L;//Change speed of requests here

    nanosleep(&tim , &tim2);
    direction++;
    Wander(C, direction);
}
