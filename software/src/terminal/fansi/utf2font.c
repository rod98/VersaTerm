#include "utf2font.h"

#define GRAPH_ON  14
#define GRAPH_OFF 15

#define GR(x) GRAPH_ON, x, GRAPH_OFF

font_char rus2font[] = {
    { 144, false},
    { 'E', false},
    { ' ', false},
    { 108, true },
    { 238, false},
    { 'S', false},
    { 'I', false},
    { 'I', false},
    { 'J', false},
    { ' ', false},
    { ' ', false},
    { 'h', false},
    { 'K', false},
    { 'N', false},
    { 'Y', false},
    { 249, false},
    { 'A', false}, // finally A
    { 156, false},
    { 'B', false},
    { 244, true },
    { 234, false},
    { 'E', false},
    { '*', false},
    { '3', false},
    { 'I', false},
    { 'I', false},  // Й :(
    { 'K', false},
    { 'L', false},
    { 'M', false},
    { 'H', false},
    { 'O', false},
    { 227, false},
    { 'P', false},
    { 'C', false},
    { 'T', false},
    { 'Y', false},
    { 232, false},
    { 'X', false},
    { 211, false},
    { '4', false},
    { 'W', false},
    { 'W', false},
    { '"', false},
    { 230, false},
    {'\'', false},
    { '}', false},
    { 154, false},
    { 142, false}, // Я
    { 'a', false}, // a
    { 235, false},
    { 'v', false},
    { 'r', false},
    { 208, false},
    { 'e', false},
    { '*', false},
    { '3', false},
    { 'i', false},
    { 141, false},
    { 'k', false},
    { 'l', false},
    { 'm', false},
    { 'h', false},
    { 'o', false},
    { 227, false},
    { 'p', false},
    { 'c', false},
    { 't', false},
    { 'y', false},
    { 237, false},
    { 'x', false},
    { 191, false},
    { '4', false},
    { 'w', false},
    { 'w', false}, // щ
    { '"', false},
    { 230, false},
    {'\'', false},
    { 238, false},
    { 129, false},
    { 132, false}, // я
    { 138, false},
    { 137, false}
};

font_char utf2font(uint32_t utf_char) {
    font_char def_str = (font_char){254, false};
    font_char def_rus = (font_char){ 96, true};

    // Russian
    if (utf_char >= 1024 && utf_char < 1106)
        return rus2font[utf_char - 1024];
    else if (utf_char < 1280)
        return def_rus;
    else
        return def_str;
}