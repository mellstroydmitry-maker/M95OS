#ifndef M95PKG_H
#define M95PKG_H

#include "gui.h"
#include "mouse.h"
#include <stdint.h>

typedef struct {
    void (*draw_rect)(int x, int y, int w, int h, uint32_t color);
    void (*draw_text)(int x, int y, const char* str, uint32_t color);
    void (*draw_button)(int x, int y, int w, int h, const char* label, int pressed);
    void (*draw_line)(int x0, int y0, int x1, int y1, uint32_t color);
    void (*fill_triangle)(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
} M95ApiTable;

void pkg_init(void);
void pkg_load_disk_package(void* data, uint32_t size);
int  pkg_command(const char* cmd, char* out_buf, int* out_len, int max_len);
void pkg_click(Window* win, MouseState* m, int prev_left);
void pkg_render(Window* win, int lang);

int  pkg_app_is_running(void);
const char* pkg_app_title(void);
void pkg_app_render(Window* win, MouseState* m, int prev_left, char ch);

#endif
