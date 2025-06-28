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

#include "internal.h"
#include "../terminal.h"
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

// global_state create_global_state(void) {
//     global_state term_state = 
//     return term_state;
// }

global_state glob_st = (global_state) {
    .terminal_state = TS_NORMAL,
    .attr = 0,
    .cur_attr = 0,
    .cursor_col = 0,
    .cursor_row = 0,
    .saved_col = 0, 
    .saved_row = 0,
    .cursor_shown = true,
    .origin_mode = false, 
    .cursor_eol = false, 
    .auto_wrap_mode = true, 
    .vt52_mode = false, 
    .localecho = false,
    .saved_eol = false, 
    .saved_origin_mode = false, 
    .insert_mode = false,
    .petscii_lower_case_charset = true
};
static global_state *gs = &glob_st;


void internal_terminal_clear_screen() {
  framebuf_fill_screen(' ', gs->color_fg, gs->color_bg);
  init_cursor(0, 0);
  gs->scroll_region_start = 0;
  gs->scroll_region_end = framebuf_get_nrows()-1;
  gs->origin_mode = false;
}

void INFLASHFUN internal_terminal_init() {
  internal_terminal_reset();
  internal_terminal_clear_screen();

  for (int i = 0; i < framebuf_get_ncols(-1); i += 4)
    gs->tabs[i] = 1;
}

void INFLASHFUN send_char(char c)
{
  serial_send_char(c);
  if( gs->localecho ) terminal_receive_char(c);
}

void INFLASHFUN send_string(const char *s)
{
  serial_send_string(s);
  if( gs->localecho ) terminal_receive_string(s);
}

void INFLASHFUN send_cursor_sequence(char c)
{
  if( config_get_terminal_type()==CFG_TTYPE_VT52 || gs->vt52_mode )
    { send_char(27); send_char(c); }
  else
    { send_char(27); send_char('['); send_char(c); }
}


void INFLASHFUN show_cursor(bool show)
{
  uint8_t attr = ATTR_INVERSE;
  switch( config_get_terminal_cursortype() )
    {
    case 1: attr = ATTR_BLINK; break;
    case 2: attr = ATTR_UNDERLINE; break;
    }
  
  framebuf_set_attr(gs->cursor_col, gs->cursor_row, show ? (gs->cur_attr ^ attr) : gs->cur_attr);
}


void INFLASHFUN move_cursor_wrap(int row, int col)
{
  if( row!=gs->cursor_row || col!=gs->cursor_col )
    {
      int top_limit    = gs->scroll_region_start;
      int bottom_limit = gs->scroll_region_end;
      
      if( gs->cursor_shown && gs->cursor_row>=0 && gs->cursor_col>=0 ) show_cursor(false);
      
      while( col<0 )                        { col += framebuf_get_ncols(row); row--; }
      while( row<top_limit )                { row++; framebuf_scroll_region(top_limit, bottom_limit, -1, gs->color_fg, gs->color_bg); }
      while( col>=framebuf_get_ncols(row) ) { col -= framebuf_get_ncols(row); row++; }
      while( row>bottom_limit )             { row--; framebuf_scroll_region(top_limit, bottom_limit, 1, gs->color_fg, gs->color_bg); }

      gs->cursor_row = row;
      gs->cursor_col = col;
      gs->cursor_eol = false;
      
      gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
      if( gs->cursor_shown ) show_cursor(true);
    }
}

void INFLASHFUN internal_terminal_reset()
{
  gs->saved_col = 0;
  gs->saved_row = 0;
  gs->cursor_shown = true;
  gs->color_fg = config_get_terminal_default_fg();
  gs->color_bg = config_get_terminal_default_bg();
  gs->scroll_region_start = 0;
  gs->scroll_region_end = framebuf_get_nrows()-1;
  gs->origin_mode = false;
  gs->cursor_eol = false;
  gs->auto_wrap_mode = true;
  gs->insert_mode = false;
  gs->vt52_mode = false;
  gs->attr = config_get_terminal_default_attr();
  gs->saved_attr = 0;
  gs->charset_G0 = CS_TEXT_US;
  gs->charset_G1 = CS_GRAPHICS;
  gs->saved_charset_G0 = CS_TEXT_US;
  gs->saved_charset_G1 = CS_GRAPHICS;
  gs->charset = &(gs->charset_G0);
  memset(gs->tabs, 0, framebuf_get_ncols(-1));
  framebuf_set_scroll_delay(0);
  gs->localecho = config_get_terminal_localecho();
  gs->petscii_lower_case_charset = true;
}

void INFLASHFUN print_char_vt(char c)
{
  if( gs->cursor_eol ) 
    { 
      // cursor was already past the end of the line => move it to the next line now
      move_cursor_wrap(gs->cursor_row+1, 0); 
      gs->cursor_eol=false; 
    }

  if( gs->insert_mode )
    {
      show_cursor(false);
      framebuf_insert(gs->cursor_col, gs->cursor_row, 1, gs->color_fg, gs->color_bg);
    }

  if( *gs->charset==CS_TEXT_UK && c==35 )
    c=font_map_graphics_char(125, (gs->attr & ATTR_BOLD)!=0); // pound sterling symbol
  else if( *gs->charset==CS_GRAPHICS )
    c=font_map_graphics_char(c, (gs->attr & ATTR_BOLD)!=0);
  
  framebuf_set_color(gs->cursor_col, gs->cursor_row, gs->color_fg, gs->color_bg);
  framebuf_set_attr (gs->cursor_col, gs->cursor_row, gs->attr);
  framebuf_set_char (gs->cursor_col, gs->cursor_row, c);

  if( gs->auto_wrap_mode && gs->cursor_col==framebuf_get_ncols(gs->cursor_row)-1 )
    {
      // cursor stays in last column but will wrap if another character is typed
      gs->cur_attr = gs->attr;
      show_cursor(gs->cursor_shown);
      gs->cursor_eol=true;
    }
  else
    init_cursor(gs->cursor_row, gs->cursor_col+1);
}

uint8_t INFLASHFUN get_charset(char c)
{
  switch( c )
    {
    case 'A' : return CS_TEXT_UK;
    case 'B' : return CS_TEXT_US;
    case '0' : return CS_GRAPHICS;
    case '1' : return CS_TEXT_US;
    case '2' : return CS_GRAPHICS;
    }

  return CS_TEXT_US;
}



static void INFLASHFUN move_cursor_within_region(int row, int col, int top_limit, int bottom_limit)
{
  if( row!=gs->cursor_row || col!=gs->cursor_col )
    {
      if( gs->cursor_shown && gs->cursor_row>=0 && gs->cursor_col>=0 ) show_cursor(false);

      if( col<0 ) 
        col = 0;
      else if( col>=framebuf_get_ncols(row) )
        col = framebuf_get_ncols(row)-1;

      if( row<top_limit ) 
        row = top_limit;
      else if( row>bottom_limit )
        row = bottom_limit;
          
      gs->cursor_row = row;
      gs->cursor_col = col;
      gs->cursor_eol = false;

      gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
      if( gs->cursor_shown ) show_cursor(true);
    }
}

void INFLASHFUN init_cursor(int row, int col)
{
  gs->cursor_row = -1;
  gs->cursor_col = -1;
  move_cursor_within_region(row, col, 0, framebuf_get_nrows()-1);
}

void INFLASHFUN move_cursor_limited(int row, int col)
{
  // only move if cursor is currently within scroll region, do not move
  // outside of scroll region
  if( gs->cursor_row >= gs->scroll_region_start && gs->cursor_row <= gs->scroll_region_end )
    move_cursor_within_region(row, col, gs->scroll_region_start, gs->scroll_region_end);
}


void INFLASHFUN terminal_process_text(char c)
{
  switch( c )
    {
    case 5: // ENQ => send answer-back string
      send_string(config_get_terminal_answerback());
      break;
      
    case 7: // BEL => produce beep
      sound_play_tone(config_get_audible_bell_frequency(), 
                      config_get_audible_bell_duration(), 
                      config_get_audible_bell_volume(), 
                      false);
      framebuf_flash_screen(config_get_visual_bell_color(), config_get_visual_bell_duration());
      break;
      
    case 8:   // backspace
    case 127: // delete
      {
        uint8_t mode = c==8 ? config_get_terminal_bs() : config_get_terminal_del();
        if( mode>0 )
          {
            int top_limit = gs->origin_mode ? gs->scroll_region_start : 0;
            if( gs->cursor_row>top_limit )
              move_cursor_wrap(gs->cursor_row, gs->cursor_col-1);
            else
              move_cursor_limited(gs->cursor_row, gs->cursor_col-1);

            if( mode==2 )
              {
                framebuf_set_char(gs->cursor_col, gs->cursor_row, ' ');
                framebuf_set_attr(gs->cursor_col, gs->cursor_row, 0);
                gs->cur_attr = 0;
                show_cursor(gs->cursor_shown);
              }
          }

        break;
      }

    case '\t': // horizontal tab
      {
        int col = gs->cursor_col+1;
        while( col < framebuf_get_ncols(gs->cursor_row)-1 && !(gs->tabs[col]) ) col++;
        move_cursor_limited(gs->cursor_row, col); 
        break;
      }
      
    case '\n': // newline
    case 11:   // vertical tab (interpreted as newline)
    case 12:   // form feed (interpreted as newline)
    case '\r': // carriage return
      {
        switch( c=='\r' ? config_get_terminal_cr() : config_get_terminal_lf() )
          {
          case 1: move_cursor_wrap(gs->cursor_row, 0); break;
          case 2: move_cursor_wrap(gs->cursor_row+1, gs->cursor_col); break;
          case 3: move_cursor_wrap(gs->cursor_row+1, 0); break;
          }
        break;
      }

    case 14:  // SO
      gs->charset = &(gs->charset_G1); 
      break;

    case 15:  // SI
      gs->charset = &(gs->charset_G0); 
      break;

    default: // regular character
      if( c>=32 ) print_char_vt(c);
      break;
    }
}

void INFLASHFUN terminal_process_command(char start_char, char final_char, uint8_t num_params, uint8_t *params)
{
  // NOTE: num_params>=1 always holds, if no parameters were received then params[0]=0
  if( final_char=='l' || final_char=='h' )
    {
      bool enabled = final_char=='h';
      if( start_char=='?' )
        {
          switch( params[0] )
            {
            case 2:
              if( !enabled ) { internal_terminal_reset(); gs->vt52_mode = true; }
              break;

            case 3: // switch 80/132 columm mode - 132 columns not supported but we can clear the screen
              internal_terminal_clear_screen();
              break;

            case 4: // enable smooth scrolling (emulated via scroll delay)
              framebuf_set_scroll_delay(enabled ? config_get_terminal_scrolldelay() : 0);
              break;
              
            case 5: // invert screen
              framebuf_set_screen_inverted(enabled);
              break;
          
            case 6: // origin mode
              gs->origin_mode = enabled; 
              move_cursor_limited(gs->scroll_region_start, 0); 
              break;
              
            case 7: // auto-wrap mode
              gs->auto_wrap_mode = enabled; 
              break;

            case 12: // local echo (send-receive mode)
              gs->localecho = !enabled;
              break;
              
            case 25: // show/hide cursor
              gs->cursor_shown = enabled;
              show_cursor(gs->cursor_shown);
              break;
            }
        }
      else if( start_char==0 )
        {
          switch( params[0] )
            {
            case 4: // insert mode
              gs->insert_mode = enabled;
              break;
            }
        }
    }
  else if( final_char=='J' )
    {
      switch( params[0] )
        {
        case 0:
          for(int i=gs->cursor_row; i<framebuf_get_nrows(); i++) framebuf_set_row_attr(i, 0);
          framebuf_fill_region(gs->cursor_col, gs->cursor_row, framebuf_get_ncols(gs->cursor_row)-1, framebuf_get_nrows()-1, ' ', gs->color_fg, gs->color_bg);
          break;
          
        case 1:
          for(int i=0; i<gs->cursor_row; i++) framebuf_set_row_attr(i, 0);
          framebuf_fill_region(0, 0, gs->cursor_col, gs->cursor_row, ' ', gs->color_fg, gs->color_bg);
          break;
          
        case 2:
          for(int i=0; i<framebuf_get_nrows(); i++) framebuf_set_row_attr(i, 0);
          framebuf_fill_region(0, 0, framebuf_get_ncols(gs->cursor_row)-1, framebuf_get_nrows()-1, ' ', gs->color_fg, gs->color_bg);
          break;
        }

      gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
      show_cursor(gs->cursor_shown);
    }
  else if( final_char=='K' )
    {
      switch( params[0] )
        {
        case 0:
          framebuf_fill_region(gs->cursor_col, gs->cursor_row, framebuf_get_ncols(gs->cursor_row)-1, gs->cursor_row, ' ', gs->color_fg, gs->color_bg);
          break;
          
        case 1:
          framebuf_fill_region(0, gs->cursor_row, gs->cursor_col, gs->cursor_row, ' ', gs->color_fg, gs->color_bg);
          break;
          
        case 2:
          framebuf_fill_region(0, gs->cursor_row, framebuf_get_ncols(gs->cursor_row)-1, gs->cursor_row, ' ', gs->color_fg, gs->color_bg);
          break;
        }

      gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
      show_cursor(gs->cursor_shown);
    }
  else if( final_char=='A' )
    {
      move_cursor_limited(gs->cursor_row-MAX(1, params[0]), gs->cursor_col);
    }
  else if( final_char=='B' )
    {
      move_cursor_limited(gs->cursor_row+MAX(1, params[0]), gs->cursor_col);
    }
  else if( final_char=='C' || final_char=='a' )
    {
      move_cursor_limited(gs->cursor_row, gs->cursor_col+MAX(1, params[0]));
    }
  else if( final_char=='D' || final_char=='j' )
    {
      move_cursor_limited(gs->cursor_row, gs->cursor_col-MAX(1, params[0]));
    }
  else if( final_char=='E' || final_char=='e' )
    {
      move_cursor_limited(gs->cursor_row+MAX(1, params[0]), 0);
    }
  else if( final_char=='F' || final_char=='k' )
    {
      move_cursor_limited(gs->cursor_row-MAX(1, params[0]), 0);
    }
  else if( final_char=='d' )
    {
      move_cursor_limited(MAX(1, params[0])-1, gs->cursor_col);
    }
  else if( final_char=='G' || final_char=='`' )
    {
      move_cursor_limited(gs->cursor_row, MAX(1, params[0])-1);
    }
  else if( final_char=='H' || final_char=='f' )
    {
      int top_limit    = gs->origin_mode ? gs->scroll_region_start : 0;
      int bottom_limit = gs->origin_mode ? gs->scroll_region_end   : framebuf_get_nrows()-1;
      move_cursor_within_region(top_limit+MAX(params[0],1)-1, num_params<2 ? 0 : MAX(params[1],1)-1, top_limit, bottom_limit);
    }
  else if( final_char=='I' )
    {
      int n = MAX(1, params[0]);
      int col = gs->cursor_col+1;
      while( n>0 && col < framebuf_get_ncols(gs->cursor_row)-1 )
        {
          while( col < framebuf_get_ncols(gs->cursor_row)-1 && !gs->tabs[col] ) col++;
          n--;
        }
      move_cursor_limited(gs->cursor_row, col); 
    }
  else if( final_char=='Z' )
    {
      int n = MAX(1, params[0]);
      int col = gs->cursor_col-1;
      while( n>0 && col>0 )
        {
          while( col>0 && !gs->tabs[col] ) col--;
          n--;
        }
      move_cursor_limited(gs->cursor_row, col); 
    }
  else if( final_char=='L' || final_char=='M' )
    {
      int n = MAX(1, params[0]);
      int bottom_limit = gs->origin_mode ? gs->scroll_region_end : framebuf_get_nrows()-1;
      show_cursor(false);
      framebuf_scroll_region(gs->cursor_row, bottom_limit, final_char=='M' ? n : -n, gs->color_fg, gs->color_bg);
      gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
      show_cursor(gs->cursor_shown);
    }
  else if( final_char=='@' )
    {
      int n = MAX(1, params[0]);
      show_cursor(false);
      framebuf_insert(gs->cursor_col, gs->cursor_row, n, gs->color_fg, gs->color_bg);
      gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
      show_cursor(gs->cursor_shown);
    }
  else if( final_char=='P' )
    {
      int n = MAX(1, params[0]);
      framebuf_delete(gs->cursor_col, gs->cursor_row, n, gs->color_fg, gs->color_bg);
      gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
      show_cursor(gs->cursor_shown);
    }
  else if( final_char=='S' || final_char=='T' )
    {
      int top_limit    = gs->origin_mode ? gs->scroll_region_start : 0;
      int bottom_limit = gs->origin_mode ? gs->scroll_region_end   : framebuf_get_nrows()-1;
      int n = MAX(1, params[0]);
      show_cursor(false);
      while( n-- ) framebuf_scroll_region(top_limit, bottom_limit, final_char=='S' ? n : -n, gs->color_fg, gs->color_bg);
      gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
      show_cursor(gs->cursor_shown);
    }
  else if( final_char=='g' )
    {
      int p = params[0];
      if( p==0 )
        gs->tabs[gs->cursor_col] = false;
      else if( p==3 )
        memset(gs->tabs, 0, framebuf_get_ncols(-1));
    }
  else if( final_char=='m' )
    {
      unsigned int i;
      for(i=0; i<num_params; i++)
        {
          int p = params[i];

          if( p==0 )
            {
              gs->color_fg = config_get_terminal_default_fg();
              gs->color_bg = config_get_terminal_default_bg();
              gs->attr     = config_get_terminal_default_attr();
              //cursor_shown = true;
              show_cursor(gs->cursor_shown);
            }
          else if( p==1 )
            gs->attr |= ATTR_BOLD;
          else if( p==4 )
            gs->attr |= ATTR_UNDERLINE;
          else if( p==5 )
            gs->attr |= ATTR_BLINK;
          else if( p==7 )
            gs->attr |= ATTR_INVERSE;
          else if( p==22 )
            gs->attr &= ~ATTR_BOLD;
          else if( p==24 )
            gs->attr &= ~ATTR_UNDERLINE;
          else if( p==25 )
            gs->attr &= ~ATTR_BLINK;
          else if( p==27 )
            gs->attr &= ~ATTR_INVERSE;
          else if( p>=30 && p<=37 )
            gs->color_fg = p-30;
          else if( p==38 && num_params>=i+2 && params[i+1]==5 )
            { gs->color_fg = params[i+2] & 15; i+=2; }
          else if( p==39 )
            gs->color_fg = config_get_terminal_default_fg();
          else if( p>=40 && p<=47 )
            gs->color_bg = p-40;
          else if( p==48 && num_params>=i+2 && params[i+1]==5 )
            { gs->color_bg = params[i+2] & 15; i+=2; }
          else if( p==49 )
            gs->color_bg = config_get_terminal_default_bg();

          show_cursor(gs->cursor_shown);
        }
    }
  else if( final_char=='r' )
    {
      if( num_params==2 && params[1]>params[0] )
        {
          gs->scroll_region_start = MAX(params[0], 1)-1;
          gs->scroll_region_end   = MIN(params[1], framebuf_get_nrows())-1;
        }
      else if( params[0]==0 )
        {
          gs->scroll_region_start = 0;
          gs->scroll_region_end   = framebuf_get_nrows()-1;
        }

      move_cursor_within_region(gs->scroll_region_start, 0, gs->scroll_region_start, gs->scroll_region_end);
    }
  else if( final_char=='s' )
    {
      gs->saved_row = gs->cursor_row;
      gs->saved_col = gs->cursor_col;
      gs->saved_eol = gs->cursor_eol;
      gs->saved_origin_mode = gs->origin_mode;
      gs->saved_fg  = gs->color_fg;
      gs->saved_bg  = gs->color_bg;
      gs->saved_attr = gs->attr;
      gs->saved_charset_G0 = gs->charset_G0;
      gs->saved_charset_G1 = gs->charset_G1;
    }
  else if( final_char=='u' )
    {
      move_cursor_limited(gs->saved_row, gs->saved_col);
      gs->origin_mode = gs->saved_origin_mode;      
      gs->cursor_eol = gs->saved_eol;
      gs->color_fg = gs->saved_fg;
      gs->color_bg = gs->saved_bg;
      gs->attr = gs->saved_attr;
      gs->charset_G0 = gs->saved_charset_G0;
      gs->charset_G1 = gs->saved_charset_G1;
    }
  else if( final_char=='c' )
    {
      // device gs->attributes resport
      // https://www.vt100.net/docs/vt100-ug/chapter3.html#DA
      // https://invisible-island.net/xterm/ctlseqs/ctlseqs.html#h4-Functions-using-CSI-_-ordered-by-the-final-character-lparen-s-rparen:CSI-Ps-c.1CA3
      // "ESC [?1;0c" => base VT100, no options
      // "ESC [?6c"   => VT102
      send_string("\033[?6c");
    }
  else if( final_char=='n' )
    {
      if( params[0] == 5 )
        {
          // terminal status report
          send_string("\033[0n");
        }
      else if( params[0] == 6 )
        {
          // cursor position report
          int top_limit = gs->origin_mode ? gs->scroll_region_start : 0;
          char buf[20];
          snprintf(buf, 20, "\033[%u;%uR", gs->cursor_row-top_limit+1, gs->cursor_col+1);
          send_string(buf);
        }
    }
}