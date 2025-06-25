#include "terminal.h"
#include "internal/internal.h"

#include "vt102/vt102.h"

#include "framebuf.h"
#include "font.h"
#include "config.h"
#include "pins.h"
#include "serial.h"
#include "sound.h"
#include "keyboard.h"
#include "hardware/uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INFLASHFUN __in_flash(".terminalfun") 

extern global_state glob_st;

global_state *gs = &glob_st;

void INFLASHFUN terminal_receive_char(char c)
{
  if( config_get_terminal_clearBit7() ) c &= 0x7f;

  switch( config_get_terminal_type() )
    {
    case CFG_TTYPE_VT102:
      if( !vt52_mode ) { terminal_receive_char_vt102(gs, c); break; }

    // case CFG_TTYPE_VT52:
    //   terminal_receive_char_vt52(gs, c);
    //   break;

    // case CFG_TTYPE_PETSCII:
    //   terminal_receive_char_petscii(gs, c);
    //   break;
    }
}



void INFLASHFUN terminal_receive_string(const char* str)
{
  while( *str ) { terminal_receive_char(gs, *str); str++; }
}

void INFLASHFUN terminal_process_key(uint16_t key);

void INFLASHFUN terminal_clear_screen()
{
  framebuf_fill_screen(' ', gs->color_fg, gs->color_bg);
  init_cursor(gs, 0, 0);
  scroll_region_start = 0;
  scroll_region_end = framebuf_get_nrows()-1;
  origin_mode = false;
}

void INFLASHFUN terminal_init() {
    internal_terminal_init(gs);
}

void INFLASHFUN terminal_apply_settings() {
    internal_terminal_init(gs);
}

void INFLASHFUN terminal_reset() {
    internal_terminal_reset(gs);
}