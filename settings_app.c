#include "settings_app.h"

extern void mouse_set_speed(int mult);
extern void keyboard_set_ru(int ru);

void settings_init(ThemeConfig* cfg, int lang) {
    cfg->desktop_color    = WIN95_DESKTOP_BG;
    cfg->title_color      = WIN95_TITLE_ACTIVE;
    cfg->start_menu_bg    = WIN95_GRAY;
    cfg->start_banner_col = WIN95_TITLE_ACTIVE;
    cfg->dark_windows     = 0;
    cfg->window_frame_col = WIN95_GRAY;
    cfg->flat_style       = 0;
    cfg->wallpaper_grad   = 0;
    cfg->cursor_theme     = 0;
    cfg->mouse_speed      = 1;
    cfg->icon_shadows     = 1;
    cfg->lang             = lang;
    cfg->screensaver_id   = 0;
    cfg->trigger_ss       = 0;
    cfg->enable_packages  = 0; // По умолчанию Experimental выключен!
    mouse_set_speed(1);
}

static int hit(int mx, int my, int x, int y, int w, int h) {
    return (mx >= x && mx <= x + w && my >= y && my <= y + h);
}

void settings_handle_click(ThemeConfig* cfg, Window* win, MouseState* m, int prev_left) {
    if (!win->visible || !m->left_button || prev_left) return;
    int cx = win->x + 14, cy = win->y + 34;

    const uint32_t bg_cols[] = { 0x002B6565, 0x001E427A, 0x0022242A, 0x001E5631, 0x00632626, 0x003B2454 };
    for (int i = 0; i < 6; i++) if (hit(m->x, m->y, cx + i * 32, cy + 16, 26, 20)) cfg->desktop_color = *(bg_cols + i);

    const uint32_t title_cols[] = { 0x00000080, 0x004B0082, 0x00006432, 0x008B0000 };
    for (int i = 0; i < 4; i++) if (hit(m->x, m->y, cx + 215 + i * 40, cy + 16, 34, 20)) cfg->title_color = *(title_cols + i);

    const uint32_t sm_bg[]  = { 0x00C0C0C0, 0x0024262E, 0x00D8D0B8, 0x00A8B8D0 };
    const uint32_t sm_ban[] = { 0x00000080, 0x00008855, 0x00804000, 0x00103070 };
    for (int i = 0; i < 4; i++) {
        if (hit(m->x, m->y, cx + i * 94, cy + 60, 88, 22)) {
            cfg->start_menu_bg = *(sm_bg + i);
            cfg->start_banner_col = *(sm_ban + i);
        }
    }

    for (int i = 0; i < 3; i++) if (hit(m->x, m->y, cx + i * 44, cy + 106, 40, 22)) { cfg->mouse_speed = i + 1; mouse_set_speed(cfg->mouse_speed); }
    for (int i = 0; i < 4; i++) if (hit(m->x, m->y, cx + 150 + i * 58, cy + 106, 54, 22)) cfg->cursor_theme = i;

    if (hit(m->x, m->y, cx,       cy + 148, 120, 24)) cfg->dark_windows = !cfg->dark_windows; cfg->window_frame_col = cfg->dark_windows ? 0x002D3038 : WIN95_GRAY;
    if (hit(m->x, m->y, cx + 128, cy + 148, 120, 24)) cfg->flat_style   = !cfg->flat_style;
    if (hit(m->x, m->y, cx + 256, cy + 148, 122, 24)) cfg->icon_shadows = !cfg->icon_shadows;

    // Кнопка Experimental: Пакетный менеджер + Язык + Сброс
    if (hit(m->x, m->y, cx,       cy + 186, 132, 24)) cfg->enable_packages = !cfg->enable_packages;
    if (hit(m->x, m->y, cx + 138, cy + 186, 118, 24)) { cfg->lang = !cfg->lang; keyboard_set_ru(cfg->lang); }
    if (hit(m->x, m->y, cx + 262, cy + 186, 116, 24)) settings_init(cfg, cfg->lang);
}

void settings_render(ThemeConfig* cfg, Window* win) {
    int cx = win->x + 14, cy = win->y + 34, ru = cfg->lang;
    uint32_t tc = cfg->dark_windows ? WIN95_WHITE : WIN95_BLACK;

    gui_draw_text(cx, cy, ru ? "1. Обои:" : "1. Wallpaper:", tc);
    const uint32_t bg_cols[] = { 0x002B6565, 0x001E427A, 0x0022242A, 0x001E5631, 0x00632626, 0x003B2454 };
    for (int i = 0; i < 6; i++) {
        gui_draw_rect(cx + i * 32, cy + 16, 26, 20, *(bg_cols + i));
        gui_draw_bevel(cx + i * 32, cy + 16, 26, 20, cfg->desktop_color == *(bg_cols + i));
    }

    gui_draw_text(cx + 215, cy, ru ? "2. Шапка:" : "2. Title:", tc);
    const uint32_t title_cols[] = { 0x00000080, 0x004B0082, 0x00006432, 0x008B0000 };
    for (int i = 0; i < 4; i++) {
        gui_draw_rect(cx + 215 + i * 40, cy + 16, 34, 20, *(title_cols + i));
        gui_draw_bevel(cx + 215 + i * 40, cy + 16, 34, 20, cfg->title_color == *(title_cols + i));
    }

    gui_draw_text(cx, cy + 42, ru ? "3. Меню «Пуск» и панель:" : "3. Start & Taskbar:", tc);
    gui_draw_button(cx,       cy + 60, 88, 22, "Win95", cfg->start_menu_bg == 0x00C0C0C0);
    gui_draw_button(cx + 94,  cy + 60, 88, 22, "Dark",  cfg->start_menu_bg == 0x0024262E);
    gui_draw_button(cx + 188, cy + 60, 88, 22, "Retro", cfg->start_menu_bg == 0x00D8D0B8);
    gui_draw_button(cx + 282, cy + 60, 88, 22, "Ocean", cfg->start_menu_bg == 0x00A8B8D0);

    gui_draw_text(cx, cy + 88, ru ? "4. Мышь:" : "4. Mouse:", tc);
    gui_draw_button(cx,      cy + 106, 40, 22, "1x", cfg->mouse_speed == 1);
    gui_draw_button(cx + 44, cy + 106, 40, 22, "2x", cfg->mouse_speed == 2);
    gui_draw_button(cx + 88, cy + 106, 40, 22, "3x", cfg->mouse_speed == 3);

    gui_draw_text(cx + 150, cy + 88, ru ? "5. Курсор:" : "5. Cursor:", tc);
    gui_draw_button(cx + 150, cy + 106, 54, 22, "White", cfg->cursor_theme == 0);
    gui_draw_button(cx + 208, cy + 106, 54, 22, "Neon",  cfg->cursor_theme == 1);
    gui_draw_button(cx + 266, cy + 106, 54, 22, "Gold",  cfg->cursor_theme == 2);
    gui_draw_button(cx + 324, cy + 106, 54, 22, "Pink",  cfg->cursor_theme == 3);

    gui_draw_text(cx, cy + 132, ru ? "6. Стиль и экспериментальные функции:" : "6. Style & Experimental:", tc);
    gui_draw_button(cx,       cy + 148, 120, 24, "DarkWin", cfg->dark_windows);
    gui_draw_button(cx + 128, cy + 148, 120, 24, "FlatUI",  cfg->flat_style);
    gui_draw_button(cx + 256, cy + 148, 122, 24, "Shadows", cfg->icon_shadows);

    gui_draw_button(cx,       cy + 186, 132, 24, "Packages [EXP]", cfg->enable_packages);
    gui_draw_button(cx + 138, cy + 186, 118, 24, ru ? "Язык: RU" : "Lang: EN", 0);
    gui_draw_button(cx + 262, cy + 186, 116, 24, ru ? "Сброс" : "Reset", 0);
}
