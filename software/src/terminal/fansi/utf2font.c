#include "utf2font.h"

#define GRAPH_ON  14
#define GRAPH_OFF 15

#define GR(x) GRAPH_ON, x, GRAPH_OFF

char rus2font[] = {
    144,
    'E',
    ' ',
    108,
    238,
    'S',
    'I',
    'I',
    'J',
    ' ',
    ' ',
    'h',
    'K',
    'N',
    'Y',
    249,
    'A', // finally A
    '6',
    'B',
    226,
     17,
    'E',
     15, // Ж 
    '3',
     19,
     19, // Й :(
    'K',
      6, // Л
    'M',
    'H',
    'O',
    239,
    'P',
    'C',
    'T',
    'Y',
    232,
    'X',
     11,
    '4',
    'W',
    'W',
     31,
     14,
     31,
    228,
    172,
      9, // Я
    'a', // a
    235, // б
      5,
    'r',
     16,
    'e',
     23, // ж
      0, // з
     18,
    127,
    'k',
      2, // л
    'm',
      1,
    'o',
    227,
    'p',
    'c',
    't',
    'y',
    236,
    'x',
     10,
     12,
    'w',
    'w', // щ
     30,
     13,
     30,
     29,
    171,
      8, // я
    138,
    137
};

char utf2font(uint32_t utf_char) {
    char def_str = 254;
    char def_rus = 29;

    // Russian
    if (utf_char >= 1024 && utf_char < 1106)
        return rus2font[utf_char - 1024];
    else if (utf_char < 1280)
        return def_rus;
    else
        return def_str;
}