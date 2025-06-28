#ifndef UTF2FONT_H
#define UTF2FONT_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    unsigned char character ;
    bool          need_shift;
} font_char;

font_char utf2font(uint32_t utf_char);

#endif