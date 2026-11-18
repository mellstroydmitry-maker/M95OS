#include "gui.h"
#include "font98.h"

static uint32_t* hw_fb;
static uint32_t hw_pitch;
static uint32_t* backbuffer = (uint32_t*)0x00200000;

static const uint8_t cursor_flat[] = {
    1,0,0,0,0,0,0,0,0,0,0,0, 1,1,0,0,0,0,0,0,0,0,0,0, 1,2,1,0,0,0,0,0,0,0,0,0,
    1,2,2,1,0,0,0,0,0,0,0,0, 1,2,2,2,1,0,0,0,0,0,0,0, 1,2,2,2,2,1,0,0,0,0,0,0,
    1,2,2,2,2,2,1,0,0,0,0,0, 1,2,2,2,2,2,2,1,0,0,0,0, 1,2,2,2,2,2,2,2,1,0,0,0,
    1,2,2,2,2,2,2,2,2,1,0,0, 1,2,2,2,2,2,1,1,1,1,1,0, 1,2,2,1,2,2,1,0,0,0,0,0,
    1,2,1,0,1,2,2,1,0,0,0,0, 1,1,0,0,1,2,2,1,0,0,0,0, 1,0,0,0,0,1,2,2,1,0,0,0,
    0,0,0,0,0,1,2,2,1,0,0,0, 0,0,0,0,0,0,1,2,1,0,0,0, 0,0,0,0,0,0,1,1,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0
};

void gui_init(uint32_t* framebuffer, uint32_t pitch) { hw_fb = framebuffer; hw_pitch = pitch; }

void gui_put_pixel(int x, int y, uint32_t color) {
    if ((uint32_t)x < SCREEN_W && (uint32_t)y < SCREEN_H) *(backbuffer + y * SCREEN_W + x) = color;
}

void gui_draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = (x + w > SCREEN_W) ? SCREEN_W : (x + w);
    int y1 = (y + h > SCREEN_H) ? SCREEN_H : (y + h);
    for (int row = y0; row < y1; row++) {
        uint32_t* dst = backbuffer + row * SCREEN_W + x0;
        for (int col = x0; col < x1; col++) *dst++ = color;
    }
}

void gui_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
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
    int th = y2 - y0; if (th == 0) return;
    for (int i = 0; i < th; i++) {
        int sh = (i > y1 - y0) || (y1 == y0), seg = sh ? (y2 - y1) : (y1 - y0);
        if (seg == 0) continue;
        int ax = x0 + ((x2 - x0) * i) / th;
        int bx = sh ? (x1 + ((x2 - x1) * (i - (y1 - y0))) / seg) : (x0 + ((x1 - x0) * i) / seg);
        if (ax > bx) { tmp = ax; ax = bx; bx = tmp; }
        gui_draw_rect(ax, y0 + i, bx - ax + 1, 1, color);
    }
}

static void gui_draw_glyph(int x, int y, int glyph_idx, uint32_t color) {
    if (glyph_idx < 0 || glyph_idx > 160) glyph_idx = 0;
    const uint8_t* glyph = font98_8x14 + glyph_idx * 14;
    for (int row = 0; row < 14; row++) {
        uint8_t line = *(glyph + row);
        for (int col = 0; col < 8; col++) if (line & (0x80 >> col)) gui_put_pixel(x + col, y + row, color);
    }
}

static int next_utf8_glyph(const char** str_ptr) {
    const uint8_t* s = (const uint8_t*)(*str_ptr);
    uint8_t b0 = *s;
    if (b0 == 0) return -1;
    if (b0 < 0x80) { (*str_ptr)++; return (b0 < 32 || b0 > 126) ? 0 : (b0 - 32); }
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
    (*str_ptr)++; return 0;
}

int gui_text_width(const char* str) {
    int count = 0; const char* p = str;
    while (*p) { if (*p == '\n') break; next_utf8_glyph(&p); count++; }
    return count * 8;
}

void gui_draw_text(int x, int y, const char* str, uint32_t color) {
    int cur_x = x; const char* p = str;
    while (*p) {
        if (*p == '\n') { cur_x = x; y += 15; p++; }
        else { int g = next_utf8_glyph(&p); if (g >= 0) { gui_draw_glyph(cur_x, y, g, color); cur_x += 8; } }
    }
}

void gui_draw_text_box(int x, int y, int max_w, int max_h, const char* str, uint32_t color) {
    int cur_x = x, cur_y = y; const char* p = str;
    while (*p) {
        if (cur_y + 14 > y + max_h) break;
        if (*p == '\n') { cur_x = x; cur_y += 15; p++; continue; }
        if (cur_x + 8 > x + max_w) { cur_x = x; cur_y += 15; if (cur_y + 14 > y + max_h) break; }
        int g = next_utf8_glyph(&p);
        if (g >= 0) { gui_draw_glyph(cur_x, cur_y, g, color); cur_x += 8; }
    }
}
void gui_draw_bevel(int x, int y, int w, int h, int sunken) {
    uint32_t tlo = sunken ? WIN95_DARK_GRAY : WIN95_LIGHT, tli = sunken ? WIN95_BLACK : WIN95_WHITE;
    uint32_t bro = sunken ? WIN95_WHITE : WIN95_BLACK, bri = sunken ? WIN95_LIGHT : WIN95_DARK_GRAY;
    gui_draw_rect(x, y, w, 1, tlo); gui_draw_rect(x, y, 1, h, tlo);
    gui_draw_rect(x, y + h - 1, w, 1, bro); gui_draw_rect(x + w - 1, y, 1, h, bro);
    gui_draw_rect(x + 1, y + 1, w - 2, 1, tli); gui_draw_rect(x + 1, y + 1, 1, h - 2, tli);
    gui_draw_rect(x + 1, y + h - 2, w - 2, 1, bri); gui_draw_rect(x + w - 2, y + 1, 1, h - 2, bri);
}

void gui_draw_button(int x, int y, int w, int h, const char* label, int pressed) {
    gui_draw_rect(x, y, w, h, pressed ? WIN95_LIGHT : WIN95_GRAY);
    gui_draw_bevel(x, y, w, h, pressed ? 1 : 0);
    int tw = gui_text_width(label);
    if (tw > w - 4) tw = w - 4;
    int tx = x + (w - tw) / 2 + (pressed ? 1 : 0);
    int ty = y + (h - 14) / 2 + (pressed ? 1 : 0);
    gui_draw_text_box(tx, ty, w - 4, 15, label, WIN95_BLACK);
}

void gui_draw_icon(int x, int y, int type, const char* label, int selected, int shadow) {
    int ix = x + 24, iy = y;
    if (type == 0) {
        gui_draw_rect(ix + 4, iy + 2, 24, 18, WIN95_GRAY); gui_draw_bevel(ix + 4, iy + 2, 24, 18, 0);
        gui_draw_rect(ix + 7, iy + 5, 18, 12, 0x00008080);
        gui_draw_rect(ix + 2, iy + 21, 28, 8, WIN95_GRAY); gui_draw_bevel(ix + 2, iy + 21, 28, 8, 0);
        gui_draw_rect(ix + 20, iy + 24, 6, 2, 0x0000AA00);
    } else if (type == 1) {
        gui_draw_rect(ix + 4, iy + 4, 24, 24, 0x00182030); gui_draw_bevel(ix + 4, iy + 4, 24, 24, 1);
        gui_fill_triangle(ix + 16, iy + 7, ix + 24, iy + 13, ix + 16, iy + 18, 0x0000CC77);
        gui_fill_triangle(ix + 16, iy + 7, ix + 8, iy + 13, ix + 16, iy + 18, 0x000088EE);
        gui_fill_triangle(ix + 8, iy + 13, ix + 16, iy + 18, ix + 16, iy + 25, 0x00EE9900);
    } else if (type == 2) {
        gui_draw_rect(ix + 6, iy + 3, 20, 26, WIN95_WHITE); gui_draw_bevel(ix + 6, iy + 3, 20, 26, 0);
        gui_draw_rect(ix + 7, iy + 4, 18, 5, 0x000040A0);
        gui_draw_line(ix + 9, iy + 12, ix + 22, iy + 12, WIN95_DARK_GRAY);
        gui_draw_line(ix + 9, iy + 16, ix + 20, iy + 16, WIN95_DARK_GRAY);
    } else if (type == 3) {
        gui_draw_rect(ix + 3, iy + 4, 26, 24, WIN95_GRAY); gui_draw_bevel(ix + 3, iy + 4, 26, 24, 0);
        gui_draw_rect(ix + 5, iy + 6, 22, 4, WIN95_TITLE_ACTIVE);
        gui_draw_rect(ix + 5, iy + 10, 22, 16, 0x00080808);
        gui_draw_text(ix + 7, iy + 11, ">_", 0x0055FF55);
    } else if (type == 4) {
        gui_draw_rect(ix + 3, iy + 3, 26, 26, WIN95_GRAY); gui_draw_bevel(ix + 3, iy + 3, 26, 26, 0);
        gui_draw_rect(ix + 7, iy + 8, 18, 3, WIN95_DARK_GRAY); gui_draw_rect(ix + 11, iy + 6, 5, 7, 0x000040A0);
        gui_draw_rect(ix + 7, iy + 19, 18, 3, WIN95_DARK_GRAY); gui_draw_rect(ix + 17, iy + 17, 5, 7, 0x00C03030);
    } else if (type == 5) {
        gui_draw_rect(ix + 3, iy + 5, 26, 22, 0x00202028); gui_draw_bevel(ix + 3, iy + 5, 26, 22, 0);
        gui_draw_rect(ix + 8, iy + 13, 16, 4, 0x00FF4444); gui_draw_rect(ix + 14, iy + 9, 4, 12, 0x00FF4444);
        gui_draw_rect(ix + 21, iy + 12, 3, 3, 0x00FFCC00);
    } else if (type == 6) {
        gui_draw_rect(ix + 4, iy + 4, 24, 24, WIN95_WHITE); gui_draw_bevel(ix + 4, iy + 4, 24, 24, 0);
        gui_draw_rect(ix + 8, iy + 8, 6, 6, 0x00EE2222); gui_draw_rect(ix + 16, iy + 8, 6, 6, 0x0022AA22);
        gui_draw_rect(ix + 8, iy + 16, 6, 6, 0x002266EE); gui_draw_rect(ix + 16, iy + 16, 6, 6, 0x00EEBB11);
    } else if (type == 7) {
        gui_draw_rect(ix + 4, iy + 4, 24, 24, 0x008B5A2B); gui_draw_bevel(ix + 4, iy + 4, 24, 24, 0);
        gui_draw_rect(ix + 5, iy + 5, 22, 7, 0x0033BB33);
    }
    int w_px = gui_text_width(label);
    int tx = x + 40 - (w_px / 2);
    if (selected) gui_draw_rect(tx - 2, iy + 33, w_px + 4, 15, WIN95_TITLE_ACTIVE);
    else if (shadow) gui_draw_text(tx + 1, iy + 35, label, 0x00080808);
    gui_draw_text(tx, iy + 34, label, WIN95_WHITE);
}

void gui_draw_window(Window* win, int is_active, ThemeConfig* cfg) {
    if (!win->visible) return;
    gui_draw_rect(win->x, win->y, win->w, win->h, cfg->window_frame_col);
    gui_draw_bevel(win->x, win->y, win->w, win->h, 0);
    int title_x = win->x + 4, title_y = win->y + 4, title_w = win->w - 8, title_h = 20;
    gui_draw_rect(title_x, title_y, title_w, title_h, is_active ? cfg->title_color : WIN95_TITLE_INACTIVE);
    if (win->title) gui_draw_text_box(title_x + 6, title_y + 3, title_w - 28, 15, win->title, WIN95_WHITE);
    int btn_x = title_x + title_w - 18, btn_y = title_y + 2;
    gui_draw_rect(btn_x, btn_y, 16, 16, WIN95_GRAY); gui_draw_bevel(btn_x, btn_y, 16, 16, 0);
    for (int i = 0; i < 8; i++) { gui_put_pixel(btn_x + 4 + i, btn_y + 4 + i, WIN95_BLACK); gui_put_pixel(btn_x + 11 - i, btn_y + 4 + i, WIN95_BLACK); }
    int cx = win->x + 6, cy = win->y + 28, cw = win->w - 12, ch = win->h - 34;
    uint32_t body_col = (win->bg_color == WIN95_GRAY) ? cfg->window_frame_col : win->bg_color;
    gui_draw_rect(cx, cy, cw, ch, body_col);
    gui_draw_bevel(cx, cy, cw, ch, 1);
}

void gui_draw_start_menu(int open, ThemeConfig* cfg) {
    if (!open) return;
    int mx = 4, my = SCREEN_H - 28 - 244, mw = 185, mh = 244;
    gui_draw_rect(mx, my, mw, mh, cfg->start_menu_bg);
    gui_draw_bevel(mx, my, mw, mh, 0);
    gui_draw_rect(mx + 3, my + 3, 20, mh - 6, cfg->start_banner_col);

    uint32_t txt_col = (cfg->start_menu_bg == 0x0024262E) ? WIN95_WHITE : WIN95_BLACK;
    int ix = mx + 28;
    if (cfg->lang == 1) {
        gui_draw_text(ix, my + 10,  "Мой компьютер", txt_col);
        gui_draw_text(ix, my + 36,  "TinyGL 3D Студия", txt_col);
        gui_draw_text(ix, my + 62,  "Блокнот", txt_col);
        gui_draw_text(ix, my + 88,  "Терминал M95", txt_col);
        gui_draw_text(ix, my + 114, "Настройки", txt_col);
        gui_draw_text(ix, my + 140, "Игры / Paint / Craft", txt_col);
        gui_draw_text(ix, my + 166, "Запустить заставку", txt_col);
        gui_draw_text(ix, my + 206, "Перезагрузка", txt_col);
    } else {
        gui_draw_text(ix, my + 10,  "My Computer", txt_col);
        gui_draw_text(ix, my + 36,  "TinyGL 3D Studio", txt_col);
        gui_draw_text(ix, my + 62,  "Notepad", txt_col);
        gui_draw_text(ix, my + 88,  "M95 Shell", txt_col);
        gui_draw_text(ix, my + 114, "Settings", txt_col);
        gui_draw_text(ix, my + 140, "Games / Paint / Craft", txt_col);
        gui_draw_text(ix, my + 166, "Start Screensaver", txt_col);
        gui_draw_text(ix, my + 206, "Restart PC", txt_col);
    }
    gui_draw_rect(ix - 2, my + 192, mw - 34, 1, WIN95_DARK_GRAY);
    gui_draw_rect(ix - 2, my + 193, mw - 34, 1, WIN95_WHITE);
}

void gui_draw_taskbar(int start_open, ThemeConfig* cfg, int kbd_lang, const char* clock_str) {
    int bar_h = 28, bar_y = SCREEN_H - bar_h;
    gui_draw_rect(0, bar_y, SCREEN_W, bar_h, cfg->start_menu_bg);
    gui_draw_rect(0, bar_y, SCREEN_W, 1, WIN95_LIGHT);
    gui_draw_rect(0, bar_y + 1, SCREEN_W, 1, WIN95_WHITE);

    int start_x = 4, start_y = bar_y + 4, start_w = 68, start_h = 20;
    gui_draw_rect(start_x, start_y, start_w, start_h, WIN95_GRAY);
    gui_draw_bevel(start_x, start_y, start_w, start_h, start_open ? 1 : 0);
    gui_draw_rect(start_x + 6, start_y + 5, 4, 4, 0x00C03030);
    gui_draw_rect(start_x + 11, start_y + 5, 4, 4, 0x0030A030);
    gui_draw_rect(start_x + 6, start_y + 10, 4, 4, 0x003050C0);
    gui_draw_rect(start_x + 11, start_y + 10, 4, 4, 0x00D0B030);
    gui_draw_text(start_x + 20, start_y + 3, cfg->lang ? "Пуск" : "Start", WIN95_BLACK);

    int tray_w = cfg->show_clock ? 96 : 38;
    int tray_x = SCREEN_W - tray_w - 4;
    gui_draw_rect(tray_x, start_y, tray_w, start_h, WIN95_GRAY);
    gui_draw_bevel(tray_x, start_y, tray_w, start_h, 1);
    gui_draw_rect(tray_x + 4, start_y + 2, 24, 16, cfg->start_banner_col);
    gui_draw_text(tray_x + 8, start_y + 3, kbd_lang ? "RU" : "EN", WIN95_WHITE);
    if (cfg->show_clock) gui_draw_text(tray_x + 34, start_y + 3, clock_str, WIN95_BLACK);
}

void gui_draw_cursor(int x, int y, int theme) {
    uint32_t fill = (theme == 1) ? 0x0044FF55 : ((theme == 2) ? 0x00FFCC22 : ((theme == 3) ? 0x0033DDFF : 0x00FFFFFF));
    for (int row = 0; row < 19; row++) for (int col = 0; col < 12; col++) {
        uint8_t pixel = *(cursor_flat + row * 12 + col);
        if (pixel == 1) gui_put_pixel(x + col, y + row, 0x00000000);
        else if (pixel == 2) gui_put_pixel(x + col, y + row, fill);
    }
}

void gui_swap_buffers(void) {
    uint32_t pitch_in_pixels = hw_pitch / 4;
    for (int y = 0; y < SCREEN_H; y++) {
        uint32_t* dst = hw_fb + y * pitch_in_pixels;
        uint32_t* src = backbuffer + y * SCREEN_W;
        for (int x = 0; x < SCREEN_W; x++) *dst++ = *src++;
    }
}
