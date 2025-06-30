#include "utf2font.h"

#define GRAPH_ON  14
#define GRAPH_OFF 15

#define B_INCL(_val, _mn, _mx) (_mn <= _val && _val <= _mx)

char rus2font[] = {
    144, 'E', ' ', 108, 238, 'S', 'I', 'I', 'J', ' ', ' ', 'h', 'K', 'N', 'Y', 249,
    'A', '6', 'B', 226,  17, 'E',  15, '3',  19,  19, 'K',   6, 'M', 'H', 'O', 239, 
    'P', 'C', 'T', 'Y', 232, 'X',  11, '4', 'W', 'W',  31,  14,  31, 228, 172,   9,
    'a', 235,   5, 'r',  16, 'e',  23,   0,  18, 127, 'k',   2, 'm',   1, 'o', 227,
    'p', 'c', 't', 'y', 236, 'x',  10,  12, 'w', 'w',  30,  13,  30,  29, 171,   8, 
    138, 137
};

// 255 is essentially space
char box2font[] = {
//
    196, 205, 179, 186, 196, 205, 179, 186, 196, 205, 179, 186, 218, 213, 214, 201,
//                                                               ├    ┝    ┞    ┟
    191, 184, 183, 187, 192, 212, 211, 200, 217, 190, 189, 188, 195, 198, 255, 255,
//   ┠    ┡    ┢    ┣    ┤    ┥    ┦    ┧    ┨    ┩    ┪    ┫    ┬              ┯
    199, 255, 255, 204, 180, 181, 255, 255, 182, 255, 255, 185, 194, 255, 255, 209,
//   ┰              ┳    ┴              ┷    ┸              ┻    ┼    ┽    ┾    ┿
    210, 255, 255, 203, 193, 255, 255, 207, 208, 255, 255, 202, 197, 255, 255, 216,
//   ╀    ╁    ╂    ╃    ╄    ╅    ╆    ╇    ╈    ╉    ╊         ╋
    255, 255, 215, 255, 255, 255, 255, 255, 255, 255, 255, 206, '-', '-', '|',
//        ═    ║    ╒    ╓    ╔    ╕    ╖    ╗    ╘    ╙    ╚    ╛    ╜    ╝    ╞
    '|', 205, 186, 213, 214, 201, 184, 183, 187, 212, 211, 200, 190, 189, 188, 198,
//   ╟    ╠    ╡    ╢    ╣    ╤    ╥    ╦    ╧    ╨    ╩    ╪    ╫    ╬
    199, 204, 181, 182, 185, 209, 210, 203, 207, 208, 202, 216, 215, 206

};

int utf2font(uint32_t utf_char) {
    int chr = 4;

    if (utf_char <= 127) // ASCII
      chr = (int)utf_char;
    else if (B_INCL(utf_char,   768,   879)) // diacritics - do not display
        chr = -1;
    else if (B_INCL(utf_char,  1024,  1105)) // Russian
        chr = rus2font[utf_char - 1024];
    else if (B_INCL(utf_char,  9472,  9580))
        chr = box2font[utf_char - 9472];
    else switch (utf_char)
    {
        case 160: chr = ' '; break; 
        case 167: chr =  22; break;
        case 171: chr = 174; break;
        case 172: chr = 170; break;
        case 181: chr = 230; break;
        case 182: chr =  20; break;
        case 187: chr = 175; break;

        // cfdisk uses UTF sequences to generate graphical characters in acccording to the default font
        // why? I do not understand :(
        case 196:
        case 218:
        case 191:
        case 179:
          chr = (int)utf_char;
    }
        
    return chr;
}