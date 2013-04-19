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
#include <stdlib.h> /* for exit() */
#include <pthread.h>
#include <assert.h>
#include "../lib/types.h"
#include "client.h"
#include "uistandalone.h"

/* A lot of this code comes from http://www.libsdl.org/cgi/docwiki.cgi */

/* Forward declaration of some dummy player code */
static void myPlayer_init(UI *ui, Player *myPlayer);
static void myPlayer_paint(UI *ui, SDL_Rect *t, Player *myPlayer);

static void paint_objects(UI *ui, SDL_Rect *t, int start_x, int start_y, int end_x, int end_y, void *game);

static void otherPlayers_init(UI *ui, void *game);
static void paint_players(UI *ui, SDL_Rect *t, int start_x, int start_y, int end_x, int end_y, void *game, Player *myPlayer);


#define SPRITE_H 32
#define SPRITE_W 32

#define UI_FLOOR_BMP "floorGREEN.bmp"
#define UI_FLOOR2_BMP "floorRED.bmp"
#define UI_REDWALL_BMP "redwall.bmp"
#define UI_GREENWALL_BMP "greenwall.bmp"
#define UI_TEAMA_BMP "teama.bmp"
#define UI_TEAMB_BMP "teamb.bmp"
#define UI_LOGO_BMP "logo.bmp"
#define UI_WIN "youwin.bmp"
#define UI_LOSE "youlose.bmp"
#define UI_REDFLAG_BMP "redflag.bmp"
#define UI_GREENFLAG_BMP "greenflag.bmp"
#define UI_JACKHAMMER_BMP "shovel.bmp"

typedef enum {UI_SDLEVENT_UPDATE, UI_SDLEVENT_QUIT} UI_SDL_Event;

struct UI_Player_Struct {
  SDL_Surface *img;
  uval base_clip_x;
  SDL_Rect clip;
};
typedef struct UI_Player_Struct UI_Player;

UI_Player *uiplayer;
UI_Player* ui_team1Players[MAX_NUM_PLAYERS];
UI_Player* ui_team2Players[MAX_NUM_PLAYERS];

static inline SDL_Surface *
ui_player_img(UI *ui, int team)
{  
  return (team == 0) ? ui->sprites[TEAMA_S].img 
    : ui->sprites[TEAMB_S].img;
}

static inline sval 
pxSpriteOffSet(int team, int state)
{
  if (state == 1)
    return (team==0) ? SPRITE_W*1 : SPRITE_W*2;
  if (state == 2) 
    return (team==0) ? SPRITE_W*2 : SPRITE_W*1;
  if (state == 3) return SPRITE_W*3;
  return 0;
}

static sval
ui_uip_init(UI *ui, UI_Player **p, int id, int team)
{
  UI_Player *ui_p;
  
  ui_p = (UI_Player *)malloc(sizeof(UI_Player));
  if (!ui_p) return 0;

  ui_p->img = ui_player_img(ui, team);
  ui_p->clip.w = SPRITE_W; ui_p->clip.h = SPRITE_H; ui_p->clip.y = 0;
  ui_p->base_clip_x = id * SPRITE_W * 4;

  *p = ui_p;

  return 1;
}

/*
 * Return the pixel value at (x, y)
 * NOTE: The surface must be locked before calling this!
 */
static uint32_t 
ui_getpixel(SDL_Surface *surface, int x, int y)
{
  int bpp = surface->format->BytesPerPixel;
  /* Here p is the address to the pixel we want to retrieve */
  uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;
  
  switch (bpp) {
  case 1:
    return *p;
  case 2:
    return *(uint16_t *)p;
  case 3:
    if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
      return p[0] << 16 | p[1] << 8 | p[2];
    else
      return p[0] | p[1] << 8 | p[2] << 16;
  case 4:
    return *(uint32_t *)p;
  default:
    return 0;       /* shouldn't happen, but avoids warnings */
  } // switch
}

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
static void 
ui_putpixel(SDL_Surface *surface, int x, int y, uint32_t pixel)
 {
   int bpp = surface->format->BytesPerPixel;
   /* Here p is the address to the pixel we want to set */
   uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

   switch (bpp) {
   case 1:
	*p = pixel;
	break;
   case 2:
     *(uint16_t *)p = pixel;
     break;     
   case 3:
     if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
       p[0] = (pixel >> 16) & 0xff;
       p[1] = (pixel >> 8) & 0xff;
       p[2] = pixel & 0xff;
     }
     else {
       p[0] = pixel & 0xff;
       p[1] = (pixel >> 8) & 0xff;
       p[2] = (pixel >> 16) & 0xff;
     }
     break;
 
   case 4:
     *(uint32_t *)p = pixel;
     break;
 
   default:
     break;           /* shouldn't happen, but avoids warnings */
   } // switch
 }

static 
sval splash(UI *ui)
{
  SDL_Rect r;
  SDL_Surface *temp;


  temp = SDL_LoadBMP(UI_LOGO_BMP);
  
  if (temp != NULL) {
    ui->sprites[LOGO_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    r.h = ui->sprites[LOGO_S].img->h;
    r.w = ui->sprites[LOGO_S].img->w;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
    //    printf("r.h=%d r.w=%d r.x=%d r.y=%d\n", r.h, r.w, r.x, r.y);
    SDL_BlitSurface(ui->sprites[LOGO_S].img, NULL, ui->screen, &r);
  } else {
    /* Map the color yellow to this display (R=0xff, G=0xFF, B=0x00)
       Note:  If the display is palettized, you must set the palette first.
    */
    r.h = 40;
    r.w = 80;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
 
    /* Lock the screen for direct access to the pixels */
    if ( SDL_MUSTLOCK(ui->screen) ) {
      if ( SDL_LockSurface(ui->screen) < 0 ) {
	fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
	return -1;
      }
    }
    SDL_FillRect(ui->screen, &r, ui->yellow_c);

    if ( SDL_MUSTLOCK(ui->screen) ) {
      SDL_UnlockSurface(ui->screen);
    }
  }
  /* Update just the part of the display that we've changed */
  SDL_UpdateRect(ui->screen, r.x, r.y, r.w, r.h);

  SDL_Delay(1000);
  return 1;
}

static 
sval splash_win(UI *ui)
{
  SDL_Rect r;
  SDL_Surface *temp;


  temp = SDL_LoadBMP(UI_WIN);
  
  if (temp != NULL) {
    ui->sprites[LOGO_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    r.h = ui->sprites[LOGO_S].img->h;
    r.w = ui->sprites[LOGO_S].img->w;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
    //    printf("r.h=%d r.w=%d r.x=%d r.y=%d\n", r.h, r.w, r.x, r.y);
    SDL_BlitSurface(ui->sprites[LOGO_S].img, NULL, ui->screen, &r);
  } else {
    /* Map the color yellow to this display (R=0xff, G=0xFF, B=0x00)
       Note:  If the display is palettized, you must set the palette first.
    */
    r.h = 40;
    r.w = 80;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
 
    /* Lock the screen for direct access to the pixels */
    if ( SDL_MUSTLOCK(ui->screen) ) {
      if ( SDL_LockSurface(ui->screen) < 0 ) {
  fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
  return -1;
      }
    }
    SDL_FillRect(ui->screen, &r, ui->yellow_c);

    if ( SDL_MUSTLOCK(ui->screen) ) {
      SDL_UnlockSurface(ui->screen);
    }
  }
  /* Update just the part of the display that we've changed */
  SDL_UpdateRect(ui->screen, r.x, r.y, r.w, r.h);

  SDL_Delay(1000);
  return 1;
}

static 
sval splash_lose(UI *ui)
{
  SDL_Rect r;
  SDL_Surface *temp;


  temp = SDL_LoadBMP(UI_LOSE);
  
  if (temp != NULL) {
    ui->sprites[LOGO_S].img = SDL_DisplayFormat(temp);
    SDL_FreeSurface(temp);
    r.h = ui->sprites[LOGO_S].img->h;
    r.w = ui->sprites[LOGO_S].img->w;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
    //    printf("r.h=%d r.w=%d r.x=%d r.y=%d\n", r.h, r.w, r.x, r.y);
    SDL_BlitSurface(ui->sprites[LOGO_S].img, NULL, ui->screen, &r);
  } else {
    /* Map the color yellow to this display (R=0xff, G=0xFF, B=0x00)
       Note:  If the display is palettized, you must set the palette first.
    */
    r.h = 40;
    r.w = 80;
    r.x = ui->screen->w/2 - r.w/2;
    r.y = ui->screen->h/2 - r.h/2;
 
    /* Lock the screen for direct access to the pixels */
    if ( SDL_MUSTLOCK(ui->screen) ) {
      if ( SDL_LockSurface(ui->screen) < 0 ) {
  fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
  return -1;
      }
    }
    SDL_FillRect(ui->screen, &r, ui->yellow_c);

    if ( SDL_MUSTLOCK(ui->screen) ) {
      SDL_UnlockSurface(ui->screen);
    }
  }
  /* Update just the part of the display that we've changed */
  SDL_UpdateRect(ui->screen, r.x, r.y, r.w, r.h);

  SDL_Delay(1000);
  return 1;
}

static sval
load_sprites(UI *ui) 
{
  SDL_Surface *temp;
  sval colorkey;

  /* setup sprite colorkey and turn on RLE */
  // FIXME:  Don't know why colorkey = purple_c; does not work here???
  colorkey = SDL_MapRGB(ui->screen->format, 255, 0, 255);
  
  temp = SDL_LoadBMP(UI_TEAMA_BMP);
  if (temp == NULL) { 
    fprintf(stderr, "ERROR: loading teama.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[TEAMA_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[TEAMA_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_TEAMB_BMP);
  if (temp == NULL) { 
    fprintf(stderr, "ERROR: loading teamb.bmp: %s\n", SDL_GetError()); 
    return -1;
  }
  ui->sprites[TEAMB_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);

  SDL_SetColorKey(ui->sprites[TEAMB_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_FLOOR_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading floor.bmp %s\n", SDL_GetError()); 
    return -1;
  }
  ui->sprites[FLOOR_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[FLOOR_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_FLOOR2_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading floor2.bmp %s\n", SDL_GetError()); 
    return -1;
  }
  ui->sprites[FLOOR2_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[FLOOR2_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
      colorkey);

  temp = SDL_LoadBMP(UI_REDWALL_BMP);
  if (temp == NULL) { 
    fprintf(stderr, "ERROR: loading redwall.bmp: %s\n", SDL_GetError());
    return -1;
  }
  ui->sprites[REDWALL_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[REDWALL_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL, 
		  colorkey);

  temp = SDL_LoadBMP(UI_GREENWALL_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading greenwall.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[GREENWALL_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[GREENWALL_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);

  temp = SDL_LoadBMP(UI_REDFLAG_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading redflag.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[REDFLAG_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[REDFLAG_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);

  temp = SDL_LoadBMP(UI_GREENFLAG_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading redflag.bmp: %s", SDL_GetError()); 
    return -1;
  }
  ui->sprites[GREENFLAG_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[GREENFLAG_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);

  temp = SDL_LoadBMP(UI_JACKHAMMER_BMP);
  if (temp == NULL) {
    fprintf(stderr, "ERROR: loading %s: %s", UI_JACKHAMMER_BMP, SDL_GetError()); 
    return -1;
  }
  ui->sprites[JACKHAMMER_S].img = SDL_DisplayFormat(temp);
  SDL_FreeSurface(temp);
  SDL_SetColorKey(ui->sprites[JACKHAMMER_S].img, SDL_SRCCOLORKEY | SDL_RLEACCEL,
		  colorkey);
  
  return 1;
}


inline static void
draw_cell(UI *ui, SPRITE_INDEX si, SDL_Rect *t, SDL_Surface *s)
{
  SDL_Surface *ts=NULL;
  uint32_t tc;

  ts = ui->sprites[si].img;

  if ( ts && t->h == SPRITE_H && t->w == SPRITE_W) 
    SDL_BlitSurface(ts, NULL, s, t);
}

static sval
ui_paintmap(UI *ui, void *game, Player *myPlayer) 
{
  Game *gameState = (Game *)game;
  Maze *maze = &gameState->map;
  SDL_Rect t;
  int player_x = myPlayer->cellposition.x;
  int player_y = myPlayer->cellposition.y;
  int i = 0;
  int j = 0;
  t.y = 0; t.x = 0; t.h = ui->tile_h; t.w = ui->tile_w;

  int start_x;
  int end_x;
  int start_y;
  int end_y;

  fprintf(stderr, "Painting map...\n");

  //Keep window consistently 20x20
  if (player_x < 10 && player_y < 10){
    start_x = 0;
    end_x = 19;
    start_y = 0;
    end_y = 19;
  }else if (player_x > 190 && player_y > 190){
    start_y = 180;
    end_y = 199;
    start_x = 180;
    end_x = 199;
  }else if (player_x < 10 && player_y > 190){
    start_y = 180;
    end_y = 199;
    start_x = 0;
    end_x = 19;
  }else if (player_x > 190 && player_y < 10){
    start_y = 0;
    end_y = 19;
    start_x = 180;
    end_x = 199;
  }else if (player_x < 10){
    start_x = 0;
    end_x = 19;
    start_y = player_y - 10;
    end_y = player_y + 9;
  }else if (player_y < 10){
    start_x = player_x - 10;
    end_x = player_x + 9;
    start_y = 0;
    end_y= 19;
  }else if (player_x > 190){
    start_x = 180;
    end_x = 199;
    start_y = player_y - 10;
    end_y = player_y + 9;
  }else if (player_y >190){
    start_y = 180;
    end_y = 199;
    start_x = player_x - 10;
    end_x = player_x + 9;
  }else{
    start_y = player_y - 10;
    end_y = player_y + 9;
    start_x = player_x - 10;
    end_x = player_x + 9;
  }

  for (i= start_y, t.y=0; i<= end_y; i++) {
    for (j= start_x, t.x=0; j<= end_x; j++) {
      int cell_type = maze->mapBody[i][j].type;

      if(cell_type == FLOOR_1 || cell_type == HOME_1 || cell_type == JAIL_1){
        draw_cell(ui, FLOOR_S, &t, ui->screen); 
      }else if (cell_type == FLOOR_2 || cell_type == HOME_2 || cell_type == JAIL_2){
        draw_cell(ui, FLOOR2_S, &t, ui->screen);
      }else if (cell_type == WALL_FIXED){
        draw_cell(ui, GREENWALL_S, &t, ui->screen);
      }else if (cell_type == WALL_UNFIXED){
        draw_cell(ui, REDWALL_S, &t, ui->screen);
      } else{
        draw_cell(ui, FLOOR_S, &t, ui->screen);
      }
      t.x+=t.w;
    }
    t.y+=t.h;
  }

  fprintf(stderr, "Painting my player...\n");
  myPlayer_paint(ui, &t, myPlayer);
  paint_players(ui, &t, start_x, start_y, end_x, end_y, game, myPlayer);
  paint_objects(ui, &t, start_x, start_y, end_x, end_y, game);

  SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);

  return 1;
}

static sval
ui_init_sdl(UI *ui, int32_t h, int32_t w, int32_t d)
{

  fprintf(stderr, "UI_init: Initializing SDL.\n");

  /* Initialize defaults, Video and Audio subsystems */
  if((SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)==-1)) { 
    fprintf(stderr, "Could not initialize SDL: %s.\n", SDL_GetError());
    return -1;
  }

  atexit(SDL_Quit);

  fprintf(stderr, "ui_init: h=%d w=%d d=%d\n", h, w, d);

  ui->depth = d;
  ui->screen = SDL_SetVideoMode(w, h, ui->depth, SDL_SWSURFACE);
  if ( ui->screen == NULL ) {
    fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n", w, h, ui->depth, 
	    SDL_GetError());
    return -1;
  }
    
  fprintf(stderr, "UI_init: SDL initialized.\n");


  if (load_sprites(ui)<=0) return -1;

  ui->black_c      = SDL_MapRGB(ui->screen->format, 0x00, 0x00, 0x00);
  ui->white_c      = SDL_MapRGB(ui->screen->format, 0xff, 0xff, 0xff);
  ui->red_c        = SDL_MapRGB(ui->screen->format, 0xff, 0x00, 0x00);
  ui->green_c      = SDL_MapRGB(ui->screen->format, 0x00, 0xff, 0x00);
  ui->yellow_c     = SDL_MapRGB(ui->screen->format, 0xff, 0xff, 0x00);
  ui->purple_c     = SDL_MapRGB(ui->screen->format, 0xff, 0x00, 0xff);

  ui->isle_c         = ui->black_c;
  ui->wall_teama_c   = ui->red_c;
  ui->wall_teamb_c   = ui->green_c;
  ui->player_teama_c = ui->red_c;
  ui->player_teamb_c = ui->green_c;
  ui->flag_teama_c   = ui->white_c;
  ui->flag_teamb_c   = ui->white_c;
  ui->jackhammer_c   = ui->yellow_c;
  
 
  /* set keyboard repeat */
  SDL_EnableKeyRepeat(70, 70);  

  SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
  SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);

  splash(ui);
  return 1;
}

static void
ui_shutdown_sdl(void)
{
  fprintf(stderr, "UI_shutdown: Quitting SDL.\n");
  SDL_Quit();
}

static sval
ui_userevent(UI *ui, SDL_UserEvent *e) 
{
  if (e->code == UI_SDLEVENT_UPDATE) return 2;
  if (e->code == UI_SDLEVENT_QUIT) return -1;
  return 0;
}

static sval
ui_process(UI *ui, Client *C)
{
  SDL_Event e;
  sval rc = 1;

  while(SDL_WaitEvent(&e)) {
    switch (e.type) {
    case SDL_QUIT:
      return -1;
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      rc = ui_keypress(ui, &(e.key), C);
      break;
    case SDL_ACTIVEEVENT:
      break;
    case SDL_USEREVENT:
      rc = ui_userevent(ui, &(e.user));
      break;
    default:
      fprintf(stderr, "%s: e.type=%d NOT Handled\n", __func__, e.type);
    }
    
    if (rc<0) break;
  }
  return rc;
}

extern sval
ui_zoom(UI *ui, sval fac)
{
  fprintf(stderr, "%s:\n", __func__);
  return 2;
}

extern sval
ui_pan(UI *ui, sval xdir, sval ydir)
{
  fprintf(stderr, "%s:\n", __func__);
  return 2;
}

extern sval
ui_move(UI *ui, sval xdir, sval ydir)
{
  fprintf(stderr, "%s:\n", __func__);
  return 1;
}


extern void
ui_update(UI *ui)
{
  SDL_Event event;
  
  event.type      = SDL_USEREVENT;
  event.user.code = UI_SDLEVENT_UPDATE;
  SDL_PushEvent(&event);

}


extern void
ui_quit(UI *ui)
{  
  SDL_Event event;
  fprintf(stderr, "ui_quit: stopping ui...\n");
  event.type      = SDL_USEREVENT;
  event.user.code = UI_SDLEVENT_QUIT;
  SDL_PushEvent(&event);
}

extern void
ui_repaint(UI *ui, void *game, Player *myPlayer)
{
  ui_paintmap(ui, game, myPlayer);
  SDL_UpdateRect(ui->screen, 0, 0, ui->screen->w, ui->screen->h);
  Game *gameState = (Game *)game;
  if (gameState->status == TEAM_1_WON){
    if (myPlayer->team == TEAM_1){
      splash_win(ui);
    }else{
      splash_lose(ui);
    }
  }

  if (gameState->status == TEAM_2_WON){
    if (myPlayer->team == TEAM_2){
      splash_win(ui);
    }else{
      splash_lose(ui);
    }
  }
}

extern void
ui_main_loop(UI *ui, uval h, uval w, void *game, Player *myPlayer, Client *C)
{
  sval rc;
  
  assert(ui);

  ui_init_sdl(ui, h, w, 32);

  myPlayer_init(ui, myPlayer);
  
  otherPlayers_init(ui, game);
  
  ui_paintmap(ui, game, myPlayer);
  
  while (1) {
    if (ui_process(ui, C)<0) break;
  }

  ui_shutdown_sdl();
}

extern void
ui_init(UI **ui)
{
  *ui = (UI *)malloc(sizeof(UI));
  if (ui==NULL) return;

  bzero(*ui, sizeof(UI));
  
  (*ui)->tile_h = SPRITE_H;
  (*ui)->tile_w = SPRITE_W;

}

static void 
myPlayer_init(UI *ui, Player *myPlayer) 
{
  int state = 0;
  // set what state the player is
  if (myPlayer->canMove == 1){
    state = NORMAL;
  }

  if (myPlayer->canMove == 0){
    state = JAIL;
  }

  if (myPlayer->inventory.type == FLAG_1){
    state = RED_FLAG;
  }

  if (myPlayer->inventory.type == FLAG_2){
    state = GREEN_FLAG;
  }

  myPlayer->current_state = state;
  pthread_mutex_init(&(myPlayer->lock), NULL);
  ui_uip_init(ui, &uiplayer, myPlayer->playerID, myPlayer->team);
}

static void
otherPlayers_init(UI *ui, void *game)
{
   Game *gameState = (Game *)game;
  
  int i;
  for (i = 0; i< MAX_NUM_PLAYERS ; i++){
    Player this_player = gameState->Team1_Players[i];

    if(this_player.playerID != 0){
      int state = 0;
      // set what state the player is
      if (this_player.canMove == 1){
        state = NORMAL;
      }

      if (this_player.canMove == 0){
        state = JAIL;
      }

      if (this_player.inventory.type == FLAG_1){
        state = RED_FLAG;
      }

      if (this_player.inventory.type == FLAG_2){
        state = GREEN_FLAG;
      }

      pthread_mutex_init(&(this_player.lock), NULL);
      ui_uip_init(ui, &ui_team1Players[i], this_player.playerID, this_player.team);
    }
  }

  for (i = 0; i< MAX_NUM_PLAYERS ; i++){
    Player this_player = gameState->Team2_Players[i];

    if(this_player.playerID != 0){
      int state = 0;
      // set what state the player is
      if (this_player.canMove == 1){
        state = NORMAL;
      }

      if (this_player.canMove == 0){
        state = JAIL;
      }

      if (this_player.inventory.type == FLAG_1){
        state = RED_FLAG;
      }

      if (this_player.inventory.type == FLAG_2){
        state = GREEN_FLAG;
      }

      pthread_mutex_init(&(this_player.lock), NULL);
      ui_uip_init(ui, &ui_team2Players[i], this_player.playerID, this_player.team);
    }
  }
}

static void 
myPlayer_paint(UI *ui, SDL_Rect *t, Player *myPlayer)
{
  // update what state the player is in
  int state = 0;
  if (myPlayer->canMove == 1){
    state = NORMAL;
  }

  if (myPlayer->canMove == 0){
    state = JAIL;
  }

  if (myPlayer->inventory.type == FLAG_1){
    state = RED_FLAG;
  }

  if (myPlayer->inventory.type == FLAG_2){
    state = GREEN_FLAG;
  }

  myPlayer->current_state = state;
  ////////

  pthread_mutex_lock(&myPlayer->lock);

  int player_x = myPlayer->cellposition.x;
  int player_y = myPlayer->cellposition.y;

  if(player_x <= 10 && player_y <= 10){
    t->y = player_y * t->h; t->x = player_x * t->w;
  }else if(player_x >= 190 && player_y <= 10){
    t->y = player_y * t->h; t->x = (20 - (200 - player_x)) * t->w;
  }else if(player_x <= 10 && player_y >= 190){
    t->y = (20 - (200 - player_y)) * t->h; t->x = player_x * t->w;
  }else if(player_x >= 190 && player_y >= 190){
    t->y = (20 - (200 - player_y)) * t->h; t->x = (20 - (200 - player_x)) * t->w;

  }else if(player_y <= 10){
    t->y = player_y * t->h; t->x = 10 * t->w;
  }else if(player_x <= 10){
    t->y = 10 * t->h; t->x = player_x * t->w;
  }else if(player_y >= 190){
    t->y = (20 - (200 - player_y)) * t->h; t->x = 10 * t->w;
  }else if(player_x >= 190){
    t->y = 10 * t->h; t->x = (20 - (200 - player_x)) * t->w;
  }else{
    t->y = 320; t->x = 320; //center of window
  }

  /////
  //UI_Player *uiplayer = myPlayer->uiPlayer;
  //////
  uiplayer->clip.x = uiplayer->base_clip_x +
    pxSpriteOffSet(myPlayer->team, myPlayer->current_state);
  SDL_BlitSurface(uiplayer->img, &(uiplayer->clip), ui->screen, t);

  pthread_mutex_unlock(&myPlayer->lock);
}

static void
paint_players(UI *ui, SDL_Rect *t, int start_x, int start_y, int end_x, int end_y, void *game, Player *myPlayer)
{
  Game *gameState = (Game *)game;
  int i;
  for (i = 0; i< MAX_NUM_PLAYERS ; i++){
    Player this_player = gameState->Team1_Players[i];

    // update what state the player is in
    int state = 0;
    if (this_player.canMove == 1){
      state = NORMAL;
    }

    if (this_player.canMove == 0){
      state = JAIL;
    }

    if (this_player.inventory.type == FLAG_1){
      state = RED_FLAG;
    }

    if (this_player.inventory.type == FLAG_2){
      state = GREEN_FLAG;
    }

    this_player.current_state = state;
    ////////

    // only paint valid players in the 20x20 range
    if(this_player.playerID != 0 && this_player.playerID != myPlayer->playerID){
      if(this_player.cellposition.x >= start_x && this_player.cellposition.x <= end_x && this_player.cellposition.y >= start_y && this_player.cellposition.y <= end_y){
        pthread_mutex_lock(&this_player.lock);
        t->y = (this_player.cellposition.y - start_y) * t->h; t->x = (this_player.cellposition.x - start_x) * t->w;
        ui_team1Players[i]->clip.x = ui_team1Players[i]->base_clip_x +
          pxSpriteOffSet(this_player.team, this_player.current_state);
        SDL_BlitSurface(ui_team1Players[i]->img, &(ui_team1Players[i]->clip), ui->screen, t);
        pthread_mutex_unlock(&this_player.lock);
      }
    }

  }

  for (i = 0; i< MAX_NUM_PLAYERS ; i++){
    Player this_player = gameState->Team2_Players[i];

    // update what state the player is in
    int state = 0;
    if (this_player.canMove == 1){
      state = NORMAL;
    }

    if (this_player.canMove == 0){
      state = JAIL;
    }

    if (this_player.inventory.type == FLAG_1){
      state = RED_FLAG;
    }

    if (this_player.inventory.type == FLAG_2){
      state = GREEN_FLAG;
    }

    this_player.current_state = state;
    ////////

    // only paint valid players in the 20x20 range
    if(this_player.playerID != 0 && this_player.playerID != myPlayer->playerID){
      if(this_player.cellposition.x >= start_x && this_player.cellposition.x <= end_x && this_player.cellposition.y >= start_y && this_player.cellposition.y <= end_y){
        pthread_mutex_lock(&this_player.lock);
        t->y = (this_player.cellposition.y - start_y) * t->h; t->x = (this_player.cellposition.x - start_x) * t->w;
        ui_team2Players[i]->clip.x = ui_team2Players[i]->base_clip_x +
          pxSpriteOffSet(this_player.team, this_player.current_state);
        SDL_BlitSurface(ui_team2Players[i]->img, &(ui_team2Players[i]->clip), ui->screen, t);
        pthread_mutex_unlock(&this_player.lock);
      }
    }
  }
  
}

static void
paint_objects(UI *ui, SDL_Rect *t, int start_x, int start_y, int end_x, int end_y, void *game){
  Game *gameState = (Game *)game;

  int i;
  for (i = 0; i< NUMOFOBJECTS ; i++){
    Object this_object = gameState->map.objects[i];
    if(this_object.cellposition.x >= start_x && this_object.cellposition.x <= end_x && this_object.cellposition.y >= start_y && this_object.cellposition.y <= end_y){

      t->y = (this_object.cellposition.y - start_y) * t->h; t->x = (this_object.cellposition.x - start_x) * t->w;

      if(this_object.type == JACKHAMMER1 || this_object.type == JACKHAMMER2){
        draw_cell(ui, JACKHAMMER_S, t, ui->screen);
      }
      if(this_object.type == FLAG_1){
        draw_cell(ui, REDFLAG_S, t, ui->screen);
      }
      if(this_object.type == FLAG_2){
        draw_cell(ui, GREENFLAG_S, t, ui->screen);
      }
    }
  }
}
