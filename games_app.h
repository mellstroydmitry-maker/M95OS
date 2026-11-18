#ifndef GAMES_APP_H
#define GAMES_APP_H

#include "gui.h"
#include "mouse.h"

void games_init(void);
void games_input(char ch);
void games_click(Window* win, MouseState* m, int prev_left, int prev_right);
void games_update_and_render(Window* win, int lang);

#endif
