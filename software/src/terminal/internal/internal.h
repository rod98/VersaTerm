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

// void internal_terminal_receive_char(char c);
// void internal_terminal_receive_string(const char* str);
// void internal_terminal_process_key(uint16_t key);

void internal_terminal_clear_screen();
void internal_terminal_init();
void internal_terminal_apply_settings();

void internal_terminal_process_text(char c);
void internal_terminal_process_command(char start_char, char final_char, uint8_t num_params, uint8_t *params);

void init_cursor(int row, int col);
void print_char_vt(char c);
void move_cursor_wrap(int row, int col);
void show_cursor(bool show);

uint8_t get_charset(char c);


void send_char(char c);
void send_string(const char *s);


void internal_terminal_reset();

void send_cursor_sequence(char c);


#endif