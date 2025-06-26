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

#include "vt102.h"
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

void INFLASHFUN terminal_receive_char_vt102(char c)
{
  static char    start_char = 0;
  static uint8_t num_params = 0;
  static uint8_t params[16];

  if( gs->terminal_state!=TS_NORMAL )
    {
      if( c==8 || c==10 || c==13 )
        {
          // processe some cursor control characters within escape sequences
          // (otherwise we fail "vttest" cursor control tests)
          terminal_process_text(c);
          return;
        }
      else if( c==11 )
        {
          // ignore VT character plus the following character
          // (otherwise we fail "vttest" cursor control tests)
          gs->terminal_state = TS_READCHAR;
          return;
        }
    }

  switch( gs->terminal_state )
    {
    case TS_NORMAL:
      {
        if( c==27 )
          gs->terminal_state = TS_WAITBRACKET;
        else
          terminal_process_text(c);

        break;
      }

    case TS_WAITBRACKET:
      {
        gs->terminal_state = TS_NORMAL;

        switch( c )
          {
          case '[':
            start_char = 0;
            num_params = 1;
            params[0] = 0;
            gs->terminal_state = TS_STARTCHAR;
            break;
            
          case '#':
            gs->terminal_state = TS_HASH;
            break;
            
          case  27: print_char_vt(c); break;                           // escaped ESC
          case 'c': internal_terminal_reset(gs); break;                           // reset
          case '7': terminal_process_command(0, 's', 0, NULL); break;  // save cursor position
          case '8': terminal_process_command(0, 'u', 0, NULL); break;  // restore cursor position
          case 'H': gs->tabs[gs->cursor_col] = true; break;                    // set tab
          case 'J': terminal_process_command(0, 'J', 0, NULL); break;  // clear to end of screen
          case 'K': terminal_process_command(0, 'K', 0, NULL); break;  // clear to end of row
          case 'D': move_cursor_wrap(gs->cursor_row+1, gs->cursor_col); break; // cursor down
          case 'E': move_cursor_wrap(gs->cursor_row+1, 0); break;          // cursor down and to first column
          case 'I': move_cursor_wrap(gs->cursor_row-1, 0); break;          // cursor up and to furst column
          case 'M': move_cursor_wrap(gs->cursor_row-1, gs->cursor_col); break; // cursor up
          case '(': 
          case ')': 
          case '+':
          case '*':
            start_char = c;
            gs->terminal_state = TS_READCHAR;
            break;
          }

        break;
      }

    case TS_STARTCHAR:
    case TS_READPARAM:
      {
        if( c>='0' && c<='9' )
          {
            params[num_params-1] = params[num_params-1]*10 + (c-'0');
            gs->terminal_state = TS_READPARAM;
          }
        else if( c == ';' )
          {
            // next parameter (max 16 parameters)
            num_params++;
            if( num_params>16 )
              gs->terminal_state = TS_NORMAL;
            else
              {
                params[num_params-1]=0;
                gs->terminal_state = TS_READPARAM;
              }
          }
        else if( gs->terminal_state==TS_STARTCHAR && (c=='?' || c=='#') )
          {
            start_char = c;
            gs->terminal_state = TS_READPARAM;
          }
        else
          {
            // not a parameter value or startchar => command is done
            terminal_process_command(start_char, c, num_params, params);
            gs->terminal_state = TS_NORMAL;
          }
        
        break;
      }

    case TS_HASH:
      {
        switch( c )
          {
          case '3':
            {
              framebuf_set_row_attr(gs->cursor_row, ROW_ATTR_DBL_WIDTH | ROW_ATTR_DBL_HEIGHT_TOP);
              break;
            }

          case '4':
            {
              framebuf_set_row_attr(gs->cursor_row, ROW_ATTR_DBL_WIDTH | ROW_ATTR_DBL_HEIGHT_BOT);
              break;
            }
            
          case '5':
            {
              framebuf_set_row_attr(gs->cursor_row, 0);
              break;
            }

          case '6':
            {
              framebuf_set_row_attr(gs->cursor_row, ROW_ATTR_DBL_WIDTH);
              break;
            }

          case '8': 
            {
              // fill screen with 'E' characters (DEC test feature)
              int top_limit    = gs->origin_mode ? gs->scroll_region_start : 0;
              int bottom_limit = gs->origin_mode ? gs->scroll_region_end   : framebuf_get_nrows()-1;
              show_cursor(false);
              framebuf_fill_region(0, top_limit, framebuf_get_ncols(-1)-1, bottom_limit, 'E', gs->color_fg, gs->color_bg);
              gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
              show_cursor(gs->cursor_shown);
              break;
            }
          }
        
        gs->terminal_state = TS_NORMAL;
        break;
      }

    case TS_READCHAR:
      {
        if( start_char=='(' )
          gs->charset_G0 = get_charset(c);
        else if( start_char==')' )
          gs->charset_G1 = get_charset(c);

        gs->terminal_state = TS_NORMAL;
        break;
      }
    }
}

void INFLASHFUN terminal_process_key_vt(uint16_t key)
{
  bool isaltcode;
  uint8_t c = keyboard_map_key_ascii(key, &isaltcode);
  switch( c )
    {
    //     LALT + 000 = \0;
    // But LALT + less-than-three-numbers = silence
    // Prevents sending extra nulls when entering codes
    case 0:
      if (isaltcode)
        send_char(0x00);
      break;

    case KEY_UP:     send_cursor_sequence('A'); break;
    case KEY_DOWN:   send_cursor_sequence('B'); break;
    case KEY_RIGHT:  send_cursor_sequence('C'); break;
    case KEY_LEFT:   send_cursor_sequence('D'); break;

    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
      {
        send_char(27);
        if( config_get_terminal_type()==CFG_TTYPE_VT102 && !gs->vt52_mode ) send_char('O');
        send_char('P' + (c-KEY_F1));
        break;
      }

    case KEY_ENTER:
      {
        switch( config_get_keyboard_enter() )
          {
          case 0: send_char(0x0d); break;
          case 1: send_char(0x0a); break;
          case 2: send_char(0x0d); send_char(0x0a); break;
          case 3: send_char(0x0a); send_char(0x0d); break;
          }
        break;
      }

    case KEY_BACKSPACE:
      {
        switch( config_get_keyboard_backspace() )
          {
          case 0: send_char(0x08); break;
          case 1: send_char(0x7f); break;
          case 2: send_char(0x5f); break;
          }
        break;
      }

    case KEY_DELETE:
      {
        switch( config_get_keyboard_delete() )
          {
          case 0: send_char(0x08); break;
          case 1: send_char(0x7f); break;
          case 2: send_char(0x5f); break;
          }
        break;
      }

    case KEY_INSERT:
      {
        terminal_receive_string(gs->insert_mode ? "\033[4l" : "\033[4h");
        break;
      }

    case KEY_HOME:
      {
        terminal_receive_string(keyboard_shift_pressed(key) ? "\033[2J\033[H" : "\033[H");
        break;
      }

    default:  
      if( config_get_terminal_uppercase() && isalpha(c) ) c = toupper(c);
      send_char(c);
      break;
    }
}
