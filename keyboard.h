#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_set_ru(int ru);
int keyboard_is_ru(void);
void keyboard_handle_scancode(uint8_t sc);
char keyboard_poll_char(void);
const char* keyboard_char_to_utf8(char c);

#endif
