#include "fansi.h"
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
#include "../vt102/vt102.h"
#include "../terminal.h"

#define INFLASHFUN __in_flash(".terminalfun") 

extern global_state glob_st;
static global_state *gs = &glob_st;

void terminal_receive_char_fansi(char c) {
    terminal_receive_char_vt102(c);
}

void INFLASHFUN terminal_process_key_fansi(uint16_t key)
{
    bool isaltcode;
    uint8_t c = keyboard_map_key_ascii(key, &isaltcode);

    // not touching black magic :p
    if (isaltcode)
        terminal_process_key_vt(key);
    else {
        switch (c)
        {
            case KEY_HOME:
                send_string("\e[H");
                break;

            case KEY_END:
                send_string("\e[F");
                break;

            case KEY_PUP:
                send_string("\e[5~");
                break;

            case KEY_PDOWN:
                send_string("\e[6~");
                break;
            
            default:
                terminal_process_key_vt(key);
                break;
        }
    }
}