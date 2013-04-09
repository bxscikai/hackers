#ifndef __CLIENT_H__
#define __CLIENT_H__

// To handle be able to handle all the UI code only on client side.
#include <unistd.h>

#include "../lib/protocol_client.h"


typedef struct ClientState  {
  int data;
  Proto_Client_Handle ph;
} Client;


void start_UI(Proto_Client *c, Player *myPlayer);

#endif
