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

#define INFLASHFUN __in_flash(".terminalfun") 

void INFLASHFUN terminal_receive_char_vt102(global_state *gs, char c)
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
          internal_terminal_process_text(gs, c);
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
          internal_terminal_process_text(gs, c);

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
            
          case  27: print_char_vt(gs, c); break;                           // escaped ESC
          case 'c': internal_terminal_reset(gs); break;                           // reset
          case '7': internal_terminal_process_command(gs, 0, 's', 0, NULL); break;  // save cursor position
          case '8': internal_terminal_process_command(gs, 0, 'u', 0, NULL); break;  // restore cursor position
          case 'H': gs->tabs[gs->cursor_col] = true; break;                    // set tab
          case 'J': internal_terminal_process_command(gs, 0, 'J', 0, NULL); break;  // clear to end of screen
          case 'K': internal_terminal_process_command(gs, 0, 'K', 0, NULL); break;  // clear to end of row
          case 'D': move_cursor_wrap(gs, gs->cursor_row+1, gs->cursor_col); break; // cursor down
          case 'E': move_cursor_wrap(gs, gs->cursor_row+1, 0); break;          // cursor down and to first column
          case 'I': move_cursor_wrap(gs, gs->cursor_row-1, 0); break;          // cursor up and to furst column
          case 'M': move_cursor_wrap(gs, gs->cursor_row-1, gs->cursor_col); break; // cursor up
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
            internal_terminal_process_command(gs, start_char, c, num_params, params);
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
              show_cursor(gs, false);
              framebuf_fill_region(0, top_limit, framebuf_get_ncols(-1)-1, bottom_limit, 'E', gs->color_fg, gs->color_bg);
              gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
              show_cursor(gs, gs->cursor_shown);
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
