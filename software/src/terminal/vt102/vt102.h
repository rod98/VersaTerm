#ifndef TERMINAL_VT102_H
#define TERMINAL_VT102_H

#include "../internal/global_state.h"

void terminal_receive_char_vt102(char c);
void terminal_process_key_vt(uint16_t key);

#endif