#ifndef PAINT_APP_H
#define PAINT_APP_H

#include "gui.h"
#include "mouse.h"

void paint_init(void);
void paint_update(Window* win, MouseState* m, int prev_left);
void paint_render(Window* win, int lang);

#endif
