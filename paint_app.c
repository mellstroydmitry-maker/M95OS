#include "paint_app.h"
#include <stdint.h>

#define CANVAS_W 280
#define CANVAS_H 165

static uint32_t* canvas = (uint32_t*)0x00420000;
static uint32_t cur_color = 0x00101010;
static int brush_size = 2;
static int is_eraser = 0;
static int last_px = -1, last_py = -1;

static void canvas_clear(uint32_t col) {
    for (int i = 0; i < CANVAS_W * CANVAS_H; i++) *(canvas + i) = col;
}

static void canvas_dot(int x, int y, int r, uint32_t col) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < CANVAS_W && ny >= 0 && ny < CANVAS_H) *(canvas + ny * CANVAS_W + nx) = col;
        }
    }
}

static void canvas_line(int x0, int y0, int x1, int y1, int r, uint32_t col) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        canvas_dot(x0, y0, r, col);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void paint_init(void) { canvas_clear(0x00FFFFFF); cur_color = 0x00101010; brush_size = 2; is_eraser = 0; }

void paint_update(Window* win, MouseState* m, int prev_left) {
    if (!win->visible) return;
    int cx = win->x + 12, cy = win->y + 34;
    const uint32_t pal[] = { 0x00101010, 0x00808080, 0x00CC0000, 0x00008000, 0x000000CC, 0x00CC8800,
                             0x00FFFFFF, 0x00C0C0C0, 0x00FF4444, 0x0033DD33, 0x003399FF, 0x00FFEE22 };
    if (m->left_button && !prev_left) {
        for (int i = 0; i < 12; i++) {
            int px = cx + (i % 6) * 20, py = cy + (i / 6) * 16;
            if (m->x >= px && m->x <= px + 18 && m->y >= py && m->y <= py + 14) { cur_color = *(pal + i); is_eraser = 0; return; }
        }
        if (m->y >= cy && m->y <= cy + 28) {
            if (m->x >= cx + 126 && m->x <= cx + 150) { brush_size = 1; is_eraser = 0; return; }
            if (m->x >= cx + 153 && m->x <= cx + 177) { brush_size = 3; is_eraser = 0; return; }
            if (m->x >= cx + 180 && m->x <= cx + 225) { is_eraser = 1; brush_size = 4; return; }
            if (m->x >= cx + 228 && m->x <= cx + 280) { canvas_clear(0x00FFFFFF); return; }
        }
    }
    int hx = cx, hy = cy + 36;
    if (m->left_button && m->x >= hx && m->x < hx + CANVAS_W && m->y >= hy && m->y < hy + CANVAS_H) {
        int px = m->x - hx, py = m->y - hy;
        uint32_t draw_col = is_eraser ? 0x00FFFFFF : cur_color;
        if (last_px != -1 && prev_left) canvas_line(last_px, last_py, px, py, brush_size, draw_col);
        else canvas_dot(px, py, brush_size, draw_col);
        last_px = px; last_py = py;
    } else { last_px = -1; last_py = -1; }
}

void paint_render(Window* win, int lang) {
    int cx = win->x + 12, cy = win->y + 34;
    const uint32_t pal[] = { 0x00101010, 0x00808080, 0x00CC0000, 0x00008000, 0x000000CC, 0x00CC8800,
                             0x00FFFFFF, 0x00C0C0C0, 0x00FF4444, 0x0033DD33, 0x003399FF, 0x00FFEE22 };
    for (int i = 0; i < 12; i++) {
        int px = cx + (i % 6) * 20, py = cy + (i / 6) * 16;
        gui_draw_rect(px, py, 18, 14, *(pal + i));
        gui_draw_bevel(px, py, 18, 14, (!is_eraser && cur_color == *(pal + i)) ? 1 : 0);
    }
    gui_draw_button(cx + 126, cy, 24, 28, "S", !is_eraser && brush_size == 1);
    gui_draw_button(cx + 153, cy, 24, 28, "L", !is_eraser && brush_size == 3);
    gui_draw_button(cx + 180, cy, 45, 28, lang ? "Ласт" : "Eras", is_eraser);
    gui_draw_button(cx + 228, cy, 52, 28, lang ? "Очист" : "Clear", 0);
    int hx = cx, hy = cy + 36;
    gui_draw_bevel(hx - 2, hy - 2, CANVAS_W + 4, CANVAS_H + 4, 1);
    for (int y = 0; y < CANVAS_H; y++) for (int x = 0; x < CANVAS_W; x++) gui_put_pixel(hx + x, hy + y, *(canvas + y * CANVAS_W + x));
}
