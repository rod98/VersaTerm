#ifndef GLOBAL_STATE_H
#define GLOBAL_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t terminal_state;
    uint8_t color_fg, color_bg, attr, cur_attr;
    int cursor_col, cursor_row, saved_col, saved_row;
    int scroll_region_start, scroll_region_end;
    bool cursor_shown, origin_mode, cursor_eol, auto_wrap_mode, vt52_mode, localecho;
    bool saved_eol, saved_origin_mode, insert_mode;
    bool petscii_lower_case_charset;
    uint8_t saved_attr, saved_fg, saved_bg, saved_charset_G0, saved_charset_G1, *charset, charset_G0, charset_G1, tabs[255];
} global_state;


global_state create_global_state(void);

#endif