#ifndef __DA_GAME_UI_H__
#define __DA_GAME_UI_H__
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

#include <SDL/SDL.h>   /* All SDL apps need this */




typedef enum { 
  TEAMA_S=0, 
  TEAMB_S, 
  TEAMA_XS, 
  TEAMB_XS, 
  FLOOR2_S, 
  FLOOR2_XS, 
  FLOOR_S,
  FLOOR_XS,  
  REDWALL_S,
  REDWALL_XS, 
  GREENWALL_S,
  GREENWALL_XS, 
  LOGO_S, 
  JACKHAMMER_S,
  JACKHAMMER_XS,  
  REDFLAG_S, 
  REDFLAG_XS, 
  GREENFLAG_S,
  GREENFLAG_XS,
  NUM_S 
} SPRITE_INDEX;

typedef enum {PAN, NOT_PAN} PAINT_TYPE;

struct UI_Struct {
  SDL_Surface *screen;
  int32_t depth;
  int32_t tile_h;
  int32_t tile_w;

  struct Sprite {
    SDL_Surface *img;
  } sprites[NUM_S];

  uint32_t red_c;
  uint32_t green_c;
  uint32_t blue_c;
  uint32_t white_c;
  uint32_t black_c;
  uint32_t yellow_c;
  uint32_t purple_c;
  uint32_t isle_c;
  uint32_t wall_teama_c;
  uint32_t wall_teamb_c;
  uint32_t player_teama_c;
  uint32_t player_teamb_c;
  uint32_t flag_teama_c;
  uint32_t flag_teamb_c;
  uint32_t jackhammer_c;
};

typedef struct UI_Struct UI;


sval ui_zoom(UI *ui, sval fac, void *game, Player *myPlayer);
sval ui_pan(UI *ui, int xdir, int ydir, void *game, Player *myPlayer);
sval ui_move(UI *ui, sval xdir, sval ydir);
sval ui_keypress(UI *ui, SDL_KeyboardEvent *e, Client *C);
void ui_update(UI *ui);
void ui_quit(UI *ui);
void ui_repaint(UI *ui, void *game, Player *myPlayer);
void ui_main_loop(UI *ui, uval h, uval w, void *map, Player *myPlayer, Client *C);
void ui_init(UI **ui);


// DUMMY TEST CALLS
int ui_dummy_normal(UI *ui);
int ui_dummy_pickup_red(UI *ui);
int ui_dummy_pickup_green(UI *ui);
int ui_dummy_jail(UI *ui);
int ui_dummy_toggle_team(UI *ui);
int ui_dummy_inc_id(UI *ui);

#endif
