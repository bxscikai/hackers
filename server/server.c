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
#include <poll.h>
#include <time.h>
#include "../lib/types.h"
#include "../lib/protocol_server.h"
#include "../lib/protocol_utils.h"

int 
doUpdateClients(void)
{
  Proto_Session *s;
  Proto_Msg_Hdr hdr;

  s = proto_server_event_session();
  hdr.type = PROTO_MT_EVENT_BASE_UPDATE;
  proto_session_hdr_marshall(s, &hdr);
  proto_server_post_event(PROTO_MT_EVENT_BASE_UPDATE);  
  return 1;
}

int 
broadCastMessage(void)
{
  proto_server_post_event(PROTO_MT_EVENT_BASE_UPDATE);    
  return 1;
}

char MenuString[] =
  "d/D-debug on/off u-update clients q-quit";

int 
docmd(char cmd)
{
  int rc = 1;

  switch (cmd) {
  case 'd':
    proto_debug_on();
    break;
  case 'D':
    proto_debug_off();
    break;
  case 'u':
    rc = doUpdateClients();
    break;
  case 'b':
    rc = broadCastMessage();
    break;    
  case 'q':
    rc=-1;
    break;
  case '\n':
  case ' ':
    rc=1;
    break;
  default:
    printf("Unkown Command\n");
  }
  return rc;
}

int
prompt(int menu) 
{
  // fprintf(stderr, "Menu:%d\n", menu);
  int ret;
  int c=0;

  if (menu) 
    printf("%s:", MenuString);

  fflush(stdout);
  c=getchar();
  return c;
}

void *
shell(void *arg)
{
  int c;
  int rc=1;
  int menu=1;

  while (1) {
    
    if ((c=prompt(menu))!=0) rc=docmd(c);
    if (rc<0) break;
    if (rc==1) menu=1; else menu=0;
  }
  fprintf(stderr, "terminating\n");
  fflush(stdout);
  return NULL;
}


// Reads the filename from std-in and tries to parse game map
int
readFile(int argc, char **argv)
{

  if (argc==1) {
    fprintf(stderr, "No inputs\n");
    return -1;
  }
  else if (argc==2) {

    return proto_server_parse_map(argv[1]);    

  }
  else if (argc>=3) {
    fprintf(stderr, "Too many inputs\n");
    return -1;
  }

  return -1;
}

int
main(int argc, char **argv)
{ 
  // Set seed for our random number generator
  srand(time(NULL));

  // Read in map from input
  int rc = readFile(argc, argv);

  // Returns if map parsing failed
  if (rc<0)
    exit (-1);

  if (proto_server_init()<0) {
    fprintf(stderr, "ERROR: failed to initialize proto_server subsystem\n");
    exit(-1);
  }

  fprintf(stderr, "RPC Port: %d, Event Port: %d\n", proto_server_rpcport(), 
    proto_server_eventport());

  if (proto_server_start_rpc_loop()<0) {
    fprintf(stderr, "ERROR: failed to start rpc loop\n");
    exit(-1);
  }
    
  shell(NULL);

  return 0;
}
