#include "utf.h"

#define GRAPH_ON  14
#define GRAPH_OFF 15

static char rus2eng_[];

char *utf2font(uint32_t utf_char) {
    static char def_str[] = {254, 0};
    static char def_rus[] = {137, 0};

    // Russian
    if (utf_char >= 1024 && utf_char < 1280)
        return def_rus;
    else
        return def_str;
}