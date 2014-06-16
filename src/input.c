/**
 * Keyboard and joystick

 * Copyright (C) 1997, 1998, 1999, 2002, 2003  Seth A. Robinson
 * Copyright (C) 2005, 2007, 2008, 2009, 2010, 2014  Sylvain Beucler

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

#include <string.h>

#include "SDL.h"
#include "game_engine.h"
#include "log.h"
#include "input.h"

/* Input state */
struct seth_joy sjoy;

/* maps joystick buttons to action IDs (attack/attack/map/...). */
/* 10 buttons (indices), 6 different actions + 4 static buttons (values) */
static enum buttons_actions buttons_map[NB_BUTTONS];



/* State of the keyboard, SDL-supported keys */
/* current keyboard state */
Uint8 scancodestate[SDL_NUM_SCANCODES];
/* true if key was just pressed, false if kept pressed or released */
int scancodejustpressed[SDL_NUM_SCANCODES];

/* Mouse left click */
/*bool*/int mouse1 = 0;
  

/* Access keyboard cached state */
Uint8 input_getscancodestate(SDL_Scancode scancode)
{
  //return SDL_GetKeyboardState(NULL)[scancode];
  return scancodestate[scancode];
}
/*bool*/int input_getscancodejustpressed(SDL_Scancode scancode)
{
  return scancodejustpressed[scancode];
}

/* Mark keys meant to be layout-dependent */
/* Using SDL_Keycode should work, if not let's switch to TextInput */
Uint8 input_getcharstate(SDL_Keycode ch)
{
  //return SDL_GetKeyboardState(NULL)[SDL_GetScancodeFromKey(ch)];
  return scancodestate[SDL_GetScancodeFromKey(ch)];
}
/*bool*/int input_getcharjustpressed(SDL_Keycode ch)
{
  return scancodejustpressed[SDL_GetScancodeFromKey(ch)];
}


void input_init(void)
{
  /* Mouse: keep it within the window and get relative motion events
     ((+x,+y) rather than (x,y)) */

  // Use mouse warping rather than raw input - raw input has a
  // different resolution and acceleration under (at least) X11.
  // It also keeps the mouse within the window in software mode.
  SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1");

  int ret = SDL_SetRelativeMouseMode(SDL_TRUE);
  if (ret == -1)
    log_error("Relative mouse positionning not supported on this platform.");
  /* TODO SDL2: free mouse when the game is not using it */

  /* We'll handle those events manually */
  SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
  SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
  /* Only track button down events */
  SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
  /* We'll enable text inputs on demande (dinkc_console, editor textbox) */
  SDL_EventState(SDL_TEXTINPUT, SDL_IGNORE);
  /* We still process through a SDL_PollEvent() loop: */
  /* - SDL_QUIT: quit on window close and Ctrl+C */
  /* - SDL_MOUSEMOTION: easier for SDL_SetRelativeMouseMode() */
  /* - SDL_MOUSEBUTTONDOWN: don't miss quick clicks */
  /* - SDL_KEYUP/SDL_KEYDOWN: process by event instead of querying the
       full keyboard state, so we can easily ignore some events
       (e.g. Ctrl+Enter when going fullscreen, Escape when exiting the
       console, etc.) */
  /* - Joystick: apparently we need to keep them, otherwise joystick
       doesn't work at all */

  /* Clear keyboard/joystick buffer */
  memset(&sjoy,0,sizeof(sjoy));
  {
    int a = ACTION_FIRST;
    for (a = ACTION_FIRST; a < ACTION_LAST; a++) 
      sjoy.joybitold[a] = 0;
  }
  sjoy.rightold = 0;
  sjoy.leftold  = 0;
  sjoy.upold    = 0;
  sjoy.downold  = 0;

  /* Define default button->action mapping */
  input_set_default_buttons();

  /* JOY */
  /* Joystick initialization never makes Dink fail for now. */
  /* Note: joystick is originaly only used by the game, not the
     editor. */
  if (joystick == 1)
    {
      if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == -1)
	{
	  log_error("Error initializing joystick, skipping: %s", SDL_GetError());
	  joystick = 0;
	}
      else
	{
	  /* first tests if a joystick driver is present */
	  /* if TRUE it makes certain that a joystick is plugged in */
	  log_info("%i joystick(s) were found.", SDL_NumJoysticks());
	  if (SDL_NumJoysticks() > 0)
	    {
	      int i;
	      log_info("The names of the joysticks are:");
	      for (i=0; i < SDL_NumJoysticks(); i++) {
		SDL_Joystick* jinfo = SDL_JoystickOpen(i);
		SDL_JoystickGUID jguid = SDL_JoystickGetGUID(jinfo);
		char guid_str[200];
		SDL_JoystickGetGUIDString(jguid, guid_str, 200);
		log_info("    %s [guid=%s] [compat=%d]",
			 SDL_JoystickName(jinfo), guid_str,
			 SDL_IsGameController(i));
	      }

	      log_info("Picking the first one...");
	      // TODO: make joystick # configurable

	      jinfo = SDL_JoystickOpen(0);
	      /* Don't activate joystick events, Dink polls joystick
		 manually.  Plus events would pile up in the queue. */
	      SDL_JoystickEventState(SDL_IGNORE);
	      
	      if (jinfo) {
		log_info("Name: %s", SDL_JoystickName(jinfo));
		log_info("Number of axes: %d", SDL_JoystickNumAxes(jinfo));
		log_info("Number of buttons: %d", SDL_JoystickNumButtons(jinfo));
		log_info("Number of balls: %d", SDL_JoystickNumBalls(jinfo));
		log_info("Number of hats: %d", SDL_JoystickNumHats(jinfo));
		
		/* Flush stacked joystick events */
		{
		  SDL_Event event;
		  while (SDL_PollEvent(&event));
		}
		
		joystick = 1;
	      } else {
		log_error("Couldn't open Joystick #0");
		joystick = 0;
	      }
	    }
	}
    }
}

void input_quit(void)
{
  if (joystick == 1)
    {
      if (jinfo != NULL)
	{
	  SDL_JoystickClose(jinfo);
	  jinfo = NULL;
	}
      SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    }
}

void input_set_default_buttons(void)
{
  /* Set default button->action mapping */
  int i = 0;
  for (i = 0; i < NB_BUTTONS; i++)
    input_set_button_action(i, ACTION_NOOP);

  input_set_button_action( 1-1, ACTION_ATTACK);
  input_set_button_action( 2-1, ACTION_TALK);
  input_set_button_action( 3-1, ACTION_MAGIC);
  input_set_button_action( 4-1, ACTION_INVENTORY);
  input_set_button_action( 5-1, ACTION_MENU);
  input_set_button_action( 6-1, ACTION_MAP);
  input_set_button_action( 7-1, ACTION_BUTTON7);
  input_set_button_action( 8-1, ACTION_BUTTON8);
  input_set_button_action( 9-1, ACTION_BUTTON9);
  input_set_button_action(10-1, ACTION_BUTTON10);

#ifdef _PSP
  /* Alternate mapping, more consistent with other apps on PSP; in
     addition, the buttons numbering/placement is different than on
     PC, so it needs to be redefined anyway. */
  /* Here are names for the button indices returned by SDL, 12 buttons
     in [0,11]; home/hold/note/screen/vol+/vol- can't be used */
  enum buttons_psp {
    BUTTON_TRIANGLE=0, BUTTON_CIRCLE, BUTTON_CROSS, BUTTON_SQUARE,
    BUTTON_LTRIGGER, BUTTON_RTRIGGER,
    BUTTON_DOWN, BUTTON_LEFT, BUTTON_UP, BUTTON_RIGHT,
    BUTTON_SELECT, BUTTON_START, BUTTON_HOME, BUTTON_HOLD };
  input_set_button_action(BUTTON_TRIANGLE,  ACTION_INVENTORY);
  input_set_button_action(BUTTON_CIRCLE,    ACTION_MAGIC);
  input_set_button_action(BUTTON_CROSS,     ACTION_ATTACK);
  input_set_button_action(BUTTON_SQUARE,    ACTION_TALK);
  input_set_button_action(BUTTON_LTRIGGER,  ACTION_MENU);
  input_set_button_action(BUTTON_RTRIGGER,  ACTION_MAP);
  // TODO: make these also work like d/l/u/r:
  input_set_button_action(BUTTON_DOWN,      ACTION_DOWN);
  input_set_button_action(BUTTON_LEFT,      ACTION_LEFT);
  input_set_button_action(BUTTON_UP,        ACTION_UP);
  input_set_button_action(BUTTON_RIGHT,     ACTION_RIGHT);
  /* TODO: we could also map:
     - debug (Alt+D),
     - pause/resume midi (Alt+N/B)
     - fast-quit (Alt+Q) - fast-quit is somewhat already available
     through the classic Home key, although handled differently. */
  /* Let's also try to get a free key to possibly implement a
     zooming/switch-view function for small screens, as well as a
     virtual keyboard feature (like ScummVM)... */
  /* Maybe also map inventory to start instead of Triangle. */
  input_set_button_action(BUTTON_START,     ACTION_INVENTORY);
#endif
}

enum buttons_actions input_get_button_action(int button_index)
{
  if (button_index >= 0 && button_index < NB_BUTTONS)
    {
      return buttons_map[button_index];
    }
  return -1; /* error */
}

/**
 * Set what action will be triggered when button 'button_index' is
 * pressed. Action '0' currently means 'do nothing'.
 */
void input_set_button_action(int button_index, enum buttons_actions action_index)
{
  if (button_index >= 0 && button_index < NB_BUTTONS)
    {
      if (action_index >= ACTION_FIRST && action_index < ACTION_LAST)
	buttons_map[button_index] = action_index;
      else
	log_error("Attempted to set invalid action %d", action_index);
    }
  else
    {
      log_error("Attempted to set invalid button %d (internal index %d)",
		button_index+1, button_index);
    }
}


void input_reset_justpressed() {
  for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
    scancodejustpressed[i] = 0;
  }
}

void input_update_keyboard(SDL_Event *ev) {
  if (!(ev->type == SDL_KEYDOWN || ev->type == SDL_KEYUP))
    return;
  if (ev->key.repeat)
    return;

  int scancode = ev->key.keysym.scancode;
  scancodestate[scancode] = ev->key.state;

  if (ev->key.state == SDL_PRESSED)
    /* We just changed from "released" to "pressed" */
    scancodejustpressed[scancode] = 1;
  else
    scancodejustpressed[scancode] = 0;
}

void input_reset_mouse() {
  mouse1 = /*false*/0;
}

void input_update_mouse(SDL_Event *ev) {
  if (ev->type != SDL_MOUSEBUTTONDOWN)
    return;

  if (ev->button.button == SDL_BUTTON_LEFT)
    mouse1 = /*true*/1;
}

/**
 * Reset input before a new game frame
 */
void input_reset() {
  input_reset_justpressed();
  input_reset_mouse();
}

/**
 * Generic for keyboard / mouse input handling.  Stores current state
 * and triggered ("was just pressed") events.
 */
void input_update(SDL_Event *ev) {
  input_update_keyboard(ev);
  input_update_mouse(ev);
}
