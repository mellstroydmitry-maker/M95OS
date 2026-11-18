#ifndef GUI_H
#define GUI_H

#include <stdint.h>

#define SCREEN_W 800
#define SCREEN_H 600

#define WIN95_DESKTOP_BG     0x002B6565
#define WIN95_GRAY           0x00C0C0C0
#define WIN95_LIGHT          0x00DFDFDF
#define WIN95_WHITE          0x00FFFFFF
#define WIN95_DARK_GRAY      0x00808080
#define WIN95_BLACK          0x00101010
#define WIN95_TITLE_ACTIVE   0x00000080
#define WIN95_TITLE_INACTIVE 0x00808080

typedef struct {
    int id;
    int x, y, w, h;
    const char* title;
    int visible;
    int dragging;
    int drag_offset_x;
    int drag_offset_y;
    uint32_t bg_color;
} Window;

void gui_init(uint32_t* framebuffer, uint32_t pitch);
void gui_put_pixel(int x, int y, uint32_t color);
void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
void gui_draw_line(int x0, int y0, int x1, int y1, uint32_t color);
void gui_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
void gui_draw_glyph(int x, int y, int glyph_idx, uint32_t color);
void gui_draw_text(int x, int y, const char* str, uint32_t color);
int  gui_text_width(const char* str);
void gui_draw_bevel(int x, int y, int w, int h, int sunken);
void gui_draw_window(Window* win, int is_active);
void gui_draw_icon(int x, int y, int type, const char* label, int selected);
void gui_draw_start_menu(int open, int lang);
void gui_draw_taskbar(int start_open, int lang, int kbd_lang, const char* clock_str);
void gui_draw_cursor(int x, int y);
void gui_swap_buffers(void);

#endif
