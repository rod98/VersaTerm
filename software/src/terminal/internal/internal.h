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

#ifndef TERMINAL_INTERNAL_H
#define TERMINAL_INTERNAL_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "global_state.h"

#define TS_NORMAL      0
#define TS_WAITBRACKET 1
#define TS_STARTCHAR   2 
#define TS_READPARAM   3
#define TS_HASH        4
#define TS_READCHAR    5

#define CS_TEXT_US  0
#define CS_TEXT_UK  1
#define CS_GRAPHICS 2

void internal_terminal_clear_screen();
void internal_terminal_init();
// void internal_terminal_apply_settings();

void terminal_process_text(char c);
void terminal_process_command(char start_char, char final_char, uint8_t num_params, uint8_t *params);

void init_cursor(int row, int col);
void print_char_vt(char c);
void move_cursor_wrap(int row, int col);
void move_cursor_limited(int row, int col);
void show_cursor(bool show);

uint8_t get_charset(char c);


void send_char(char c);
void send_string(const char *s);


void internal_terminal_reset();

void send_cursor_sequence(char c);


#endif