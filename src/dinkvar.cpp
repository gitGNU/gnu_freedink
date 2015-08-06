/**
 * Common code for FreeDink and FreeDinkedit

 * Copyright (C) 1997, 1998, 1999, 2002, 2003  Seth A. Robinson
 * Copyright (C) 2003  Shawn Betts
 * Copyright (C) 2005, 2006  Dan Walma
 * Copyright (C) 2005, 2007, 2008, 2009, 2010, 2012, 2014  Sylvain Beucler

 * This file is part of GNU FreeDink

 * GNU FreeDink is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.

 * GNU FreeDink is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> /* strncasecmp */
#include <ctype.h>
#include <time.h>

#include <fcntl.h>

#include "SDL.h"
#include "SDL_image.h"

#include "game_engine.h"
#include "live_sprites_manager.h"
#include "live_screen.h"
#include "map.h"
#include "screen.h"
#include "hardness_tiles.h"
#include "dinkini.h"
#include "input.h"

#include "fastfile.h"
#include "io_util.h"


#include "freedink.h"
#include "dinkvar.h"
#include "gfx.h"
#include "gfx_tiles.h"
#include "gfx_sprites.h"
#include "gfx_palette.h"
/* for DinkC's initfonts(): */
#include "gfx_fonts.h"
#include "bgm.h"
#include "sfx.h"
#include "dinkc.h"
#include "dinkc_bindings.h"

#include "str_util.h"
#include "paths.h"
#include "log.h"

int dinkspeed = 3;
int show_inventory = 0; // display inventory?

void update_status_all(void);
int add_sprite(int x1, int y, int brain,int pseq, int pframe );

void add_exp(int num, int h);
void draw_status_all(void);
void check_seq_status(int h);

int realhard(int tile);
int flub_mode = -500;
int draw_screen_tiny = -1;

int walk_off_screen = /*false*/0;

/* Skip flipping the double buffer for this frame only - used when
   setting up show_bmp and copy_bmp */
/*bool*/int abort_this_flip = /*false*/0;

int screenlock = 0;

struct show_bmp showb;

int keep_mouse = 0;


struct attackinfo_struct bow;

unsigned long mold;

int mbase_count;


int push_active = 1;


int stop_entire_game;
const int max_game = 20;
/*bool*/int in_enabled = /*false*/0;
char *in_string;


char dir[80];


//defaults




unsigned long timer = 0;
char *command_line;
/*bool*/int dinkedit = /*false*/0;
int base_timing = 0;

int sp_mode = 0;
int fps,fps_final = 0;
int move_screen = 0;
int move_counter = 0;
int playx = 620;
/*bool*/int windowed = /*false*/0; /* TODO: move to gfx.c? */
int playl = 20;

int playy = 400;
int cur_map;
int* pplayer_map;

/* Number of ms since an arbitrarily fixed point */
Uint32 thisTickCount,lastTickCount;
/* SDL_gfx accurate framerate */
FPSmanager framerate_manager;

unsigned long timecrap;
rect math,box_crap,box_real;


int mode = 0;


/* Screen transition */
/*bool*/int transition_in_progress = /*false*/0;


/* LPDIRECTDRAWSURFACE     game[max_game];       // Game pieces */
/* LPDIRECTDRAWPALETTE     lpDDPal = NULL;        // The primary surface palette */
/* PALETTEENTRY    pe[256]; */

int bActive = /*false*/0;        // is application active/foreground?
//LPDIRECTINPUT lpDI;


//LPCDIDATAFORMAT lpc;

unsigned char torusColors[256];  // Marks the colors used in the torus




char * lmon(long money, char *dest)
{
        char ho[30];
        int k,c;
        char lmon1[30];
        char buffer[30];
        /*BOOL*/int quit1;
        quit1 = /*FALSE*/0;

        sprintf(buffer, "%ld", money);
        strcpy(lmon1, buffer);
        // prf("ORG IS '%s'",lmon1);

        if (strlen(lmon1) < 4)
        {
                strcpy(dest, lmon1);
                return(dest);
        }

        sprintf(buffer, "%ld", money);
        strcpy(ho, buffer);
        k = strlen(ho);
        c = -1;
        lmon1[0]=0;
        do {
                strchar(lmon1,ho[k]);
                k--;
                c++;
                if (c == 3)
                {
                        if (k > -1)
                        {
                                strchar(lmon1,',');
                                c = 0;
                        }
                }
                if (k < 0) quit1 = /*TRUE*/1;
        }while (quit1 == /*FALSE*/0);
        reverse(lmon1);

        strcpy(dest, lmon1);
        return(dest);
}

/**
 * Check whether planned new position (x1,y1) is solid
 * 
 * Does something weird when hard value is > 100??
 * 
 * Only used in 'human_brain'
 */
unsigned char get_hard_play(int h, int x1, int y1)
{
  x1 -= 20;

  if (screenlock)
    {
      if (x1 < 0)        x1 = 0;
      else if (x1 > 599) x1 = 599;

      if (y1 < 0)        y1 = 0;
      else if (y1 > 399) y1 = 399;
    }
  if ((x1 < 0) || (y1 < 0) || (x1 > 599) || (y1 > 399))
    return 0;

  int value =  screen_hitmap[x1][y1];
  if (value > 100 && cur_screen.sprite[value-100].is_warp != 0)
    {
      flub_mode = value;
      value = 0;
    }
  return(value);
}


unsigned char get_hard_map(int h,int x1, int y1)
{


        if ((x1 < 0) || (y1 < 0)) return(0);
        if ((x1 > 599) ) return(0);
        if (y1 > 399) return(0);


        int til = (x1 / 50) + ( ((y1 / 50)) * 12);
        //til++;

        int offx = x1 - ((x1 / 50) * 50);


        int offy = y1 - ((y1 / 50) * 50);

        //Msg("tile %d ",til);

        return( hmap.htile[ realhard(til )  ].hm[offx][offy]);

}



void fill_hardxy(rect box)
{
  int x1, y1;
  //Msg("filling hard of %d %d %d %d", box.top, box.left, box.right, box.bottom);

  if (box.right > 600)  box.right  = 600;
  if (box.top < 0)      box.top    = 0;
  if (box.bottom > 400) box.bottom = 400;
  if (box.left < 0)     box.left   = 0;

  for (x1 = box.left; x1 < box.right; x1++)
    for (y1 = box.top; y1 < box.bottom; y1++)
      screen_hitmap[x1][y1] = get_hard_map(0,x1,y1);
}


/**
 * Parse a dink.ini line, and store instructions for later processing
 * (used in game initialization through 'load_batch')
 */
void pre_figure_out(char* line)
{
  int i;
  char* ev[10];
  memset(&ev, 0, sizeof(ev));
  for (i = 0; i < 10; i++)
    ev[i] = separate_string(line, i+1, ' ');
  char *command = ev[0];

  // PLAYMIDI  filename
  if (compare(command, "playmidi"))
    {
      char* midi_filename = ev[1];
      if (!dinkedit)
	PlayMidi(midi_filename);
    }

  // LOAD_SEQUENCE_NOW  path  seq  BLACK
  // LOAD_SEQUENCE_NOW  path  seq  LEFTALIGN
  // LOAD_SEQUENCE_NOW  path  seq  NOTANIM
  // LOAD_SEQUENCE_NOW  path  seq  speed
  // LOAD_SEQUENCE_NOW  path  seq  speed  offsetx offsety  hard.left hard.top hard.right hard.bottom
  else if (compare(command, "LOAD_SEQUENCE_NOW"))
    {
      rect hardbox;
      memset(&hardbox, 0, sizeof(rect));

      int myseq = atol(ev[2]);
      seq[myseq].is_active = 1;
      seq_set_ini(myseq, line);

      int flags = 0;
      if (compare(ev[3], "BLACK"))
	{
	  flags = DINKINI_NOTANIM | DINKINI_BLACK;
	}
      else if (compare(ev[3], "LEFTALIGN"))
	{
	  flags = DINKINI_LEFTALIGN;
	}
      else if (compare(ev[3], "NOTANIM"))
	{
	  //not an animation!
	  flags = 0;
	}
      else
	{
	  //yes, an animation!
	  hardbox.left = atol(ev[6]);
	  hardbox.top = atol(ev[7]);
	  hardbox.right = atol(ev[8]);
	  hardbox.bottom = atol(ev[9]);

	  flags = DINKINI_NOTANIM;
	}

      load_sprites(ev[1],atol(ev[2]),atol(ev[3]),atol(ev[4]),atol(ev[5]),
		   hardbox, flags);


      /* In the original engine, due to a bug, make_idata() modifies
	 unused sequence #0, but this isn't really important because
	 sequence was already configured in was already done in
	 'load_sprites'. This is consistent with 'figure_out', which
	 doesn't call 'make_idata' at all. */
      /* We still call 'make_idata' for compatibility, to use the same
	 number of idata, hence preserving the same max_idata. */
      make_idata(IDATA_SPRITE_INFO, 0,0, 0,0, hardbox);
    }

  // LOAD_SEQUENCE  path  seq  BLACK
  // LOAD_SEQUENCE  path  seq  LEFTALIGN
  // LOAD_SEQUENCE  path  seq  NOTANIM
  // LOAD_SEQUENCE  path  seq  speed
  // LOAD_SEQUENCE  path  seq  speed  offsetx offsety  hard.left hard.top hard.right hard.bottom
  else if (compare(command, "LOAD_SEQUENCE"))
    {
      int myseq = atol(ev[2]);
      seq_set_ini(myseq, line);
      seq[myseq].is_active = 1;
    }
  
  else if (compare(command, "SET_SPRITE_INFO"))
    {
      //           name   seq    speed       offsetx     offsety       hardx      hardy
      //if (k[seq[myseq].frame[myframe]].frame = 0) Msg("Changing sprite that doesn't exist...");
      
      rect hardbox;
      int myseq = atol(ev[1]);
      int myframe = atol(ev[2]);
      rect_set(&hardbox, atol(ev[5]), atol(ev[6]), atol(ev[7]), atol(ev[8]));
      make_idata(IDATA_SPRITE_INFO, myseq, myframe,atol(ev[3]), atol(ev[4]),hardbox);
    }
  
  else if (compare(command, "SET_FRAME_SPECIAL"))
    {
      rect hardbox;
      int myseq = atol(ev[1]);
      int myframe = atol(ev[2]);
      int special = atol(ev[3]);
      make_idata(IDATA_FRAME_SPECIAL, myseq, myframe, special, 0, hardbox);
    }
  
  else if (compare(command, "SET_FRAME_DELAY"))
    {
      rect hardbox;
      int myseq = atol(ev[1]);
      int myframe = atol(ev[2]);
      int delay = atol(ev[3]);
      make_idata(IDATA_FRAME_DELAY, myseq, myframe, delay, 0, hardbox);
    }
  
  // SET_FRAME_FRAME  seq frame  new_seq new_frame
  // SET_FRAME_FRAME  seq frame  -1
  else if (compare(command, "SET_FRAME_FRAME"))
    {
      rect hardbox;
      int myseq = atol(ev[1]);
      int myframe = atol(ev[2]);
      int new_seq = atol(ev[3]);
      int new_frame = atol(ev[4]);
      
      make_idata(IDATA_FRAME_FRAME, myseq, myframe, new_seq, new_frame, hardbox);
    }

  /* Clean-up */
  for (i = 0; i < 10; i++)
    free(ev[i]);
}

/**
 * Parse a delayed seq[].ini or a DinkC init("...") , and act
 * immediately
 */
void figure_out(char* line)
{
  int myseq = 0, myframe = 0;
  int special = 0;
  int special2 = 0;
  int i;
  char* ev[10];
  memset(&ev, 0, sizeof(ev));
  for (i = 0; i < 10; i++)
    {
      ev[i] = separate_string(line, i+1, ' ');
      if (ev[i] == NULL)
	ev[i] = strdup("");
    }
  char *command = ev[0];

  // LOAD_SEQUENCE_NOW  path  seq  BLACK
  // LOAD_SEQUENCE_NOW  path  seq  LEFTALIGN
  // LOAD_SEQUENCE_NOW  path  seq  NOTANIM
  // LOAD_SEQUENCE_NOW  path  seq  speed  offsetx offsety  hard.left hard.top hard.right hard.bottom
  if (compare(command, "LOAD_SEQUENCE_NOW") ||
      compare(command, "LOAD_SEQUENCE"))
    {
      rect hardbox;
      memset(&hardbox, 0, sizeof(rect));

      int myseq = atol(ev[2]);
      seq[myseq].is_active = 1;
      seq_set_ini(myseq, line);

      int flags = 0;

      if (compare(ev[3], "BLACK"))
	{
	  flags = DINKINI_NOTANIM | DINKINI_BLACK;
	}
      else if (compare(ev[3], "LEFTALIGN"))
	{
	  flags = DINKINI_LEFTALIGN;
	}
      else if (compare(ev[3], "NOTANIM"))
	{
	  //not an animation!
	  flags = 0;
	}
      else
	{
	  //yes, an animation!
	  hardbox.left = atol(ev[6]);
	  hardbox.top = atol(ev[7]);
	  hardbox.right = atol(ev[8]);
	  hardbox.bottom = atol(ev[9]);
	  
	  flags = DINKINI_NOTANIM;
	}

      load_sprites(ev[1],atol(ev[2]),atol(ev[3]),atol(ev[4]),atol(ev[5]),
		   hardbox, flags);
      
      program_idata();
    }

  else if (compare(command, "SET_SPRITE_INFO"))
    {
      //           name   seq    speed       offsetx     offsety       hardx      hardy
      myseq = atol(ev[1]);
      myframe = atol(ev[2]);
      k[seq[myseq].frame[myframe]].xoffset = atol(ev[3]);
      k[seq[myseq].frame[myframe]].yoffset = atol(ev[4]);
      k[seq[myseq].frame[myframe]].hardbox.left = atol(ev[5]);
      k[seq[myseq].frame[myframe]].hardbox.top = atol(ev[6]);
      k[seq[myseq].frame[myframe]].hardbox.right = atol(ev[7]);
      k[seq[myseq].frame[myframe]].hardbox.bottom = atol(ev[8]);
    }
  
  else if (compare(command, "SET_FRAME_SPECIAL"))
    {
      //           name   seq    speed       offsetx     offsety       hardx      hardy
      myseq = atol(ev[1]);
      myframe = atol(ev[2]);
      special = atol(ev[3]);
      
      seq[myseq].special[myframe] = special;
      log_debug("Set special.  %d %d %d", myseq, myframe, special);
    }

  else if (compare(command, "SET_FRAME_DELAY"))
    {
      //           name   seq    speed       offsetx     offsety       hardx      hardy
      myseq = atol(ev[1]);
      myframe = atol(ev[2]);
      special = atol(ev[3]);
      
      seq[myseq].delay[myframe] = special;
      log_debug("Set delay.  %d %d %d",myseq, myframe, special);
    }

  else if (compare(command, "SET_FRAME_FRAME"))
    {
      //           name   seq    speed       offsetx     offsety       hardx      hardy
      myseq = atol(ev[1]);
      myframe = atol(ev[2]);
      special = atol(ev[3]);
      special2 = atol(ev[4]);
      
      if (special == -1)
	seq[myseq].frame[myframe] = special;
      else
	seq[myseq].frame[myframe] = seq[special].frame[special2];
      log_debug("Set frame.  %d %d %d", myseq, myframe, special);
    }

  /* Clean-up */
  for (i = 0; i < 10; i++)
    free(ev[i]);
}

/*bool*/int inside_box(int x1, int y1, rect box)
{

        if (x1 > box.right) return(/*false*/0);
        if (x1 < box.left) return(/*false*/0);

        if (y1 > box.bottom) return(/*false*/0);
        if (y1 < box.top) return(/*false*/0);

        return(/*true*/1);

}


/*bool*/int get_box (int h, rect * box_scaled, rect * box_real)
{
  int x_offset, y_offset;

  int mplayx = playx;
  int mplayl = playl;
  int mplayy = playy;

  if (spr[h].noclip)
    {
      mplayx = 640;
      mplayl = 0;
      mplayy = 480;
    }

  // added to fix frame-not-in-memory immediately
  if (getpic(h) < 1)
    {
      if (spr[h].pseq != 0)
	check_seq_status(spr[h].pseq);
    }

  // if frame is still not in memory:
  if (getpic(h) < 1)
    {
      if (dinkedit)
	log_warn("Yo, sprite %d has a bad pic. (Map %d) Seq %d, Frame %d",
		 h, cur_map, spr[h].pseq, spr[h].pframe);
      else
	log_warn("Yo, sprite %d has a bad pic. (Map %d) Seq %d, Frame %d",
		 h, *pplayer_map, spr[h].pseq, spr[h].pframe);
      goto nodraw;
    }

  *box_real = k[getpic(h)].box;

  /* This doesn't really make sense, but that's the way the game was
     released, so we keep it for compatibility */
  {
    rect krect;
    rect_copy(&krect, &k[getpic(h)].box);

    double size_ratio = spr[h].size / 100.0;
    int x_compat = krect.right  * (size_ratio - 1) / 2;
    int y_compat = krect.bottom * (size_ratio - 1) / 2;

    int center_x = k[getpic(h)].xoffset;
    int center_y = k[getpic(h)].yoffset;
    box_scaled->left   = spr[h].x - center_x - x_compat;
    box_scaled->top    = spr[h].y - center_y - y_compat;

    box_scaled->right  = box_scaled->left + krect.right  * size_ratio;
    box_scaled->bottom = box_scaled->top  + krect.bottom * size_ratio;
  }

  if (spr[h].alt.right != 0 || spr[h].alt.left != 0 || spr[h].alt.top != 0)
    {
      // checks for correct box stuff
      if (spr[h].alt.left < 0)
	spr[h].alt.left = 0;
      if (spr[h].alt.left > k[getpic(h)].box.right)
	spr[h].alt.left = k[getpic(h)].box.right;

      if (spr[h].alt.top < 0)
	spr[h].alt.top = 0;
      if (spr[h].alt.top > k[getpic(h)].box.bottom)
	spr[h].alt.top = k[getpic(h)].box.bottom;

      if (spr[h].alt.right < 0)
	spr[h].alt.right = 0;
      if (spr[h].alt.right > k[getpic(h)].box.right)
	spr[h].alt.right = k[getpic(h)].box.right;

      if (spr[h].alt.bottom < 0)
	spr[h].alt.bottom = 0;
      if (spr[h].alt.bottom > k[getpic(h)].box.bottom)
	spr[h].alt.bottom = k[getpic(h)].box.bottom;

      box_scaled->left += spr[h].alt.left;
      box_scaled->top  += spr[h].alt.top;
      box_scaled->right  = box_scaled->right  - (k[getpic(h)].box.right  - spr[h].alt.right);
      box_scaled->bottom = box_scaled->bottom - (k[getpic(h)].box.bottom - spr[h].alt.bottom);

      rect_copy(box_real, &spr[h].alt);
    }

  //********* Check to see if they need to be cut down and do clipping

  if (spr[h].size == 0)
    spr[h].size = 100;

  if (dinkedit && (mode == 1 || mode == 5) && draw_screen_tiny < 1)
    goto do_draw;

  if (box_scaled->left < mplayl)
    {
      x_offset = box_scaled->left * (-1) + mplayl;
      box_scaled->left = mplayl;

      if (spr[h].size == 100)
	box_real->left += x_offset;
      else
	box_real->left += (x_offset * 100) / spr[h].size;

      if (box_scaled->right - 1 < mplayl)
	goto nodraw;
    }

  if (box_scaled->top < 0)
    {
      y_offset = box_scaled->top * (-1);
      box_scaled->top = 0;

      if (spr[h].size == 100)
	box_real->top += y_offset;
      else
	box_real->top += (y_offset * 100) / spr[h].size;

      if (box_scaled->bottom-1 < 0)
	goto nodraw;
    }

  if (box_scaled->right > mplayx)
    {
      x_offset = (box_scaled->right) - mplayx;
      box_scaled->right = mplayx;

      if (spr[h].size == 100)
	box_real->right -= x_offset;
      else
	box_real->right -= (x_offset * 100) / spr[h].size;

      if (box_scaled->left+1 > mplayx)
	goto nodraw;
    }

  if (box_scaled->bottom > mplayy)
    {
      y_offset = (box_scaled->bottom) - mplayy;
      box_scaled->bottom = mplayy;

      if (spr[h].size == 100)
	box_real->bottom -= y_offset;
      else
	box_real->bottom -= (y_offset * 100) / spr[h].size;

      if (box_scaled->top+1 > mplayy)
	goto nodraw;
    }

 do_draw:
    return(/*true*/1);

 nodraw:
    return(/*false*/0);
}


/* void reload_sprites(char name[100], int nummy, int junk) */
/* { */
/*         HRESULT     ddrval; */
/*     PALETTEENTRY    holdpal[256];          */

/*         char crap[100],hold[10]; */
/*         int n; */
/*         n = 0;   */

/*         lpDDPal->GetEntries(0,0,256,holdpal);      */
/*         lpDDPal->SetEntries(0,0,256,real_pal); */


/*         for (int oo = index[nummy].s+1; oo <= index[nummy].s + index[nummy].last; oo++) */
/*         { */
/*                 n++; */

                //  Msg( "%s", crap);

                //      initFail(hWndMain, crap);
/*                 ddrval = k[oo].k->Restore(); */
/*         if( ddrval == DD_OK ) */
/*         { */


/*                         if (n < 10) strcpy(hold, "0"); else strcpy(hold,""); */
/*                         sprintf(crap, "%s%s%d.BMP",name,hold,n); */

/*                         DDReLoadBitmap(k[oo].k, crap); */
                        //Msg("Sprite %s%d.bmp reloaded into area %d. ",name,n,oo);


/*         } */
/*         } */
/*         lpDDPal->SetEntries(0,0,256,holdpal);    */
/* } */

/* Editor only */
void check_sprite_status(int h)
{
/*         HRESULT dderror; */
/*         char word1[80]; */
        //is sprite in memory?
        if (spr[h].pseq > 0)
        {
                // Msg("Smartload: Loading seq %d..", spr[h].seq);
                if (seq[spr[h].pseq].frame[1] == 0)
                {
		  if (seq[spr[h].pseq].is_active)
		    figure_out(seq[spr[h].pseq].ini);
		  else
		    log_error("Error: sprite %d on map %d references non-existent sequence %d",
			      h, cur_map, spr[h].pseq);
                }
                else
                {
                        //it's been loaded before.. is it lost or still there?
                        //Msg("Sprite %d's seq is %d",h,spr[h].seq);

/*                         dderror = k[seq[spr[h].pseq].frame[1]].k->IsLost(); */

/*                         if (dderror == DDERR_SURFACELOST) */
/*                         { */
/*                                 get_word(seq[spr[h].pseq].data, 2, word1); */

/*                                 reload_sprites(word1, spr[h].pseq,0); */
/*                                 //Msg("Reloaded seq %d with path of %s should be %s", spr[h].seq, word1,seq[spr[h].seq].data ); */
/*                         } */


                }
        }




}

/* Editor only */
void check_frame_status(int h, int frame)

{
/*         HRESULT dderror; */
/*         char word1[80]; */

        if (!seq[h].is_active) return;

        if (h > 0)
        {
                // Msg("Smartload: Loading seq %d..", spr[h].seq);
                if (seq[h].frame[1] == 0 || GFX_k[seq[h].frame[1]].k == NULL)
                {
                        figure_out(seq[h].ini);
                }
                else
                {
                        //it's been loaded before.. is it lost or still there?
                        //Msg("Sprite %d's seq is %d",h,spr[h].seq);

/*                         dderror = k[seq[h].frame[1]].k->IsLost(); */

/*                         if (dderror == DDERR_SURFACELOST) */
/*                         { */
/*                                 get_word(seq[h].data, 2, word1); */

/*                                 reload_sprites(word1, h,0); */
/*                                 //Msg("Reloaded seq %d with path of %s should be %s", spr[h].seq, word1,seq[spr[h].seq].data ); */
/*                         } */
                }
        }


}

/**
 * Load sequence in memory if not already, using cached dink.ini info
 */
void check_seq_status(int seq_no)
{
  if (seq_no > 0 && seq_no < MAX_SEQUENCES)
    {
      /* Skip empty/unused sequences */
      if (!seq[seq_no].is_active)
	return;

      if (seq[seq_no].frame[1] == 0 || GFX_k[seq[seq_no].frame[1]].k == NULL)
	figure_out(seq[seq_no].ini);
    }
  else if (seq_no > 0)
    {
      log_error("Warning: check_seq_status: invalid sequence %d", seq_no);
    }
}

/**
 * Load all +1->+9 sequences from base sequence 'base' in memory,
 * useful to load all of a moving sprite sequences
 */
void check_base(int base)
{
  int i;
  for (i = 1; i < 10; i++)
    if (seq[base+i].is_active)
      check_seq_status(base+i);
}

/**
 * Checks for all seq's used by the (base) commands
 */
void check_sprite_status_full(int sprite_no)
{
  //is sprite in memory?
  check_seq_status(spr[sprite_no].pseq);

  if (spr[sprite_no].base_walk > -1)
    check_base(spr[sprite_no].base_walk);
}


void get_right(char line[200], char thing[100], char *ret)
        {
                char *dumb;
                int pos = strcspn(line, thing );


                if (pos == 0){ strcpy(ret, ""); return; }


                dumb = &ret[pos+1];
                strcpy(ret, dumb);
        }





void draw_sprite_game(SDL_Surface *GFX_lpdest, int h)
{
  if (spr[h].brain == 8)
    return; // text
  if (spr[h].nodraw == 1)
    return; // invisible

  rect box_crap,box_real;

  if (get_box(h, &box_crap, &box_real))
    {
      /* Generic scaling */
      /* Not perfectly accurate yet: move a 200% sprite to the border
	 of the screen to it is clipped: it's scaled size will slighly
	 vary. Maybe we need to clip the source zone before scaling
	 it.. */
      // error checking for invalid rectangle
      if (box_crap.left >= box_crap.right || box_crap.top >= box_crap.bottom)
	return;
      
      SDL_Rect src, dst;
      int retval = 0;
      src.x = box_real.left;
      src.y = box_real.top;
      src.w = box_real.right - box_real.left;
      src.h = box_real.bottom - box_real.top;
      dst.x = box_crap.left;
      dst.y = box_crap.top;
      dst.w = box_crap.right - box_crap.left;
      dst.h = box_crap.bottom - box_crap.top;

      retval = gfx_blit_stretch(GFX_k[getpic(h)].k, &src, GFX_lpdest, &dst);
      
      if (retval < 0) {
	log_error("Could not draw sprite %d: %s", getpic(h), SDL_GetError());
	/* If we failed, then maybe the sprite was actually loaded
	   yet, let's try now */
	if (spr[h].pseq != 0)
	  check_seq_status(spr[h].pseq);
    }
  }
}


        void changedir( int dir1, int k,int base)
        {
                int hspeed;
                int speed_hold = spr[k].speed;
                if (k > 1) if (spr[k].brain != 9) if (spr[k].brain != 10)
                {
                        hspeed = spr[k].speed * (base_timing / 4);
                        if (hspeed > 49)
                        {
                                log_debug("Speed was %d", hspeed);
                                spr[k].speed = 49;
                        } else
                                spr[k].speed = hspeed;
                }
                int old_seq = spr[k].seq;
                spr[k].dir = dir1;

                if (dir1 == 1)
                {
                        spr[k].mx = (0 - spr[k].speed ) + (spr[k].speed / 3);
                        spr[k].my = spr[k].speed - (spr[k].speed / 3);

                        if (base != -1)
                        {


                                spr[k].seq = base + 1;
                                if (!seq[spr[k].seq].is_active)
                                {
                                        spr[k].seq = base + 9;

                                }

                        }

                        if (old_seq != spr[k].seq)
                        {
                                spr[k].frame = 0;
                                spr[k].delay = 0;
                        }


                }

                if (dir1 == 2)
                {
                        spr[k].mx = 0;
                        spr[k].my = spr[k].speed;
                        if (base != -1)
                                spr[k].seq = base + 2;

                        if (!seq[spr[k].seq].is_active && seq[base+3].is_active)
			  spr[k].seq = base + 3;
                        if (!seq[spr[k].seq].is_active && seq[base+1].is_active)
			  spr[k].seq = base + 1;


                        if (old_seq != spr[k].seq)
                        {
                                spr[k].frame = 0;
                                spr[k].delay = 0;
                        }


                }
                if (dir1 == 3)
                {
                        spr[k].mx = spr[k].speed - (spr[k].speed / 3);
                        spr[k].my = spr[k].speed - (spr[k].speed / 3);
                        if (base != -1)
                        {
                                spr[k].seq = base + 3;
                                if (!seq[spr[k].seq].is_active)
                                        spr[k].seq = base + 7;

                        }

                        if (old_seq != spr[k].seq)
                        {
                                spr[k].frame = 0;
                                spr[k].delay = 0;
                        }


                }

                if (dir1 == 4)
                {

                        //Msg("Changing %d to four..",k);
                        spr[k].mx = (0 - spr[k].speed);
                        spr[k].my = 0;
                        if (base != -1)
                                spr[k].seq = base + 4;
                        if (!seq[spr[k].seq].is_active && seq[base+7].is_active)
			  spr[k].seq = base + 7;
                        if (!seq[spr[k].seq].is_active && seq[base+1].is_active)
			  spr[k].seq = base + 1;
                }

                if (dir1 == 6)
                {
                        spr[k].mx = spr[k].speed;
                        spr[k].my = 0;
                        if (base != -1)
                                spr[k].seq = base + 6;

                        if (!seq[spr[k].seq].is_active && seq[base+3].is_active)
			  spr[k].seq = base + 3;
                        if (!seq[spr[k].seq].is_active && seq[base+9].is_active)
			  spr[k].seq = base + 9;

                }

                if (dir1 == 7)
                {
                        spr[k].mx = (0 - spr[k].speed) + (spr[k].speed / 3);
                        spr[k].my = (0 - spr[k].speed)+ (spr[k].speed / 3);
                        if (base != -1)
                        {
                                spr[k].seq = base + 7;


                                if (!seq[spr[k].seq].is_active)
				  spr[k].seq = base + 3;
                        }

                }
                if (dir1 == 8)
                {
                        spr[k].mx = 0;
                        spr[k].my = (0 - spr[k].speed);
                        if (base != -1)
                                spr[k].seq = base + 8;

                        if (!seq[spr[k].seq].is_active && seq[base+7].is_active)
			  spr[k].seq = base + 7;
                        if (!seq[spr[k].seq].is_active && seq[base+9].is_active)
			  spr[k].seq = base + 9;

                }


                if (dir1 == 9)
                {
                        spr[k].mx = spr[k].speed- (spr[k].speed / 3);
                        spr[k].my = (0 - spr[k].speed)+ (spr[k].speed / 3);
                        if (base != -1)
                        {
                                spr[k].seq = base + 9;
                                if (!seq[spr[k].seq].is_active)
                                        spr[k].seq = base + 1;
                        }
                }



                if (old_seq != spr[k].seq)
                {
                        spr[k].frame = 0;
                        spr[k].delay = 0;
                }


                if (!seq[spr[k].seq].is_active)
                {
                        //spr[k].mx = 0;
                        //spr[k].my = 0;
                        spr[k].seq = old_seq;

                }

                //Msg("Leaving with %d..", spr[k].dir);

                //Msg("Changedir: Tried to switch sprite %d to dir %d",k,dir1);

                spr[k].speed = speed_hold;

}



void show_bmp(char* name, int showdot, int script)
{
  char* fullpath = paths_dmodfile(name);
  SDL_Surface* image = IMG_Load(fullpath);
  if (image == NULL)
    {
      log_error("Couldn't load '%s': %s", name, SDL_GetError());
      return;
    }
  
  /* Set physical screen palette */
  if (!truecolor)
    {
      gfx_palette_set_from_surface(image);
      SDL_Color phys_pal[256];
      gfx_palette_get_phys(phys_pal);

      /* In case the DX bug messed the palette, let's convert the
	 image to the new palette. This also converts 24->8bit if
	 necessary. */
      {
	SDL_Surface* converted = SDL_ConvertSurfaceFormat(image, SDL_PIXELFORMAT_INDEX8, 0);
	SDL_SetPaletteColors(converted->format->palette, phys_pal, 0, 256);
	SDL_BlitSurface(image, NULL, converted, NULL);
	SDL_FreeSurface(image);
	image = converted;
      }

      /* Next blit without palette conversion */
      SDL_SetPaletteColors(image->format->palette, GFX_real_pal, 0, 256);
    }

  showb.active = /*true*/1;
  showb.showdot = showdot;
  showb.script = script;

  SDL_BlitSurface(image, NULL, GFX_lpDDSTrick, NULL);
  SDL_FreeSurface(image);

  // After show_bmp(), and before the flip_it() call in updateFrame(),
  // other parts of the code will draw sprites on lpDDSBack and mess
  // the showbmp(). So skip the next flip_it().
  abort_this_flip = /*true*/1;
}


/* Used to implement DinkC's copy_bmp_to_screen(). Difference with
   show_cmp: does not set showb.* (wait for button), install the image
   to lpDDSTwo (background) and not lpDDSBack (screen double
   buffer) */
void copy_bmp(char* name)
{
  char* fullpath = paths_dmodfile(name);
  SDL_Surface* image = IMG_Load(fullpath);
  if (image == NULL)
    {
      log_error("Couldn't load '%s': %s", name, SDL_GetError());
      return;
    }
  
  /* Set physical screen palette */
  if (!truecolor)
    {
      gfx_palette_set_from_surface(image);
      /* Grab back palette with DX bug overwrite */
      SDL_Color phys_pal[256];
      gfx_palette_get_phys(phys_pal);

      /* In case the DX bug messed the palette, let's convert the
	 image to the new palette. This also converts 24->8bit if
	 necessary. */
      {
	SDL_Surface* converted =  SDL_CreateRGBSurface(0, image->w,image->h,
						       8, 0,0,0,0);
	SDL_SetPaletteColors(converted->format->palette, phys_pal, 0, 256);
	SDL_BlitSurface(image, NULL, converted, NULL);
	SDL_FreeSurface(image);
	image = converted;
      }

      /* Next blit without palette conversion */
      SDL_SetPaletteColors(image->format->palette, GFX_real_pal, 0, 256);
    }

  SDL_BlitSurface(image, NULL, GFX_lpDDSTwo, NULL);
  SDL_FreeSurface(image);

  abort_this_flip = /*true*/1;
}

        int hurt_thing(int h, int damage, int special)
        {
                //lets hurt this sprite but good
                if (damage < 1) return(0);
                int num = damage - spr[h].defense;

                //      Msg("num is %d.. defense was %d.of sprite %d", num, spr[h].defense, h);
                if (num < 1) num = 0;

                if (num == 0)
                {
                        if ((rand() % 2)+1 == 1) num = 1;
                }

                spr[h].damage += num;
                return(num);
                //draw blood here
        }

        void random_blood(int mx, int my, int sprite)
        {
                int myseq;
                /* v1.08 introduces custom blood sequence, as well as
                   a slightly different default (select blood in range
                   187-189 included, instead of 187-188 included) */
                int randy;
                if (spr[sprite].bloodseq > 0 && spr[sprite].bloodnum > 0)
                  {
                    myseq = spr[sprite].bloodseq;
                    randy = spr[sprite].bloodnum;
                  }
                else
                  {
                    myseq = 187;
                    if (dversion >= 108)
                      randy = 3;
                    else
                      randy = 2;
                  }
                myseq += (rand () % randy);
                
                int crap2 = add_sprite(mx,my,5,myseq,1);
                /* TODO: add_sprite might return 0, and the following
                   would trash spr[0] - cf. bugs.debian.org/688934 */
                spr[crap2].speed = 0;
                spr[crap2].base_walk = -1;
                spr[crap2].nohit = 1;
                spr[crap2].seq = myseq;
                if (sprite > 0)
                        spr[crap2].que = spr[sprite].y+1;

        }


void fill_screen(int num)
{
  /* Warning: palette indexes 0 and 255 are hard-coded
     to black and white (cf. gfx_palette.c). */
  if (!truecolor)
    SDL_FillRect(GFX_lpDDSTwo, NULL, num);
  else
    SDL_FillRect(GFX_lpDDSTwo, NULL, SDL_MapRGB(GFX_lpDDSTwo->format,
						GFX_real_pal[num].r,
						GFX_real_pal[num].g,
						GFX_real_pal[num].b));
}


void set_mode(int new_mode) {
  mode = new_mode;
  if (mode == 3 && !keep_mouse) {
    SDL_SetWindowGrab(window, SDL_FALSE);
  } else {
    /* Jail window cursor (Alt+Tab still works) */
    SDL_SetWindowGrab(window, SDL_TRUE);
  }
}

void set_keep_mouse(int on) {
  keep_mouse = on;
  if (!on)
    SDL_SetWindowGrab(window, SDL_FALSE);
}
