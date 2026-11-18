#ifndef CRAFT_APP_H
#define CRAFT_APP_H

#include "gui.h"
#include "mouse.h"

void craft_init(void);
void craft_input(char ch);
void craft_click(Window* win, MouseState* m, int prev_left, int prev_right);
void craft_render(Window* win, int lang);

#endif
