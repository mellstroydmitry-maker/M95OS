#include "gui.h"
#include "font98.h"

static uint32_t* hw_fb;
static uint32_t hw_pitch;
static uint32_t* backbuffer = (uint32_t*)0x00200000;

static const uint8_t cursor_flat[] = {
    1,0,0,0,0,0,0,0,0,0,0,0,
    1,1,0,0,0,0,0,0,0,0,0,0,
    1,2,1,0,0,0,0,0,0,0,0,0,
    1,2,2,1,0,0,0,0,0,0,0,0,
    1,2,2,2,1,0,0,0,0,0,0,0,
    1,2,2,2,2,1,0,0,0,0,0,0,
    1,2,2,2,2,2,1,0,0,0,0,0,
    1,2,2,2,2,2,2,1,0,0,0,0,
    1,2,2,2,2,2,2,2,1,0,0,0,
    1,2,2,2,2,2,2,2,2,1,0,0,
    1,2,2,2,2,2,1,1,1,1,1,0,
    1,2,2,1,2,2,1,0,0,0,0,0,
    1,2,1,0,1,2,2,1,0,0,0,0,
    1,1,0,0,1,2,2,1,0,0,0,0,
    1,0,0,0,0,1,2,2,1,0,0,0,
    0,0,0,0,0,1,2,2,1,0,0,0,
    0,0,0,0,0,0,1,2,1,0,0,0,
    0,0,0,0,0,0,1,1,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0
};

void gui_init(uint32_t* framebuffer, uint32_t pitch) {
    hw_fb = framebuffer;
    hw_pitch = pitch;
}

void gui_put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        *(backbuffer + y * SCREEN_W + x) = color;
    }
}

void gui_draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int i = y; i < y + h; i++) {
        for (int j = x; j < x + w; j++) {
            gui_put_pixel(j, i, color);
        }
    }
}

void gui_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        gui_put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gui_fill_triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    int tmp;
    if (y0 > y1) { tmp = y0; y0 = y1; y1 = tmp; tmp = x0; x0 = x1; x1 = tmp; }
    if (y0 > y2) { tmp = y0; y0 = y2; y2 = tmp; tmp = x0; x0 = x2; x2 = tmp; }
    if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; tmp = x1; x1 = x2; x2 = tmp; }
    int total_height = y2 - y0;
    if (total_height == 0) return;
    for (int i = 0; i < total_height; i++) {
        int second_half = (i > y1 - y0) || (y1 == y0);
        int segment_height = second_half ? (y2 - y1) : (y1 - y0);
        if (segment_height == 0) continue;
        int ax = x0 + ((x2 - x0) * i) / total_height;
        int bx = second_half
            ? (x1 + ((x2 - x1) * (i - (y1 - y0))) / segment_height)
            : (x0 + ((x1 - x0) * i) / segment_height);
        if (ax > bx) { tmp = ax; ax = bx; bx = tmp; }
        for (int j = ax; j <= bx; j++) {
            gui_put_pixel(j, y0 + i, color);
        }
    }
}
void gui_draw_glyph(int x, int y, int glyph_idx, uint32_t color) {
    if (glyph_idx < 0 || glyph_idx > 160) glyph_idx = 0;
    const uint8_t* glyph = font98_8x14 + glyph_idx * 14;
    for (int row = 0; row < 14; row++) {
        uint8_t line = *(glyph + row);
        for (int col = 0; col < 8; col++) {
            if (line & (0x80 >> col)) {
                gui_put_pixel(x + col, y + row, color);
            }
        }
    }
}

static int next_utf8_glyph(const char** str_ptr) {
    const uint8_t* s = (const uint8_t*)(*str_ptr);
    uint8_t b0 = *s;
    if (b0 == 0) return -1;
    if (b0 < 0x80) {
        (*str_ptr)++;
        if (b0 < 32 || b0 > 126) return 0;
        return b0 - 32;
    }
    uint8_t b1 = *(s + 1);
    if ((b0 == 0xD0 || b0 == 0xD1) && b1 != 0) {
        (*str_ptr) += 2;
        uint16_t cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        if (cp >= 0x0410 && cp <= 0x042F) return 95 + (cp - 0x0410);
        if (cp >= 0x0430 && cp <= 0x044F) return 127 + (cp - 0x0430);
        if (cp == 0x0401) return 159;
        if (cp == 0x0451) return 160;
        return 0;
    }
    (*str_ptr)++;
    return 0;
}

int gui_text_width(const char* str) {
    int count = 0;
    const char* p = str;
    while (*p) {
        if (*p == '\n') break;
        next_utf8_glyph(&p);
        count++;
    }
    return count * 8;
}

void gui_draw_text(int x, int y, const char* str, uint32_t color) {
    int cur_x = x;
    const char* p = str;
    while (*p) {
        if (*p == '\n') {
            cur_x = x;
            y += 15;
            p++;
        } else {
            int g = next_utf8_glyph(&p);
            if (g >= 0) {
                gui_draw_glyph(cur_x, y, g, color);
                cur_x += 8;
            }
        }
    }
}

void gui_draw_bevel(int x, int y, int w, int h, int sunken) {
    uint32_t top_left_outer = sunken ? WIN95_DARK_GRAY : WIN95_LIGHT;
    uint32_t top_left_inner = sunken ? WIN95_BLACK     : WIN95_WHITE;
    uint32_t bot_right_outer = sunken ? WIN95_WHITE    : WIN95_BLACK;
    uint32_t bot_right_inner = sunken ? WIN95_LIGHT    : WIN95_DARK_GRAY;

    gui_draw_rect(x, y, w, 1, top_left_outer);
    gui_draw_rect(x, y, 1, h, top_left_outer);
    gui_draw_rect(x, y + h - 1, w, 1, bot_right_outer);
    gui_draw_rect(x + w - 1, y, 1, h, bot_right_outer);

    gui_draw_rect(x + 1, y + 1, w - 2, 1, top_left_inner);
    gui_draw_rect(x + 1, y + 1, 1, h - 2, top_left_inner);
    gui_draw_rect(x + 1, y + h - 2, w - 2, 1, bot_right_inner);
    gui_draw_rect(x + w - 2, y + 1, 1, h - 2, bot_right_inner);
}

void gui_draw_icon(int x, int y, int type, const char* label, int selected) {
    int ix = x + 24;
    int iy = y;
    if (type == 0) {
        gui_draw_rect(ix + 4, iy + 2, 24, 18, WIN95_GRAY);
        gui_draw_bevel(ix + 4, iy + 2, 24, 18, 0);
        gui_draw_rect(ix + 7, iy + 5, 18, 12, 0x00008080);
        gui_draw_rect(ix + 2, iy + 21, 28, 8, WIN95_GRAY);
        gui_draw_bevel(ix + 2, iy + 21, 28, 8, 0);
        gui_draw_rect(ix + 20, iy + 24, 6, 2, 0x0000AA00);
    } else if (type == 1) {
        gui_draw_rect(ix + 4, iy + 4, 24, 24, 0x00182030);
        gui_draw_bevel(ix + 4, iy + 4, 24, 24, 1);
        gui_fill_triangle(ix + 16, iy + 7, ix + 24, iy + 13, ix + 16, iy + 18, 0x0000CC77);
        gui_fill_triangle(ix + 16, iy + 7, ix + 8, iy + 13, ix + 16, iy + 18, 0x000088EE);
        gui_fill_triangle(ix + 8, iy + 13, ix + 16, iy + 18, ix + 16, iy + 25, 0x00EE9900);
    } else if (type == 2) {
        gui_draw_rect(ix + 6, iy + 3, 20, 26, WIN95_WHITE);
        gui_draw_bevel(ix + 6, iy + 3, 20, 26, 0);
        gui_draw_rect(ix + 7, iy + 4, 18, 5, 0x000040A0);
        gui_draw_line(ix + 9, iy + 12, ix + 22, iy + 12, WIN95_DARK_GRAY);
        gui_draw_line(ix + 9, iy + 16, ix + 20, iy + 16, WIN95_DARK_GRAY);
        gui_draw_line(ix + 9, iy + 20, ix + 22, iy + 20, WIN95_DARK_GRAY);
    } else if (type == 3) {
        gui_draw_rect(ix + 3, iy + 4, 26, 24, WIN95_GRAY);
        gui_draw_bevel(ix + 3, iy + 4, 26, 24, 0);
        gui_draw_rect(ix + 5, iy + 6, 22, 4, WIN95_TITLE_ACTIVE);
        gui_draw_rect(ix + 5, iy + 10, 22, 16, 0x00080808);
        gui_draw_text(ix + 7, iy + 11, ">_", 0x0055FF55);
    }
    int w_px = gui_text_width(label);
    int tx = x + 40 - (w_px / 2);
    if (selected) {
        gui_draw_rect(tx - 2, iy + 33, w_px + 4, 15, WIN95_TITLE_ACTIVE);
    }
    gui_draw_text(tx, iy + 34, label, WIN95_WHITE);
}

void gui_draw_window(Window* win, int is_active) {
    if (!win->visible) return;
    gui_draw_rect(win->x, win->y, win->w, win->h, WIN95_GRAY);
    gui_draw_bevel(win->x, win->y, win->w, win->h, 0);
    int title_x = win->x + 4;
    int title_y = win->y + 4;
    int title_w = win->w - 8;
    int title_h = 20;
    uint32_t bar_col = is_active ? WIN95_TITLE_ACTIVE : WIN95_TITLE_INACTIVE;
    gui_draw_rect(title_x, title_y, title_w, title_h, bar_col);
    if (win->title) {
        gui_draw_text(title_x + 6, title_y + 3, win->title, WIN95_WHITE);
    }
    int btn_size = 16;
    int btn_x = title_x + title_w - btn_size - 2;
    int btn_y = title_y + 2;
    gui_draw_rect(btn_x, btn_y, btn_size, btn_size, WIN95_GRAY);
    gui_draw_bevel(btn_x, btn_y, btn_size, btn_size, 0);
    for (int i = 0; i < 8; i++) {
        gui_put_pixel(btn_x + 4 + i, btn_y + 4 + i, WIN95_BLACK);
        gui_put_pixel(btn_x + 11 - i, btn_y + 4 + i, WIN95_BLACK);
    }
    int client_x = win->x + 6;
    int client_y = win->y + 28;
    int client_w = win->w - 12;
    int client_h = win->h - 34;
    gui_draw_rect(client_x, client_y, client_w, client_h, win->bg_color);
    gui_draw_bevel(client_x, client_y, client_w, client_h, 1);
}

void gui_draw_start_menu(int open, int lang) {
    if (!open) return;
    int mx = 4;
    int my = SCREEN_H - 28 - 166;
    int mw = 185;
    int mh = 166;
    gui_draw_rect(mx, my, mw, mh, WIN95_GRAY);
    gui_draw_bevel(mx, my, mw, mh, 0);
    gui_draw_rect(mx + 3, my + 3, 20, mh - 6, WIN95_TITLE_ACTIVE);
    int ix = mx + 28;
    if (lang == 1) {
        gui_draw_text(ix, my + 10,  "Мой компьютер", WIN95_BLACK);
        gui_draw_text(ix, my + 36,  "TinyGL 3D Студия", WIN95_BLACK);
        gui_draw_text(ix, my + 62,  "Блокнот", WIN95_BLACK);
        gui_draw_text(ix, my + 88,  "Терминал M95", WIN95_BLACK);
        gui_draw_text(ix, my + 128, "Перезагрузка", WIN95_BLACK);
    } else {
        gui_draw_text(ix, my + 10,  "My Computer", WIN95_BLACK);
        gui_draw_text(ix, my + 36,  "TinyGL 3D Studio", WIN95_BLACK);
        gui_draw_text(ix, my + 62,  "Notepad", WIN95_BLACK);
        gui_draw_text(ix, my + 88,  "M95 Shell", WIN95_BLACK);
        gui_draw_text(ix, my + 128, "Restart PC", WIN95_BLACK);
    }
    gui_draw_rect(ix - 2, my + 114, mw - 34, 1, WIN95_DARK_GRAY);
    gui_draw_rect(ix - 2, my + 115, mw - 34, 1, WIN95_WHITE);
}

void gui_draw_taskbar(int start_open, int lang, int kbd_lang, const char* clock_str) {
    int bar_h = 28;
    int bar_y = SCREEN_H - bar_h;
    gui_draw_rect(0, bar_y, SCREEN_W, bar_h, WIN95_GRAY);
    gui_draw_rect(0, bar_y, SCREEN_W, 1, WIN95_LIGHT);
    gui_draw_rect(0, bar_y + 1, SCREEN_W, 1, WIN95_WHITE);

    int start_x = 4;
    int start_y = bar_y + 4;
    int start_w = 68;
    int start_h = 20;
    gui_draw_rect(start_x, start_y, start_w, start_h, WIN95_GRAY);
    gui_draw_bevel(start_x, start_y, start_w, start_h, start_open ? 1 : 0);

    gui_draw_rect(start_x + 6, start_y + 5, 4, 4, 0x00C03030);
    gui_draw_rect(start_x + 11, start_y + 5, 4, 4, 0x0030A030);
    gui_draw_rect(start_x + 6, start_y + 10, 4, 4, 0x003050C0);
    gui_draw_rect(start_x + 11, start_y + 10, 4, 4, 0x00D0B030);
    gui_draw_text(start_x + 20, start_y + 3, (lang == 1) ? "Пуск" : "Start", WIN95_BLACK);

    int tray_w = 104;
    int tray_x = SCREEN_W - tray_w - 4;
    gui_draw_bevel(tray_x, start_y, tray_w, start_h, 1);
    gui_draw_rect(tray_x + 4, start_y + 2, 24, 16, WIN95_TITLE_ACTIVE);
    gui_draw_text(tray_x + 8, start_y + 3, kbd_lang ? "RU" : "EN", WIN95_WHITE);
    gui_draw_text(tray_x + 36, start_y + 3, clock_str, WIN95_BLACK);
}

void gui_draw_cursor(int x, int y) {
    for (int row = 0; row < 19; row++) {
        for (int col = 0; col < 12; col++) {
            uint8_t pixel = *(cursor_flat + row * 12 + col);
            if (pixel == 1) gui_put_pixel(x + col, y + row, 0x00000000);
            else if (pixel == 2) gui_put_pixel(x + col, y + row, 0x00FFFFFF);
        }
    }
}

void gui_swap_buffers(void) {
    uint32_t pitch_in_pixels = hw_pitch / 4;
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) {
            *(hw_fb + y * pitch_in_pixels + x) = *(backbuffer + y * SCREEN_W + x);
        }
    }
}
