#ifndef SETTINGS_APP_H
#define SETTINGS_APP_H

#include "gui.h"
#include "mouse.h"

void settings_init(ThemeConfig* cfg, int lang);
void settings_handle_click(ThemeConfig* cfg, Window* win, MouseState* m, int prev_left);
void settings_render(ThemeConfig* cfg, Window* win);

#endif
