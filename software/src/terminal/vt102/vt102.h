#ifndef TERMINAL_VT102_H
#define TERMINAL_VT102_H

#include "../internal/global_state.h"

void terminal_receive_char_vt102(global_state *gs, char c);

#endif