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
#include "utf2font.h"

#define INFLASHFUN __in_flash(".terminalfun") 

extern global_state glob_st;
static global_state *gs = &glob_st;

static uint8_t  utfs_left = 0;
static uint32_t utfs_sum  = 0;
static bool     in_utf    = false;

void INFLASHFUN terminal_receive_char_fansi(char c) {
    unsigned char uc  = (unsigned char)c;
    unsigned char bit = 1 << 7;
    
    if (in_utf) {
        if (uc < 128) {
            unsigned char restored = utfs_sum;
            int i;

            in_utf = false;

            for (i = 0; i <= utfs_left; ++i)
                restored |= bit >> i;

            terminal_receive_char_vt102(restored);
            terminal_receive_char_vt102(uc);
        }
        else {
            utfs_left -= 1;

            utfs_sum <<= 6;
            utfs_sum  += (bit - 1) & uc;

            if (!utfs_left) {
                int i;
                font_char fc = utf2font(utfs_sum);

                if (fc.need_shift)
                    terminal_receive_char_vt102(14);    
                
                terminal_receive_char_vt102(fc.character);

                if (fc.need_shift)
                    terminal_receive_char_vt102(15); 

                in_utf = false;
            }
        }
    }
    else {
        if ((uc & (128 + 64)) != (128 + 64)) {
            terminal_receive_char_vt102(c);
        }
        else {
            in_utf = true;

            while (bit & uc) {
                utfs_left += 1;
                bit      >>= 1;
            }
            utfs_left -= 1;

            utfs_sum = ((bit - 1) & uc);
        }
    }

    if (!in_utf) {
        utfs_sum  = 0;
        utfs_left = 0;
    }
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