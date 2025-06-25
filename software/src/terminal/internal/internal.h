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

void internal_terminal_receive_char(global_state *gs, char c);
void internal_terminal_receive_string(global_state *gs, const char* str);
void internal_terminal_process_key(global_state *gs, uint16_t key);

void internal_terminal_clear_screen(global_state *gs);
void internal_terminal_init(global_state *gs);
void internal_terminal_apply_settings(global_state *gs);

void internal_terminal_process_text(global_state *gs, char c);
void internal_terminal_process_command(global_state *gs, char start_char, char final_char, uint8_t num_params, uint8_t *params);

void init_cursor(global_state *gs, int row, int col);
void print_char_vt(global_state *gs, char c);
void move_cursor_wrap(global_state *gs, int row, int col);
void show_cursor(global_state *gs, bool show);

uint8_t get_charset(char c);



void internal_terminal_reset(global_state *gs);


#endif