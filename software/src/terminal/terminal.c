#include "terminal.h"
#include "internal/internal.h"

#include "vt102/vt102.h"
#include "vt52/vt52.h"
#include "petscii/petscii.h"

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
static global_state *gs = &glob_st;

void wait(uint32_t milliseconds);

void INFLASHFUN terminal_receive_char(char c)
{
  if( config_get_terminal_clearBit7() ) c &= 0x7f;

  switch( config_get_terminal_type() )
    {
    case CFG_TTYPE_VT102:
      if( !gs->vt52_mode ) { terminal_receive_char_vt102(c); break; }

    case CFG_TTYPE_VT52:
      terminal_receive_char_vt52(c);
      break;

    case CFG_TTYPE_PETSCII:
      terminal_receive_char_petscii(c);
      break;
    }
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
      gs->localecho = !gs->localecho;
    }
//   else if( config_get_terminal_type()==2 )
//     terminal_process_key_petscii(key);
  else
    terminal_process_key_vt(key);
}


void INFLASHFUN terminal_receive_string(const char* str)
{
  while( *str ) { terminal_receive_char(*str); str++; }
}

// void INFLASHFUN terminal_process_key(uint16_t key) {
//     internal_terminal_process_key(key);
// }

void INFLASHFUN terminal_clear_screen()
{
    internal_terminal_clear_screen();
}

void INFLASHFUN terminal_init() {
    internal_terminal_init();
}

void INFLASHFUN terminal_apply_settings() {
    internal_terminal_init();
}

void INFLASHFUN terminal_reset() {
    internal_terminal_reset();
}