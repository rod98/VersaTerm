// -----------------------------------------------------------------------------
// VersaTerm - A versatile serial terminal
// Copyright (C) 2022 David Hansel
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
// -----------------------------------------------------------------------------

#include "vt52.h"
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

#include "../internal/internal.h"
#include "../terminal.h"

#define INFLASHFUN __in_flash(".terminalfun") 

extern global_state glob_st;
static global_state *gs = &glob_st;

void INFLASHFUN terminal_receive_char_vt52(char c)
{
  static char start_char, row;

  switch( gs->terminal_state )
    {
    case TS_NORMAL:
      {
        if( c==27 )
          gs->terminal_state = TS_STARTCHAR;
        else
          terminal_process_text(c);
        
        break;
      }

    case TS_STARTCHAR:
      {
        gs->terminal_state = TS_NORMAL;

        switch( c )
          {
          case 'A': 
            move_cursor_limited(gs->cursor_row-1, gs->cursor_col);
            break;

          case 'B': 
            move_cursor_limited(gs->cursor_row+1, gs->cursor_col);
            break;

          case 'C': 
            move_cursor_limited(gs->cursor_row, gs->cursor_col+1);
            break;

          case 'D': 
            move_cursor_limited(gs->cursor_row, gs->cursor_col-1);
            break;

          case 'E':
            framebuf_fill_screen(' ', gs->color_fg, gs->color_bg);
            // fall through

          case 'H': 
            move_cursor_limited(0, 0);
            break;

          case 'I': 
            move_cursor_wrap(gs->cursor_row-1, gs->cursor_col);
            break;

          case 'J':
            show_cursor(false);
            framebuf_fill_region(gs->cursor_col, gs->cursor_row, framebuf_get_ncols(gs->cursor_row)-1, framebuf_get_nrows()-1, ' ', gs->color_fg, gs->color_bg);
            gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
            show_cursor(gs->cursor_shown);
            break;

          case 'K':
            show_cursor(false);
            framebuf_fill_region(gs->cursor_col, gs->cursor_row, framebuf_get_ncols(gs->cursor_row)-1, gs->cursor_row, ' ', gs->color_fg, gs->color_bg);
            gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
            show_cursor(gs->cursor_shown);
            break;

          case 'L':
          case 'M':
            show_cursor(false);
            framebuf_scroll_region(gs->cursor_row, framebuf_get_nrows()-1, c=='M' ? 1 : -1, gs->color_fg, gs->color_bg);
            gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
            show_cursor(gs->cursor_shown);
            break;

          case 'Y':
            start_char = c;
            row = 0;
            gs->terminal_state = TS_READPARAM;
            break;
            
          case 'Z':
            send_string("\033/K");
            break;

          case 'b':
          case 'c':
            start_char = c;
            gs->terminal_state = TS_READPARAM;
            break;

          case 'd':
            framebuf_fill_region(0, 0, gs->cursor_col, gs->cursor_row, ' ', gs->color_fg, gs->color_bg);
            init_cursor(gs->cursor_col, gs->cursor_row);
            break;
            
          case 'e':
            show_cursor(true);
            break;

          case 'f':
            show_cursor(false);
            break;

          case 'j':
            gs->saved_col = gs->cursor_col;
            gs->saved_row = gs->cursor_row;
            break;

          case 'k':
            move_cursor_limited(gs->saved_row, gs->saved_col);
            break;

          case 'l':
            framebuf_fill_region(0, gs->cursor_row, framebuf_get_ncols(gs->cursor_row)-1, gs->cursor_row, ' ', gs->color_fg, gs->color_bg);
            init_cursor(0, gs->cursor_row);
            break;

          case 'o':
            framebuf_fill_region(0, gs->cursor_row, gs->cursor_col, gs->cursor_row, ' ', gs->color_fg, gs->color_bg);
            show_cursor(gs->cursor_shown);
            break;

          case 'p':
            framebuf_set_screen_inverted(true);
            break;

          case 'q':
            framebuf_set_screen_inverted(false);
            break;

          case 'v':
            gs->auto_wrap_mode = true;
            break;

          case 'w':
            gs->auto_wrap_mode = false;
            break;

          case '<':
            terminal_reset();
            gs->vt52_mode = false;
            break;
          }

        break;
      }

    case TS_READPARAM:
      {
        if( start_char=='Y' )
          {
            if( row==0 )
              row = c;
            else
              {
                if( row>=32 && c>=32 ) move_cursor_limited(row-32, c-32);
                gs->terminal_state = TS_NORMAL;
              }
          }
        else if( start_char=='b' && c>=32 )
          {
            gs->color_fg = (c-32) & 15;
            show_cursor(gs->cursor_shown);
          }
        else if( start_char=='c' && c>=32 )
          {
            gs->color_bg = (c-32) & 15;
            show_cursor(gs->cursor_shown);
          }

        break;
      }
    }
}