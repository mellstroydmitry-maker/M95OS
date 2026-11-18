#include "keyboard.h"
#include <stdint.h>

static int ru_mode = 0;
static int alt_down = 0;
static int shift_down = 0;
static char last_char = 0;

void keyboard_set_ru(int ru) { ru_mode = ru ? 1 : 0; }
int keyboard_is_ru(void) { return ru_mode; }

static const char sc_en[] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
};

static const uint8_t sc_ru[] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t', 137,150,147,138,133,141,131,152,153,135,149,154,'\n',
    0, 148,155,130,128,143,144,142,139,132,134,157,160,
    0,'\\',159,151,145,140,136,146,156,129,158,'.',0,'*',0,' '
};

void keyboard_handle_scancode(uint8_t sc) {
    if (sc == 0x2A || sc == 0x36) { shift_down = 1; if (alt_down) ru_mode = !ru_mode; return; }
    if (sc == 0xAA || sc == 0xB6) { shift_down = 0; return; }
    if (sc == 0x38) { alt_down = 1; if (shift_down) ru_mode = !ru_mode; return; }
    if (sc == 0xB8) { alt_down = 0; return; }
    if (sc & 0x80) return;

    if (sc < sizeof(sc_en)) {
        last_char = ru_mode ? (char)*(sc_ru + sc) : *(sc_en + sc);
    }
}

char keyboard_poll_char(void) {
    char c = last_char;
    last_char = 0;
    return c;
}

static char utf8_tmp[] = { 0, 0, 0 };

const char* keyboard_char_to_utf8(char c) {
    uint8_t u = (uint8_t)c;
    if (u < 128) {
        *(utf8_tmp + 0) = c;
        *(utf8_tmp + 1) = '\0';
        return utf8_tmp;
    }
    if (u >= 128 && u <= 159) {
        uint16_t cp = 0x0430 + (u - 128);
        *(utf8_tmp + 0) = (char)(0xD0 | (cp >> 6));
        *(utf8_tmp + 1) = (char)(0x80 | (cp & 0x3F));
        *(utf8_tmp + 2) = '\0';
        return utf8_tmp;
    }
    if (u == 160) {
        *(utf8_tmp + 0) = (char)0xD1;
        *(utf8_tmp + 1) = (char)0x91;
        *(utf8_tmp + 2) = '\0';
        return utf8_tmp;
    }
    *(utf8_tmp + 0) = ' ';
    *(utf8_tmp + 1) = '\0';
    return utf8_tmp;
}
