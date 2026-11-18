#include "games_app.h"
#include <stdint.h>

static int current_game = 0; // 0=Сапер, 1=Змейка, 2=Арканоид, 3=Тетрис, 4=Понг
static uint32_t rng_seed = 54321;

static uint32_t next_rand(void) {
    rng_seed = rng_seed * 1103515245 + 12345;
    return (rng_seed >> 16) & 0x7FFF;
}

// Буферы игр в памяти (0x00410000+)
static uint8_t* mine_grid  = (uint8_t*)0x00410000;
static uint8_t* mine_state = (uint8_t*)0x00410100;
static int mine_over = 0;

static void mines_reset(void) {
    mine_over = 0;
    for (int i = 0; i < 81; i++) { *(mine_grid + i) = 0; *(mine_state + i) = 0; }
    int placed = 0;
    while (placed < 10) {
        int idx = next_rand() % 81;
        if (*(mine_grid + idx) != 9) { *(mine_grid + idx) = 9; placed++; }
    }
    for (int y = 0; y < 9; y++) for (int x = 0; x < 9; x++) {
        if (*(mine_grid + y * 9 + x) == 9) continue;
        int cnt = 0;
        for (int dy = -1; dy <= 1; dy++) for (int dx = -1; dx <= 1; dx++) {
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < 9 && ny >= 0 && ny < 9 && *(mine_grid + ny * 9 + nx) == 9) cnt++;
        }
        *(mine_grid + y * 9 + x) = cnt;
    }
}

static int* snake_x = (int*)0x00410200;
static int* snake_y = (int*)0x00410400;
static int snake_len = 4, snake_dx = 1, snake_dy = 0, food_x = 12, food_y = 7, snake_timer = 0, snake_over = 0;

static void snake_reset(void) {
    snake_len = 4; snake_dx = 1; snake_dy = 0; snake_over = 0;
    for (int i = 0; i < 4; i++) { *(snake_x + i) = 6 - i; *(snake_y + i) = 7; }
    food_x = 15; food_y = 7;
}

static uint8_t* bricks = (uint8_t*)0x00410600;
static int pad_x = 110, ball_x = 130, ball_y = 130, ball_dx = 2, ball_dy = -2;

static void breakout_reset(void) {
    pad_x = 110; ball_x = 130; ball_y = 130; ball_dx = 2; ball_dy = -2;
    for (int i = 0; i < 24; i++) *(bricks + i) = 1;
}

// --- ТЕТРИС (10x16) ---
static uint8_t* tet_board = (uint8_t*)0x00410700;
static int tet_x = 4, tet_y = 0, tet_type = 0, tet_rot = 0, tet_timer = 0, tet_over = 0;
static const uint16_t tet_shapes[] = {
    0x0F00, 0x2222, 0x0F00, 0x2222, // I
    0x0660, 0x0660, 0x0660, 0x0660, // O
    0x0E40, 0x4C40, 0x4E00, 0x4640, // T
    0x06C0, 0x8C40, 0x06C0, 0x8C40, // S
    0x0E80, 0xC440, 0x2E00, 0x4460  // L
};

static int tet_collide(int nx, int ny, int nrot) {
    uint16_t mask = *(tet_shapes + tet_type * 4 + (nrot & 3));
    for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) {
        if (mask & (0x8000 >> (r * 4 + c))) {
            int bx = nx + c, by = ny + r;
            if (bx < 0 || bx >= 10 || by >= 16) return 1;
            if (by >= 0 && *(tet_board + by * 10 + bx)) return 1;
        }
    }
    return 0;
}

static void tetris_reset(void) {
    tet_over = 0; tet_x = 3; tet_y = 0; tet_type = next_rand() % 5; tet_rot = 0;
    for (int i = 0; i < 160; i++) *(tet_board + i) = 0;
}

// --- ПИНГ-ПОНГ ---
static int pong_p1 = 65, pong_p2 = 65, pong_bx = 130, pong_by = 80, pong_vx = 3, pong_vy = 2;
static void pong_reset(void) { pong_p1 = 65; pong_p2 = 65; pong_bx = 130; pong_by = 80; pong_vx = 3; pong_vy = 2; }

void games_init(void) { mines_reset(); snake_reset(); breakout_reset(); tetris_reset(); pong_reset(); }

void games_input(char ch) {
    if (current_game == 1) {
        if ((ch == 'w' || (uint8_t)ch == 150) && snake_dy == 0) { snake_dx = 0; snake_dy = -1; }
        else if ((ch == 's' || (uint8_t)ch == 155) && snake_dy == 0) { snake_dx = 0; snake_dy = 1; }
        else if ((ch == 'a' || (uint8_t)ch == 148) && snake_dx == 0) { snake_dx = -1; snake_dy = 0; }
        else if ((ch == 'd' || (uint8_t)ch == 130) && snake_dx == 0) { snake_dx = 1; snake_dy = 0; }
        else if (ch == ' ') snake_reset();
    } else if (current_game == 2) {
        if (ch == 'a' || (uint8_t)ch == 148) { pad_x -= 20; if (pad_x < 0) pad_x = 0; }
        else if (ch == 'd' || (uint8_t)ch == 130) { pad_x += 20; if (pad_x > 210) pad_x = 210; }
        else if (ch == ' ') breakout_reset();
    } else if (current_game == 3) {
        if (tet_over) { if (ch == ' ') tetris_reset(); return; }
        if ((ch == 'a' || (uint8_t)ch == 148) && !tet_collide(tet_x - 1, tet_y, tet_rot)) tet_x--;
        else if ((ch == 'd' || (uint8_t)ch == 130) && !tet_collide(tet_x + 1, tet_y, tet_rot)) tet_x++;
        else if ((ch == 's' || (uint8_t)ch == 155) && !tet_collide(tet_x, tet_y + 1, tet_rot)) tet_y++;
        else if ((ch == 'w' || (uint8_t)ch == 150 || ch == ' ') && !tet_collide(tet_x, tet_y, tet_rot + 1)) tet_rot = (tet_rot + 1) & 3;
    } else if (current_game == 4) {
        if (ch == 'w' || (uint8_t)ch == 150) { pong_p1 -= 16; if (pong_p1 < 4) pong_p1 = 4; }
        else if (ch == 's' || (uint8_t)ch == 155) { pong_p1 += 16; if (pong_p1 > 125) pong_p1 = 125; }
    }
}
void games_click(Window* win, MouseState* m, int prev_left, int prev_right) {
    if (!win->visible) return;
    int cx = win->x + 12, cy = win->y + 34;

    if (m->left_button && !prev_left && m->y >= cy && m->y <= cy + 20) {
        int tab = (m->x - cx) / 56;
        if (tab >= 0 && tab < 5) { current_game = tab; return; }
    }
    if (m->left_button && !prev_left && m->x >= cx + 195 && m->x <= cx + 275 && m->y >= cy + 28 && m->y <= cy + 50) {
        if (current_game == 0) mines_reset(); else if (current_game == 1) snake_reset();
        else if (current_game == 2) breakout_reset(); else if (current_game == 3) tetris_reset(); else pong_reset();
        return;
    }
    if (current_game == 0 && !mine_over) {
        int gx = cx + 10, gy = cy + 28;
        if (m->x >= gx && m->x < gx + 180 && m->y >= gy && m->y < gy + 180) {
            int idx = ((m->y - gy) / 20) * 9 + ((m->x - gx) / 20);
            if (m->left_button && !prev_left && *(mine_state + idx) == 0) {
                *(mine_state + idx) = 1;
                if (*(mine_grid + idx) == 9) mine_over = 1;
            } else if (m->right_button && !prev_right && *(mine_state + idx) != 1) {
                *(mine_state + idx) = (*(mine_state + idx) == 2) ? 0 : 2;
            }
        }
    }
}

void games_update_and_render(Window* win, int lang) {
    rng_seed++;
    int cx = win->x + 12, cy = win->y + 34;

    gui_draw_button(cx,       cy, 54, 20, lang ? "Сапер" : "Mines", current_game == 0);
    gui_draw_button(cx + 56,  cy, 54, 20, lang ? "Змея"  : "Snake", current_game == 1);
    gui_draw_button(cx + 112, cy, 54, 20, lang ? "Блоки" : "Brick", current_game == 2);
    gui_draw_button(cx + 168, cy, 54, 20, lang ? "Тетр." : "Tetri", current_game == 3);
    gui_draw_button(cx + 224, cy, 54, 20, "Pong", current_game == 4);

    if (current_game == 0) {
        int gx = cx + 10, gy = cy + 28;
        for (int r = 0; r < 9; r++) for (int c = 0; c < 9; c++) {
            int idx = r * 9 + c, bx = gx + c * 20, by = gy + r * 20;
            if (*(mine_state + idx) != 1) {
                gui_draw_rect(bx, by, 19, 19, WIN95_GRAY); gui_draw_bevel(bx, by, 19, 19, 0);
                if (*(mine_state + idx) == 2) gui_draw_text(bx + 6, by + 3, "F", 0x00CC0000);
            } else {
                gui_draw_rect(bx, by, 19, 19, 0x00D8D8D8);
                int v = *(mine_grid + idx);
                if (v == 9) { gui_draw_rect(bx, by, 19, 19, 0x00CC2020); gui_draw_text(bx + 6, by + 3, "*", WIN95_WHITE); }
                else if (v > 0) { char ns[] = { (char)('0' + v), '\0' }; gui_draw_text(bx + 6, by + 3, ns, 0x000000CC); }
            }
        }
        gui_draw_button(cx + 195, cy + 28, 80, 22, lang ? "Заново" : "Reset", 0);
        gui_draw_text(cx + 196, cy + 60, mine_over ? "BOOM!" : "LMB/RMB", mine_over ? 0x00CC0000 : WIN95_BLACK);
    }
    else if (current_game == 1) {
        int fx = cx + 10, fy = cy + 28;
        gui_draw_rect(fx, fy, 260, 178, 0x00101810); gui_draw_bevel(fx, fy, 260, 178, 1);
        if (!snake_over && ++snake_timer >= 8) {
            snake_timer = 0;
            int nx = *(snake_x + 0) + snake_dx, ny = *(snake_y + 0) + snake_dy;
            if (nx < 0 || nx >= 25 || ny < 0 || ny >= 17) snake_over = 1;
            else {
                for (int i = snake_len - 1; i > 0; i--) { *(snake_x + i) = *(snake_x + i - 1); *(snake_y + i) = *(snake_y + i - 1); }
                *(snake_x + 0) = nx; *(snake_y + 0) = ny;
                if (nx == food_x && ny == food_y) { if (snake_len < 90) snake_len++; food_x = next_rand() % 24; food_y = next_rand() % 16; }
            }
        }
        gui_draw_rect(fx + 4 + food_x * 10, fy + 4 + food_y * 10, 9, 9, 0x00FF3333);
        for (int i = 0; i < snake_len; i++) gui_draw_rect(fx + 4 + *(snake_x + i) * 10, fy + 4 + *(snake_y + i) * 10, 9, 9, i == 0 ? 0x0088FF88 : 0x0033CC33);
        if (snake_over) gui_draw_text(fx + 70, fy + 80, "GAME OVER (Space)", 0x00FF5555);
    }
    else if (current_game == 2) {
        int fx = cx + 10, fy = cy + 28;
        gui_draw_rect(fx, fy, 260, 178, 0x00101422); gui_draw_bevel(fx, fy, 260, 178, 1);
        ball_x += ball_dx; ball_y += ball_dy;
        if (ball_x <= 2 || ball_x >= 252) ball_dx = -ball_dx;
        if (ball_y <= 2) ball_dy = -ball_dy;
        if (ball_y >= 160 && ball_x >= pad_x - 4 && ball_x <= pad_x + 48) ball_dy = -ball_dy;
        if (ball_y > 173) breakout_reset();
        const uint32_t bcols[] = { 0x00E04040, 0x00E0A030, 0x0040B050 };
        for (int r = 0; r < 3; r++) for (int c = 0; c < 8; c++) {
            int idx = r * 8 + c; if (!*(bricks + idx)) continue;
            int bx = 6 + c * 31, by = 8 + r * 14;
            gui_draw_rect(fx + bx, fy + by, 29, 11, *(bcols + r));
            if (ball_x >= bx && ball_x <= bx + 29 && ball_y >= by && ball_y <= by + 11) { *(bricks + idx) = 0; ball_dy = -ball_dy; }
        }
        gui_draw_rect(fx + pad_x, fy + 166, 46, 6, 0x0044AAFF);
        gui_draw_rect(fx + ball_x, fy + ball_y, 6, 6, 0x00FFFFFF);
    }
    else if (current_game == 3) { // ТЕТРИС
        int fx = cx + 40, fy = cy + 28;
        gui_draw_rect(fx, fy, 112, 178, 0x00101018); gui_draw_bevel(fx, fy, 112, 178, 1);
        if (!tet_over && ++tet_timer >= 14) {
            tet_timer = 0;
            if (!tet_collide(tet_x, tet_y + 1, tet_rot)) tet_y++;
            else {
                uint16_t mask = *(tet_shapes + tet_type * 4 + (tet_rot & 3));
                for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++)
                    if ((mask & (0x8000 >> (r * 4 + c))) && tet_y + r >= 0) *(tet_board + (tet_y + r) * 10 + tet_x + c) = tet_type + 1;
                for (int r = 15; r >= 0; r--) {
                    int full = 1;
                    for (int c = 0; c < 10; c++) if (!*(tet_board + r * 10 + c)) full = 0;
                    if (full) {
                        for (int rr = r; rr > 0; rr--) for (int c = 0; c < 10; c++) *(tet_board + rr * 10 + c) = *(tet_board + (rr - 1) * 10 + c);
                        r++;
                    }
                }
                tet_x = 3; tet_y = 0; tet_type = next_rand() % 5; tet_rot = 0;
                if (tet_collide(tet_x, tet_y, tet_rot)) tet_over = 1;
            }
        }
        const uint32_t tcols[] = { 0, 0x0000CCCC, 0x00CCCC00, 0x00AA00CC, 0x0000CC00, 0x00CC6600 };
        for (int r = 0; r < 16; r++) for (int c = 0; c < 10; c++) {
            uint8_t v = *(tet_board + r * 10 + c);
            if (v) gui_draw_rect(fx + 2 + c * 11, fy + 2 + r * 11, 10, 10, *(tcols + v));
        }
        uint16_t mask = *(tet_shapes + tet_type * 4 + (tet_rot & 3));
        for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++)
            if ((mask & (0x8000 >> (r * 4 + c))) && tet_y + r >= 0)
                gui_draw_rect(fx + 2 + (tet_x + c) * 11, fy + 2 + (tet_y + r) * 11, 10, 10, *(tcols + tet_type + 1));
        gui_draw_text(cx + 165, cy + 40, "A/D: Move\nW: Rotate\nS: Drop", WIN95_BLACK);
        if (tet_over) gui_draw_text(cx + 165, cy + 100, "OVER (Space)", 0x00CC0000);
    }
    else if (current_game == 4) { // ПИНГ-ПОНГ
        int fx = cx + 10, fy = cy + 28;
        gui_draw_rect(fx, fy, 260, 178, 0x00081810); gui_draw_bevel(fx, fy, 260, 178, 1);
        pong_bx += pong_vx; pong_by += pong_vy;
        if (pong_by <= 4 || pong_by >= 168) pong_vy = -pong_vy;
        if (pong_p2 + 20 < pong_by && pong_p2 < 128) pong_p2 += 2;
        else if (pong_p2 + 20 > pong_by && pong_p2 > 4) pong_p2 -= 2;
        if (pong_bx <= 16 && pong_by >= pong_p1 - 4 && pong_by <= pong_p1 + 42) pong_vx = -pong_vx;
        if (pong_bx >= 238 && pong_by >= pong_p2 - 4 && pong_by <= pong_p2 + 42) pong_vx = -pong_vx;
        if (pong_bx < 2 || pong_bx > 252) pong_reset();
        gui_draw_rect(fx + 8, fy + pong_p1, 6, 40, 0x0055FF55);
        gui_draw_rect(fx + 244, fy + pong_p2, 6, 40, 0x00FF5555);
        gui_draw_rect(fx + pong_bx, fy + pong_by, 6, 6, 0x00FFFFFF);
    }
}
