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

#include "petscii.h"
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
#include <stdint.h>

#include "../internal/internal.h"
#include "../terminal.h"

#define INFLASHFUN __in_flash(".terminalfun") 

void wait(uint32_t milliseconds);

extern global_state glob_st;
static global_state *gs = &glob_st;

static bool petscii_lower_case_charset = true;

static void INFLASHFUN print_char_petscii(char c)
{
  framebuf_set_color(gs->cursor_col, gs->cursor_row, gs->color_fg, gs->color_bg);
  framebuf_set_attr(gs->cursor_col, gs->cursor_row, gs->attr);
  framebuf_set_char(gs->cursor_col, gs->cursor_row, c);
  int row = gs->cursor_row, col = gs->cursor_col;
  gs->cursor_row = -1;
  gs->cursor_col = -1;
  move_cursor_wrap(row, col+1);
}

void INFLASHFUN terminal_receive_char_petscii(uint8_t c)
{
  static uint8_t inserted = 0;
  static bool quoteMode = false;

  if( c>=192 )
    {
      if     ( c<=223 ) c -= 96;
      else if( c<=254 ) c -= 64;
      else if( c==255 ) c  = 126;
    }

  if( c==34 ) quoteMode=!quoteMode;

  if( (quoteMode || inserted>0) )
    {
      uint8_t cc = 0;
      if( c<32 && c!=13 && (!quoteMode || c!=20) )
        {
          switch( c )
            {
            case 27: cc = 0x5B; break;
            case 28: cc = 0x9A; break;
            case 29: cc = 0x5D; break;
            case 30: cc = 0x98; break;
            case 31: cc = 0x99; break;
            default: cc = petscii_lower_case_charset ? c+96 : c+64; break;
            }
        }
      else if( c>=128 && c<160 && (quoteMode || c!=148) )
        {
          switch( c )
            {
            case 155: cc = 0x9B; break;
            case 156: cc = 0x9C; break;
            case 157: cc = 0x9D; break;
            case 158: cc = petscii_lower_case_charset ? 0x9E : 0xCE; break;
            case 159: cc = petscii_lower_case_charset ? 0x9F : 0xDF; break;
            default:  cc = petscii_lower_case_charset ? c-64 : c+64; break;
            }
        }
          
      if( cc>0 )
        {
          uint8_t a = gs->attr;
          gs->attr |= ATTR_INVERSE;
          print_char_petscii(cc);
          if( inserted>0 ) inserted--;
          gs->attr = a;
          return;
        }
    }
      
  switch( c )
    {
    case 5: // WHITE
      gs->color_fg = 1;
      break;

    case 10:  // LF
    case 13:  // CR
    case 141: // shift+CR
      {
        switch( c==10 ? config_get_terminal_lf() : config_get_terminal_cr() )
          {
          case 1: move_cursor_wrap(gs->cursor_row, 0); break;
          case 2: move_cursor_wrap(gs->cursor_row+1, gs->cursor_col); break;
          case 3: move_cursor_wrap(gs->cursor_row+1, 0); break;
          }
        if( c!=10 ) { inserted = 0; quoteMode = false; gs->attr &= ~ATTR_INVERSE; }
        break;
      }

    case 14: // Switch to lower case character set
      petscii_lower_case_charset = true;
      for(uint8_t row=0; row<framebuf_get_nrows(); row++)
        for(uint8_t col=0; col<framebuf_get_ncols(col); col++)
          {
            char c = framebuf_get_char(col, row);
            if( c>='A' && c<='Z' )
              c += 32;
            else if( c>='A'+128 && c<='Z'+128 ) 
              c -= 128;
            else if( c==0xDE || c==0xDF || c==0xE9 || c==0xFA )
              c -= 64;
            framebuf_set_char(col, row, c);
          }
      break;

    case 17: // cursor down
      move_cursor_wrap(gs->cursor_row+1, gs->cursor_col);
      break;

    case 18: // enable reverse character mode
      gs->attr |= ATTR_INVERSE;
      break;

    case 19: // cursor home
      move_cursor_wrap(0, 0);
      break;

    case 20: // backspace/delete
      if( gs->cursor_col>0 || gs->cursor_row>0 )
        {
          move_cursor_wrap(gs->cursor_row, gs->cursor_col-1);
          framebuf_delete(gs->cursor_col, gs->cursor_row, 1, gs->color_fg, gs->color_bg);
          gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
          show_cursor(gs->cursor_shown);
        }
      break;

    case 28: // red
      gs->color_fg = 2;
      break;

    case 29: // cursor right
      move_cursor_wrap(gs->cursor_row, gs->cursor_col+1);
      break;
      
    case 30: // green
      gs->color_fg = 5;
      break;

    case 31: // blue
      gs->color_fg = 6;
      break;

    case 129: // orange
      gs->color_fg = 8;
      break;

    case 142: // Switch to upper case character set
      petscii_lower_case_charset = false;
      for(uint8_t row=0; row<framebuf_get_nrows(); row++)
        for(uint8_t col=0; col<framebuf_get_ncols(col); col++)
          {
            char c = framebuf_get_char(col, row);
            if( c>='a' && c<='z' ) 
              c -= 32;
            else if( c>='A' && c<='Z' )
              c += 128;
            else if( c==0x9E || c==0x9F || c==0xA9 || c==0xBA )
              c += 64;
            framebuf_set_char(col, row, c);
          }
      break;

    case 144: // black
      gs->color_fg = 0;
      break;

    case 145: // cursor up
      move_cursor_limited(gs->cursor_row-1, gs->cursor_col);
      break;

    case 146: // disable reverse character mode
      gs->attr &= ~ATTR_INVERSE;
      break;

    case 147: // clear screen
      terminal_clear_screen();
      break;

    case 148: // insert
      show_cursor(false);
      framebuf_insert(gs->cursor_col, gs->cursor_row, 1, gs->color_fg, gs->color_bg);
      gs->cur_attr = framebuf_get_attr(gs->cursor_col, gs->cursor_row);
      show_cursor(gs->cursor_shown);
      inserted++;
      break;

    case 149: // brown
      gs->color_fg = 9;
      break;

    case 150: // light red
      gs->color_fg = 10;
      break;

    case 151: // dark grey
      gs->color_fg = 11;
      break;

    case 152: // grey
      gs->color_fg = 12;
      break;

    case 153: // light green
      gs->color_fg = 13;
      break;

    case 154: // light blue
      gs->color_fg = 14;
      break;

    case 155: // light gray
      gs->color_fg = 15;
      break;

    case 156: // purple
      gs->color_fg = 4;
      break;

    case 157: // cursor left
      if( gs->cursor_row>0 )
        move_cursor_wrap(gs->cursor_row, gs->cursor_col-1);
      else
        move_cursor_limited(gs->cursor_row, gs->cursor_col-1);
      break;

    case 158: // yellow
      gs->color_fg = 7;
      break;

    case 159: // cyan
      gs->color_fg = 3;
      break;

    default:
      {
        if( c>=65 && c<=90 && petscii_lower_case_charset )
          c += 32;
        else if( c>=97 && c<=122 )
          c  = petscii_lower_case_charset ? c-32 : c+96;
        else if( c>=149 && c<=191 && c!=169 && c!=186 )
          c += 64; // more PETSCII graphics characters
        else if( c>=133 && c<=140 )
          c = 0; // function keys (no function in terminal and not printable)
        else
          {
            switch( c )
              {
              case  92: c = 0x9A; break; // pound symbol
              case  94: c = 0x98; break; // up arrow
              case  95: c = 0x99; break; // left arrow
              case  96: c = 0xC0; break; // middle horizontal line
              case 123: c = 0x9B; break; // cross
              case 124: c = 0x9C; break; // left checkerboard
              case 125: c = 0x9D; break; // middle vertical line
              case 126: c = petscii_lower_case_charset ? 0x9E : 0xDE ; break; // full checkerboard / pi
              case 127: c = petscii_lower_case_charset ? 0x9F : 0xDF ; break; // down diagonals / top right triangle
              case 169: c = petscii_lower_case_charset ? 0xA9 : 0xE9 ; break; // up diagonals / top left triangle
              case 186: c = petscii_lower_case_charset ? 0xBA : 0xFA ; break; // checkmark / bottom right corner
              }
          }

        if( c>0 ) print_char_petscii(c);
        if( inserted>0 ) inserted--;
        break;
      }
    }
}

void INFLASHFUN terminal_process_key_petscii(uint16_t key)
{
  uint8_t cc = 0;

  // mapping the key to ASCII performs three important functions:
  // - apply mapping according to keyboard layout (language)
  // - map numpad keys to regular keys
  // - provide LeftAlt-NNN for entering specific codes
  bool isaltcode = false;
  uint8_t c = keyboard_map_key_ascii(key, &isaltcode);

  // if c is the result of user pressing LeftAlt-NNN then send without mapping
  if( isaltcode ) { send_char(c); return; }

  switch( c )
    {
    case KEY_UP:        cc = 145; break;
    case KEY_DOWN:      cc = 17;  break;
    case KEY_RIGHT:     cc = 29;  break;
    case KEY_LEFT:      cc = 157; break;
    case KEY_ENTER:     cc = 13;  break;
    case KEY_HOME:      cc = keyboard_shift_pressed(key) ? 147 : 19;  break;
    case KEY_BACKSPACE: cc = 20;  break;
    case KEY_DELETE:    cc = 20;  break;
    case KEY_INSERT:    cc = 148; break;
    case KEY_F1:        cc = 133; break;
    case KEY_F2:        cc = 137; break;
    case KEY_F3:        cc = 134; break;
    case KEY_F4:        cc = 138; break;
    case KEY_F5:        cc = 135; break;
    case KEY_F6:        cc = 139; break;
    case KEY_F7:        cc = 136; break;
    case KEY_F8:        cc = 140; break;
    case '`':           cc =  95; break; // left arrow 
    case '\\':          cc =  94; break; // up arrow
    case '|':           cc = 126; break; // pi
    case '_':           cc = 123; break; // full cross
    case '{':           cc = 186; break; // bottom right corner
    case '}':           cc = 192; break; // middle line
    case '-':           cc = keyboard_alt_pressed(key) ? 126 : c; break; // full checkerboard
    case '[':           cc = keyboard_alt_pressed(key) ? 164 : c; break; // underscore
    case ']':           cc = keyboard_alt_pressed(key) ? 223 : c; break; // top right triangle

    default:  
      {
        if( keyboard_alt_pressed(key) && c>='a' && c <='z' )
          {
            static const uint8_t gfx[26] = {176, 191, 188, 172, 177, 187, 165, 180, 162, 181, 161, 182, 167, 
                                            170, 185, 175, 171, 178, 174, 163, 184, 190, 179, 189, 183, 173};
            cc = gfx[c-'a'];
          }
        else if( keyboard_ctrl_pressed(key) && c>='0' && c<='9' )
          {
            static const uint8_t colors[10] = {146, 144, 5, 28, 159, 156, 30, 31, 158, 18};
            cc = colors[c-'0'];
          }
        else if( keyboard_alt_pressed(key) && c>='1' && c<='8' )
          {
            static const uint8_t colors[8] = {129, 149, 150, 151, 152, 153, 154, 155};
            cc = colors[c-'1'];
          }
        else if( keyboard_ctrl_pressed(key) && keyboard_shift_pressed(key) && (key&0xFF)==HID_KEY_Z )
          cc = petscii_lower_case_charset ? 142 : 14;
        else if( c>='a' && c<='z' )
          cc = c - 32;
        else if( c>='A' && c<='Z' )
          cc = c + 128;
        else //if( c<127 )
          cc = c;
      }
          
      break;
    }

  if( cc>0 ) send_char(cc);
}
