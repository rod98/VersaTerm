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

#include "framebuf.h"
#include "font.h"
#include "terminal/terminal.h"
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

// defined in main.c
void wait(uint32_t milliseconds);

#define INFLASHFUN __in_flash(".terminalfun") 

#define TS_NORMAL      0
#define TS_WAITBRACKET 1
#define TS_STARTCHAR   2 
#define TS_READPARAM   3
#define TS_HASH        4
#define TS_READCHAR    5

#define CS_TEXT_US  0
#define CS_TEXT_UK  1
#define CS_GRAPHICS 2

static uint8_t terminal_state = TS_NORMAL;
static uint8_t color_fg, color_bg, attr = 0, cur_attr = 0;
static int cursor_col = 0, cursor_row = 0, saved_col = 0, saved_row = 0;
static int scroll_region_start, scroll_region_end;
static bool cursor_shown = true, origin_mode = false, cursor_eol = false, auto_wrap_mode = true, vt52_mode = false, localecho = false;
static bool saved_eol = false, saved_origin_mode = false, insert_mode = false;
static bool petscii_lower_case_charset = true;
static uint8_t saved_attr, saved_fg, saved_bg, saved_charset_G0, saved_charset_G1, *charset, charset_G0, charset_G1, tabs[255];








static void INFLASHFUN print_char_petscii(char c)
{
  framebuf_set_color(cursor_col, cursor_row, color_fg, color_bg);
  framebuf_set_attr(cursor_col, cursor_row, attr);
  framebuf_set_char(cursor_col, cursor_row, c);
  int row = cursor_row, col = cursor_col;
  cursor_row = -1;
  cursor_col = -1;
  move_cursor_wrap(row, col+1);
}


void INFLASHFUN terminal_reset()
{
  saved_col = 0;
  saved_row = 0;
  cursor_shown = true;
  color_fg = config_get_terminal_default_fg();
  color_bg = config_get_terminal_default_bg();
  scroll_region_start = 0;
  scroll_region_end = framebuf_get_nrows()-1;
  origin_mode = false;
  cursor_eol = false;
  auto_wrap_mode = true;
  insert_mode = false;
  vt52_mode = false;
  attr = config_get_terminal_default_attr();
  saved_attr = 0;
  charset_G0 = CS_TEXT_US;
  charset_G1 = CS_GRAPHICS;
  saved_charset_G0 = CS_TEXT_US;
  saved_charset_G1 = CS_GRAPHICS;
  charset = &charset_G0;
  memset(tabs, 0, framebuf_get_ncols(-1));
  framebuf_set_scroll_delay(0);
  localecho = config_get_terminal_localecho();
  petscii_lower_case_charset = true;
}


void INFLASHFUN terminal_clear_screen()
{
  framebuf_fill_screen(' ', color_fg, color_bg);
  init_cursor(0, 0);
  scroll_region_start = 0;
  scroll_region_end = framebuf_get_nrows()-1;
  origin_mode = false;
}


static void INFLASHFUN send_char(char c)
{
  serial_send_char(c);
  if( localecho ) terminal_receive_char(c);
}


static void INFLASHFUN send_string(const char *s)
{
  serial_send_string(s);
  if( localecho ) terminal_receive_string(s);
}


static INFLASHFUN void terminal_process_text(char c)
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
            int top_limit = origin_mode ? scroll_region_start : 0;
            if( cursor_row>top_limit )
              move_cursor_wrap(cursor_row, cursor_col-1);
            else
              move_cursor_limited(cursor_row, cursor_col-1);

            if( mode==2 )
              {
                framebuf_set_char(cursor_col, cursor_row, ' ');
                framebuf_set_attr(cursor_col, cursor_row, 0);
                cur_attr = 0;
                show_cursor(cursor_shown);
              }
          }

        break;
      }

    case '\t': // horizontal tab
      {
        int col = cursor_col+1;
        while( col < framebuf_get_ncols(cursor_row)-1 && !tabs[col] ) col++;
        move_cursor_limited(cursor_row, col); 
        break;
      }
      
    case '\n': // newline
    case 11:   // vertical tab (interpreted as newline)
    case 12:   // form feed (interpreted as newline)
    case '\r': // carriage return
      {
        switch( c=='\r' ? config_get_terminal_cr() : config_get_terminal_lf() )
          {
          case 1: move_cursor_wrap(cursor_row, 0); break;
          case 2: move_cursor_wrap(cursor_row+1, cursor_col); break;
          case 3: move_cursor_wrap(cursor_row+1, 0); break;
          }
        break;
      }

    case 14:  // SO
      charset = &charset_G1; 
      break;

    case 15:  // SI
      charset = &charset_G0; 
      break;

    default: // regular character
      if( c>=32 ) print_char_vt(c);
      break;
    }
}
















static void INFLASHFUN send_cursor_sequence(char c)
{
  if( config_get_terminal_type()==CFG_TTYPE_VT52 || vt52_mode )
    { send_char(27); send_char(c); }
  else
    { send_char(27); send_char('['); send_char(c); }
}





static void INFLASHFUN terminal_process_key_petscii(uint16_t key)
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

void INFLASHFUN terminal_process_key(uint16_t key)
{
  if( (key&0xFF)==HID_KEY_PAUSE )
    {
      if( keyboard_ctrl_pressed(key) )
        {
          // CTRL-Pause/Break sends answerback message
          send_string(config_get_terminal_answerback());
        }
      else
        {
          // Pause/Break key sends BREAK condition on serial port
          serial_set_break(true);
          wait(MAX(1, (12000/config_get_serial_baud())));
          serial_set_break(false);
        }
    }
  else if( key==HID_KEY_F10 )
    {
      sound_play_tone(880, 50, config_get_audible_bell_volume(), false);
      localecho = !localecho;
    }
  else if( config_get_terminal_type()==2 )
    terminal_process_key_petscii(key);
  else
    terminal_process_key_vt(key);
}


void INFLASHFUN terminal_init()
{
  terminal_reset();
  terminal_clear_screen();
}


void INFLASHFUN terminal_apply_settings()
{
  terminal_init();
}
