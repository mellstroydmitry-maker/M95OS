#include "craft_app.h"
#include <stdint.h>

// Воксельный мир 8x8x5 в RAM (0x00440000)
static uint8_t* world = (uint8_t*)0x00440000;
static int cur_x = 3, cur_y = 3;
static int selected_block = 1; // 1=Трава, 2=Земля, 3=Камень, 4=Алмаз, 5=Золото

static int idx3(int x, int y, int z) { return z * 64 + y * 8 + x; }

void craft_init(void) {
    for (int i = 0; i < 320; i++) *(world + i) = 0;
    // Базовый слой травы и небольшая горка
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            *(world + idx3(x, y, 0)) = 1;
        }
    }
    *(world + idx3(2, 2, 1)) = 3;
    *(world + idx3(2, 3, 1)) = 3;
    *(world + idx3(5, 5, 1)) = 4;
    cur_x = 3; cur_y = 3; selected_block = 1;
}

static void place_block(void) {
    for (int z = 0; z < 5; z++) {
        if (*(world + idx3(cur_x, cur_y, z)) == 0) {
            *(world + idx3(cur_x, cur_y, z)) = selected_block;
            return;
        }
    }
}

static void break_block(void) {
    for (int z = 4; z >= 1; z--) {
        if (*(world + idx3(cur_x, cur_y, z)) != 0) {
            *(world + idx3(cur_x, cur_y, z)) = 0;
            return;
        }
    }
}

void craft_input(char ch) {
    if ((ch == 'w' || (uint8_t)ch == 150) && cur_y > 0) cur_y--;
    else if ((ch == 's' || (uint8_t)ch == 155) && cur_y < 7) cur_y++;
    else if ((ch == 'a' || (uint8_t)ch == 148) && cur_x > 0) cur_x--;
    else if ((ch == 'd' || (uint8_t)ch == 130) && cur_x < 7) cur_x++;
    else if (ch == ' ') place_block();
    else if (ch == 'e' || (uint8_t)ch == 147) break_block();
    else if (ch == 'c' || (uint8_t)ch == 145) craft_init();
    else if (ch >= '1' && ch <= '5') selected_block = ch - '0';
}

void craft_click(Window* win, MouseState* m, int prev_left, int prev_right) {
    if (!win->visible) return;
    int cx = win->x + 12, cy = win->y + 34;

    if (m->left_button && !prev_left && m->y >= cy && m->y <= cy + 20) {
        for (int i = 0; i < 5; i++) {
            if (m->x >= cx + i * 32 && m->x <= cx + i * 32 + 28) { selected_block = i + 1; return; }
        }
        if (m->x >= cx + 170 && m->x <= cx + 225) { place_block(); return; }
        if (m->x >= cx + 230 && m->x <= cx + 285) { break_block(); return; }
    }
    // Клик по полю строит/ломает блок
    if (m->y > cy + 26 && m->y < win->y + win->h - 10 && m->x > cx && m->x < win->x + win->w - 12) {
        if (m->left_button && !prev_left) place_block();
        if (m->right_button && !prev_right) break_block();
    }
}

static void draw_iso_cube(int sx, int sy, uint32_t top_col, int highlight) {
    uint32_t left_col  = (top_col >> 1) & 0x007F7F7F;
    uint32_t right_col = ((top_col >> 2) & 0x003F3F3F) + left_col;

    // Верхняя грань (ромб 28x14)
    gui_fill_triangle(sx, sy - 7, sx + 14, sy, sx, sy + 7, top_col);
    gui_fill_triangle(sx, sy - 7, sx - 14, sy, sx, sy + 7, top_col);

    // Левая грань
    gui_fill_triangle(sx - 14, sy, sx, sy + 7, sx, sy + 21, left_col);
    gui_fill_triangle(sx - 14, sy, sx - 14, sy + 14, sx, sy + 21, left_col);

    // Правая грань
    gui_fill_triangle(sx, sy + 7, sx + 14, sy, sx + 14, sy + 14, right_col);
    gui_fill_triangle(sx, sy + 7, sx + 14, sy + 14, sx, sy + 21, right_col);

    uint32_t edge = highlight ? 0x00FFFFFF : 0x00181818;
    gui_draw_line(sx, sy - 7, sx + 14, sy, edge);
    gui_draw_line(sx + 14, sy, sx, sy + 7, edge);
    gui_draw_line(sx, sy + 7, sx - 14, sy, edge);
    gui_draw_line(sx - 14, sy, sx, sy - 7, edge);
}

void craft_render(Window* win, int lang) {
    int cx = win->x + 12, cy = win->y + 34;
    const uint32_t bcols[] = { 0, 0x0044CC44, 0x00996633, 0x009098A0, 0x0033CCFF, 0x00FFCC22 };

    // Панель выбора блоков
    for (int i = 1; i <= 5; i++) {
        int bx = cx + (i - 1) * 32;
        gui_draw_rect(bx, cy, 28, 20, *(bcols + i));
        gui_draw_bevel(bx, cy, 28, 20, selected_block == i);
    }
    gui_draw_button(cx + 170, cy, 55, 20, lang ? "+Блок" : "+Put", 0);
    gui_draw_button(cx + 230, cy, 55, 20, lang ? "-Слом" : "-Dig", 0);

    // Небо фона
    gui_draw_rect(cx, cy + 26, win->w - 24, win->h - 68, 0x0078B8F0);
    gui_draw_bevel(cx, cy + 26, win->w - 24, win->h - 68, 1);

    int origin_x = win->x + win->w / 2;
    int origin_y = cy + 95;

    for (int z = 0; z < 5; z++) {
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                uint8_t b = *(world + idx3(x, y, z));
                int is_sel = (x == cur_x && y == cur_y && z == 0);
                if (b != 0) {
                    int sx = origin_x + (x - y) * 14;
                    int sy = origin_y + (x + y) * 7 - z * 14;
                    draw_iso_cube(sx, sy, *(bcols + b), (x == cur_x && y == cur_y));
                } else if (is_sel) {
                    int sx = origin_x + (x - y) * 14;
                    int sy = origin_y + (x + y) * 7;
                    draw_iso_cube(sx, sy, 0x00FFFFFF, 1);
                }
            }
        }
    }
    gui_draw_text(cx, win->y + win->h - 22, "WASD: Move | Space: Build | E: Dig", WIN95_BLACK);
}
