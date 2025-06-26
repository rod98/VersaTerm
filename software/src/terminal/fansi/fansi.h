#ifndef FANSI_H
#define FANSI_H

#include "../internal/internal.h"

void terminal_receive_char_fansi(char c);
void terminal_process_key_fansi (uint16_t key);

#endif