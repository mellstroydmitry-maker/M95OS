#include <stdint.h>
#include "gui.h"
#include "mouse.h"
#include "keyboard.h"
#include "settings_app.h"
#include "games_app.h"
#include "paint_app.h"
#include "craft_app.h"
#include "m95pkg.h"


typedef struct {
    uint32_t flags, mem_lower, mem_upper, boot_device, cmdline, mods_count, mods_addr;
    uint32_t syms1, syms2, syms3, syms4, mmap_length, mmap_addr, drives_length, drives_addr;
    uint32_t config_table, boot_loader_name, apm_table, vbe_control_info, vbe_mode_info;
    uint16_t vbe_mode, vbe_interface_seg, vbe_interface_off, vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch, framebuffer_width, framebuffer_height;
    uint8_t  framebuffer_bpp;
} __attribute__((packed)) multiboot_info_t;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

int str_eq(const char* a, const char* b) {
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return (*a == '\0' && *b == '\0');
}

void str_append(char* dst, int* len, const char* src, int max_len) {
    while (*src && *len < max_len - 1) { *(dst + *len) = *src; (*len)++; src++; }
    *(dst + *len) = '\0';
}

void uint_to_str(uint32_t val, char* buf) {
    char* tmp = (char*)0x00400000;
    int i = 0, j = 0;
    if (val == 0) { *(buf + 0) = '0'; *(buf + 1) = '\0'; return; }
    while (val > 0) { *(tmp + i) = (val % 10) + '0'; i++; val /= 10; }
    while (i > 0) { i--; *(buf + j) = *(tmp + i); j++; }
    *(buf + j) = '\0';
}

void get_cpu_vendor(char* vendor) {
    uint32_t ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    uint32_t* v32 = (uint32_t*)vendor;
    *(v32 + 0) = ebx; *(v32 + 1) = edx; *(v32 + 2) = ecx; *(vendor + 12) = '\0';
}
static const int16_t sin_table[] = {
    0,4,9,13,18,22,27,31,36,40,44,49,53,58,62,66,71,75,79,83,88,92,96,100,104,108,112,116,120,124,128,132,
    136,139,143,147,150,154,158,161,165,168,171,175,178,181,184,187,190,193,196,199,202,204,207,210,212,215,
    217,219,222,224,226,228,230,232,234,236,237,239,241,242,243,245,246,247,248,249,250,251,252,253,254,254,
    255,255,255,256,256,256,256
};

int fast_sin(int deg) {
    deg %= 360; if (deg < 0) deg += 360;
    if (deg <= 90) return *(sin_table + deg);
    if (deg <= 180) return *(sin_table + (180 - deg));
    if (deg <= 270) return -(*(sin_table + (deg - 180)));
    return -(*(sin_table + (360 - deg)));
}
int fast_cos(int deg) { return fast_sin(deg + 90); }

void render_solid_3d_cube(int cx, int cy, int yaw, int pitch) {
    int sy = fast_sin(yaw), cy_cos = fast_cos(yaw), sp = fast_sin(pitch), cp = fast_cos(pitch);
    int *px = (int*)0x00401000, *py = (int*)0x00401100, *pz = (int*)0x00401200;
    for (int i = 0; i < 8; i++) {
        int x = (i & 1) ? 58 : -58, y = (i & 2) ? 58 : -58, z = (i & 4) ? 58 : -58;
        int rx = (x * cy_cos - z * sy) >> 8, rz = (x * sy + z * cy_cos) >> 8;
        int ry = (y * cp - rz * sp) >> 8, fz = (y * sp + rz * cp) >> 8;
        int dist = 250 + fz; if (dist < 50) dist = 50;
        *(px + i) = cx + (rx * 190) / dist; *(py + i) = cy + (ry * 190) / dist; *(pz + i) = fz;
    }
    const uint8_t faces[] = { 0,1,3,2, 4,6,7,5, 0,2,6,4, 1,5,7,3, 2,3,7,6, 0,4,5,1 };
    const uint32_t colors[] = { 0x002E8B57, 0x001E90FF, 0x00DAA520, 0x00CD5C5C, 0x006A5ACD, 0x0020B2AA };
    int *fo = (int*)0x00401300, *fd = (int*)0x00401400;
    for (int f = 0; f < 6; f++) {
        *(fo + f) = f;
        *(fd + f) = *(pz + *(faces + f*4)) + *(pz + *(faces + f*4 + 1)) + *(pz + *(faces + f*4 + 2)) + *(pz + *(faces + f*4 + 3));
    }
    for (int i = 0; i < 5; i++) for (int j = 0; j < 5 - i; j++) {
        int f1 = *(fo + j), f2 = *(fo + j + 1);
        if (*(fd + f1) < *(fd + f2)) { *(fo + j) = f2; *(fo + j + 1) = f1; }
    }
    for (int i = 0; i < 6; i++) {
        int f = *(fo + i), v0 = *(faces + f*4), v1 = *(faces + f*4 + 1), v2 = *(faces + f*4 + 2), v3 = *(faces + f*4 + 3);
        uint32_t col = *(colors + f);
        gui_fill_triangle(*(px+v0), *(py+v0), *(px+v1), *(py+v1), *(px+v2), *(py+v2), col);
        gui_fill_triangle(*(px+v0), *(py+v0), *(px+v2), *(py+v2), *(px+v3), *(py+v3), col);
        gui_draw_line(*(px+v0), *(py+v0), *(px+v1), *(py+v1), 0x00FFFFFF);
        gui_draw_line(*(px+v1), *(py+v1), *(px+v2), *(py+v2), 0x00FFFFFF);
        gui_draw_line(*(px+v2), *(py+v2), *(px+v3), *(py+v3), 0x00FFFFFF);
        gui_draw_line(*(px+v3), *(py+v3), *(px+v0), *(py+v0), 0x00FFFFFF);
    }
}

int handle_window_mouse(Window* win, MouseState* m, int prev_left) {
    if (!win->visible) return 0;
    if (m->left_button && !prev_left) {
        int btn_x = win->x + win->w - 22, btn_y = win->y + 6;
        if (m->x >= btn_x && m->x <= btn_x + 16 && m->y >= btn_y && m->y <= btn_y + 16) { win->visible = 0; return 2; }
        if (m->x >= win->x + 4 && m->x <= win->x + win->w - 4 && m->y >= win->y + 4 && m->y <= win->y + 24) {
            win->dragging = 1; win->drag_offset_x = m->x - win->x; win->drag_offset_y = m->y - win->y; return 1;
        }
        if (m->x >= win->x && m->x <= win->x + win->w && m->y >= win->y && m->y <= win->y + win->h) return 1;
    }
    if (!m->left_button) win->dragging = 0;
    if (win->dragging) { win->x = m->x - win->drag_offset_x; win->y = m->y - win->drag_offset_y; }
    return 0;
}

int run_language_selector(MouseState* mouse) {
    int selected = 1; uint8_t prev_left = 0;
    int box_w = 340, box_h = 200, box_x = (SCREEN_W - box_w) / 2, box_y = (SCREEN_H - box_h) / 2;
    while (1) {
        mouse_poll(mouse);
        char ch = keyboard_poll_char();
        if (ch == '1') selected = 0; else if (ch == '2') selected = 1; else if (ch == '\n') return selected;
        int btn_en_x = box_x + 30, btn_ru_x = box_x + 180, btn_y = box_y + 85, btn_w = 130, btn_h = 36;
        int ok_x = box_x + 110, ok_y = box_y + 145, ok_w = 120, ok_h = 28;
        if (mouse->left_button && !prev_left) {
            if (mouse->x >= btn_en_x && mouse->x <= btn_en_x + btn_w && mouse->y >= btn_y && mouse->y <= btn_y + btn_h) selected = 0;
            if (mouse->x >= btn_ru_x && mouse->x <= btn_ru_x + btn_w && mouse->y >= btn_y && mouse->y <= btn_y + btn_h) selected = 1;
            if (mouse->x >= ok_x && mouse->x <= ok_x + ok_w && mouse->y >= ok_y && mouse->y <= ok_y + ok_h) return selected;
        }
        prev_left = mouse->left_button;
        gui_draw_rect(0, 0, SCREEN_W, SCREEN_H, 0x001E4848);
        gui_draw_rect(box_x, box_y, box_w, box_h, WIN95_GRAY);
        gui_draw_bevel(box_x, box_y, box_w, box_h, 0);
        gui_draw_rect(box_x + 4, box_y + 4, box_w - 8, 20, WIN95_TITLE_ACTIVE);
        gui_draw_text(box_x + 10, box_y + 7, "M95OS beta 1.9 - Language", WIN95_WHITE);
        gui_draw_text(box_x + 24, box_y + 38, "Select system language:", WIN95_BLACK);
        gui_draw_text(box_x + 24, box_y + 55, "Выберите язык системы:", WIN95_BLACK);
        gui_draw_button(btn_en_x, btn_y, btn_w, btn_h, "English", selected == 0);
        gui_draw_button(btn_ru_x, btn_y, btn_w, btn_h, "Русский", selected == 1);
        gui_draw_button(ok_x, ok_y, ok_w, ok_h, "OK / Enter", 0);
        gui_draw_cursor(mouse->x, mouse->y, 0);
        gui_swap_buffers();
    }
}
void kernel_main(multiboot_info_t* mbi) {
    gui_init((uint32_t*)(uint32_t)mbi->framebuffer_addr, mbi->framebuffer_pitch);
    mouse_init(); games_init(); paint_init(); craft_init(); pkg_init();

    if (mbi->mods_count > 0) {
        uint32_t* mod = (uint32_t*)(mbi->mods_addr);
        for (uint32_t i = 0; i < mbi->mods_count; i++) {
            uint32_t start = *(mod + i * 4 + 0);
            uint32_t end   = *(mod + i * 4 + 1);
            pkg_load_disk_package((void*)start, end - start);
        }
    }

    MouseState mouse = { SCREEN_W / 2, SCREEN_H / 2, 0, 0 };
    int init_lang = run_language_selector(&mouse);
    keyboard_set_ru(init_lang);

    ThemeConfig* cfg = (ThemeConfig*)0x00404500;
    settings_init(cfg, init_lang);

    char *cpu_vendor_buf = (char*)0x00400100, *ram_str_buf = (char*)0x00400200, *clock_buf = (char*)0x00400300;
    char *notepad_buf = (char*)0x00405000, *shell_out_buf = (char*)0x00402000, *shell_cmd_buf = (char*)0x00402800;

    get_cpu_vendor(cpu_vendor_buf);
    uint_to_str((mbi->mem_lower + mbi->mem_upper) / 1024 + 1, ram_str_buf);

    int note_len = 0;
    str_append(notepad_buf, &note_len, init_lang ? "M95OS pre-release!\nВключи Packages [EXP] в Настройках!" : "M95OS pre-release!\nEnable Packages [EXP] in Settings!", 480);

    int shell_out_len = 0;
    str_append(shell_out_buf, &shell_out_len, "M95OS Shell [pre-release]\nType 'help'\n", 1400);
    int shell_cmd_len = 0; *(shell_cmd_buf + 0) = '\0';

    Window w0 = { 0, 185, 50,  340, 220, "My Computer", 0, 0, 0, 0, WIN95_GRAY };
    Window w1 = { 1, 470, 35,  300, 230, "TinyGL 3D", 0, 0, 0, 0, 0x0010141C };
    Window w2 = { 2, 185, 80,  330, 220, "Notepad", 0, 0, 0, 0, WIN95_WHITE };
    Window w3 = { 3, 185, 345, 360, 205, "M95 Shell", 1, 0, 0, 0, 0x00080808 };
    Window w4 = { 4, 185, 45,  410, 260, "Settings", 1, 0, 0, 0, WIN95_GRAY };
    Window w5 = { 5, 455, 280, 305, 265, "M95 Arcade", 0, 0, 0, 0, WIN95_GRAY };
    Window w6 = { 6, 185, 35,  306, 245, "M95 Paint", 0, 0, 0, 0, WIN95_GRAY };
    Window w7 = { 7, 465, 35,  325, 270, "M95 Craft", 0, 0, 0, 0, WIN95_GRAY };
    Window w8 = { 8, 185, 40,  345, 240, "M95 Packages", 0, 0, 0, 0, WIN95_GRAY };
    Window w9 = { 9, 545, 60,  220, 165, "User App", 0, 0, 0, 0, WIN95_GRAY };

    Window* wins = (Window*)0x00403000;
    *(wins + 0) = w0; *(wins + 1) = w1; *(wins + 2) = w2; *(wins + 3) = w3; *(wins + 4) = w4;
    *(wins + 5) = w5; *(wins + 6) = w6; *(wins + 7) = w7; *(wins + 8) = w8; *(wins + 9) = w9;

    int* z_order = (int*)0x00403200;
    for (int i = 0; i < 10; i++) *(z_order + i) = i;

    uint8_t prev_left = 0, prev_right = 0;
    int selected_icon = -1, start_menu_open = 0, ctx_open = 0, ctx_x = 0, ctx_y = 0;
    int yaw = 30, pitch = 25, auto_rotate = 1;
    uint32_t ticks = 0;
    while (1) {
        mouse_poll(&mouse); ticks++;
        if (auto_rotate) { yaw = (yaw + 2) % 360; pitch = (pitch + 1) % 360; }

        if (!cfg->enable_packages) { (wins + 8)->visible = 0; (wins + 9)->visible = 0; }
        else if (pkg_app_is_running()) { (wins + 9)->visible = 1; (wins + 9)->title = pkg_app_title(); }
        else (wins + 9)->visible = 0;

        int lang = cfg->lang;
        (wins + 0)->title = lang ? "Мой компьютер" : "My Computer";
        (wins + 2)->title = lang ? "Блокнот" : "Notepad";
        (wins + 3)->title = lang ? "Терминал M95" : "M95 Shell";
        (wins + 4)->title = lang ? "Настройки" : "Settings";
        (wins + 5)->title = lang ? "Игры M95" : "M95 Arcade";
        (wins + 6)->title = "M95 Paint";
        (wins + 7)->title = "M95 Craft";
        (wins + 8)->title = lang ? "Пакеты [EXP]" : "Packages [EXP]";

        uint32_t secs = ticks / 60, mins = secs / 60;
        *(clock_buf + 0) = '0' + ((mins / 10) % 10); *(clock_buf + 1) = '0' + (mins % 10);
        *(clock_buf + 2) = ':';
        *(clock_buf + 3) = '0' + ((secs / 10) % 6); *(clock_buf + 4) = '0' + (secs % 10);
        *(clock_buf + 5) = '\0';

        int active_id = *(z_order + 9);
        char ch = keyboard_poll_char();
        if (ch != 0) {
            if (active_id == 7 && (wins + 7)->visible) craft_input(ch);
            else if (active_id == 5 && (wins + 5)->visible) games_input(ch);
            else if (active_id == 1 && (wins + 1)->visible) {
                if (ch == ' ') auto_rotate = !auto_rotate;
                else if (ch == 'a' || (uint8_t)ch == 148) { auto_rotate = 0; yaw -= 10; }
                else if (ch == 'd' || (uint8_t)ch == 130) { auto_rotate = 0; yaw += 10; }
                else if (ch == 'w' || (uint8_t)ch == 150) { auto_rotate = 0; pitch -= 10; }
                else if (ch == 's' || (uint8_t)ch == 155) { auto_rotate = 0; pitch += 10; }
            } else if (active_id == 2 && (wins + 2)->visible) {
                if (ch == '\b' && note_len > 0) { note_len--; if (note_len > 0 && ((uint8_t)*(notepad_buf + note_len) & 0xC0) == 0x80) note_len--; *(notepad_buf + note_len) = '\0'; }
                else if (ch != '\b') str_append(notepad_buf, &note_len, keyboard_char_to_utf8(ch), 470);
            } else if (active_id == 3 && (wins + 3)->visible) {
                if (ch == '\b' && shell_cmd_len > 0) { shell_cmd_len--; if (shell_cmd_len > 0 && ((uint8_t)*(shell_cmd_buf + shell_cmd_len) & 0xC0) == 0x80) shell_cmd_len--; *(shell_cmd_buf + shell_cmd_len) = '\0'; }
                else if (ch == '\n') {
                    if (shell_out_len > 850) { shell_out_len = 0; *(shell_out_buf + 0) = '\0'; }
                    str_append(shell_out_buf, &shell_out_len, "> ", 1400); str_append(shell_out_buf, &shell_out_len, shell_cmd_buf, 1400); str_append(shell_out_buf, &shell_out_len, "\n", 1400);
                    int pres = cfg->enable_packages ? pkg_command(shell_cmd_buf, shell_out_buf, &shell_out_len, 1400) : 0;
                    if (pres == 2) { (wins + 9)->visible = 1; *(z_order + 9) = 9; }
                    else if (pres == 0) {
                        if (str_eq(shell_cmd_buf, "help")) str_append(shell_out_buf, &shell_out_len, "craft, paint, games, settings, cube, note, m95pkg, cls, reboot\n", 1400);
                        else if (str_eq(shell_cmd_buf, "m95pkg") && !cfg->enable_packages) str_append(shell_out_buf, &shell_out_len, "[EXP] Enable Packages in Settings first!\n", 1400);
                        else if (str_eq(shell_cmd_buf, "craft")) (wins + 7)->visible = 1;
                        else if (str_eq(shell_cmd_buf, "paint")) (wins + 6)->visible = 1;
                        else if (str_eq(shell_cmd_buf, "games")) (wins + 5)->visible = 1;
                        else if (str_eq(shell_cmd_buf, "settings")) (wins + 4)->visible = 1;
                        else if (str_eq(shell_cmd_buf, "cls") || str_eq(shell_cmd_buf, "clear")) { shell_out_len = 0; *(shell_out_buf + 0) = '\0'; }
                        else if (str_eq(shell_cmd_buf, "cube")) (wins + 1)->visible = 1;
                        else if (str_eq(shell_cmd_buf, "note")) (wins + 2)->visible = 1;
                        else if (str_eq(shell_cmd_buf, "reboot")) outb(0x64, 0xFE);
                    }
                    shell_cmd_len = 0; *(shell_cmd_buf + 0) = '\0';
                } else if (shell_cmd_len < 42 && ch != '\b') str_append(shell_cmd_buf, &shell_cmd_len, keyboard_char_to_utf8(ch), 45);
            }
        }

        if (mouse.left_button && !prev_left) {
            if (ctx_open) {
                if (mouse.x >= ctx_x && mouse.x <= ctx_x + 140 && mouse.y >= ctx_y && mouse.y <= ctx_y + 78) {
                    int ry = mouse.y - ctx_y;
                    if (ry < 26) (wins + 7)->visible = 1;
                    else if (ry < 52) (wins + 6)->visible = 1;
                    else (wins + 4)->visible = 1;
                }
                ctx_open = 0;
            }
            if (mouse.x >= SCREEN_W - 104 && mouse.x <= SCREEN_W - 40 && mouse.y >= SCREEN_H - 24) keyboard_set_ru(!keyboard_is_ru());
            else if (mouse.x >= 4 && mouse.x <= 72 && mouse.y >= SCREEN_H - 24) start_menu_open = !start_menu_open;
            else if (mouse.y >= SCREEN_H - 24 && mouse.x >= 76 && mouse.x < SCREEN_W - 110) {
                int tab_idx = (mouse.x - 76) / 60;
                if (tab_idx >= 0 && tab_idx < 10 && (wins + tab_idx)->visible) {
                    for (int k = 0; k < 10; k++) if (*(z_order + k) == tab_idx) {
                        for (int m = k; m < 9; m++) *(z_order + m) = *(z_order + m + 1);
                        *(z_order + 9) = tab_idx; break;
                    }
                }
            } else if (start_menu_open) {
                int mx = 4, my = SCREEN_H - 246;
                if (mouse.x >= mx && mouse.x <= mx + 185 && mouse.y >= my && mouse.y <= my + 218) {
                    int ry = mouse.y - my, oid = -1;
                    if (ry < 32) oid = 0; else if (ry < 58) oid = 1; else if (ry < 84) oid = 2;
                    else if (ry < 110) oid = 3; else if (ry < 138) oid = 4; else if (ry < 164) oid = 5;
                    else if (ry > 172) outb(0x64, 0xFE);
                    if (oid != -1) {
                        (wins + oid)->visible = 1;
                        for (int k = 0; k < 10; k++) if (*(z_order + k) == oid) {
                            for (int m = k; m < 9; m++) *(z_order + m) = *(z_order + m + 1);
                            *(z_order + 9) = oid; break;
                        }
                    }
                }
                start_menu_open = 0;
            }
        }

        int wh = 0;
        for (int i = 9; i >= 0; i--) {
            int id = *(z_order + i), res = handle_window_mouse(wins + id, &mouse, prev_left);
            if (res != 0) {
                wh = 1;
                if (res == 1) {
                    for (int k = i; k < 9; k++) *(z_order + k) = *(z_order + k + 1);
                    *(z_order + 9) = id;
                    if (id == 4) settings_handle_click(cfg, wins + 4, &mouse, prev_left);
                }
                break;
            }
        }
        if (*(z_order + 9) == 5) games_click(wins + 5, &mouse, prev_left, prev_right);
        if (*(z_order + 9) == 6) paint_update(wins + 6, &mouse, prev_left);
        if (*(z_order + 9) == 7) craft_click(wins + 7, &mouse, prev_left, prev_right);
        if (*(z_order + 9) == 8 && cfg->enable_packages) pkg_click(wins + 8, &mouse, prev_left);

        if (mouse.right_button && !prev_right && !wh) {
            ctx_open = 1; ctx_x = mouse.x; ctx_y = mouse.y;
            if (ctx_x > SCREEN_W - 145) ctx_x = SCREEN_W - 145;
            if (ctx_y > SCREEN_H - 115) ctx_y = SCREEN_H - 115;
        }

        if (mouse.left_button && !prev_left && !wh) {
            int ci = -1;
            if (mouse.x >= 10 && mouse.x <= 88) {
                if (mouse.y >= 15 && mouse.y <= 70) ci = 0; else if (mouse.y >= 85 && mouse.y <= 140) ci = 1;
                else if (mouse.y >= 155 && mouse.y <= 210) ci = 2; else if (mouse.y >= 225 && mouse.y <= 280) ci = 3;
                else if (mouse.y >= 295 && mouse.y <= 350) ci = 4; else if (mouse.y >= 365 && mouse.y <= 420) ci = 5;
                else if (mouse.y >= 435 && mouse.y <= 490) ci = 6;
            }
            if (mouse.x >= 92 && mouse.x <= 172) {
                if (mouse.y >= 15 && mouse.y <= 70) ci = 7;
                else if (mouse.y >= 85 && mouse.y <= 140 && cfg->enable_packages) ci = 8;
            }
            if (ci != -1 && selected_icon == ci) {
                (wins + ci)->visible = 1;
                for (int k = 0; k < 10; k++) if (*(z_order + k) == ci) {
                    for (int m = k; m < 9; m++) *(z_order + m) = *(z_order + m + 1);
                    *(z_order + 9) = ci; break;
                }
            }
            selected_icon = ci;
        }
        prev_left = mouse.left_button; prev_right = mouse.right_button;

        gui_draw_rect(0, 0, SCREEN_W, SCREEN_H, cfg->desktop_color);
        gui_draw_icon(10, 15,  0, lang ? "Мой комп." : "My PC", selected_icon == 0, cfg->icon_shadows);
        gui_draw_icon(10, 85,  1, "TinyGL 3D", selected_icon == 1, cfg->icon_shadows);
        gui_draw_icon(10, 155, 2, lang ? "Блокнот" : "Notepad", selected_icon == 2, cfg->icon_shadows);
        gui_draw_icon(10, 225, 3, lang ? "Терминал" : "Shell", selected_icon == 3, cfg->icon_shadows);
        gui_draw_icon(10, 295, 4, lang ? "Настройки" : "Settings", selected_icon == 4, cfg->icon_shadows);
        gui_draw_icon(10, 365, 5, lang ? "Игры M95" : "Arcade", selected_icon == 5, cfg->icon_shadows);
        gui_draw_icon(10, 435, 6, "Paint", selected_icon == 6, cfg->icon_shadows);
        gui_draw_icon(92, 15,  7, "M95 Craft", selected_icon == 7, cfg->icon_shadows);
if (cfg->enable_packages) gui_draw_icon(92, 85, 8, lang ? "Пакеты" : "Packages", selected_icon == 8, cfg->icon_shadows);
for (int i = 0; i < 10; i++) {
int id = *(z_order + i); Window* w = wins + id;
if (!w->visible) continue;
gui_draw_window(w, i == 9, cfg);
int cx = w->x + 14, cy = w->y + 36;
if (id == 0) {
gui_draw_text(cx, cy, "M95OS pre-release", WIN95_BLACK);
gui_draw_text(cx, cy + 22, cpu_vendor_buf, 0x00000080);
gui_draw_text(cx, cy + 42, ram_str_buf, 0x00000080); gui_draw_text(cx + 40, cy + 42, "MB RAM", WIN95_BLACK);
} else if (id == 1) render_solid_3d_cube(w->x + w->w / 2, w->y + w->h / 2 + 6, yaw, pitch);
else if (id == 2) gui_draw_text_box(cx, cy, w->w - 28, w->h - 46, notepad_buf, WIN95_BLACK);
else if (id == 3) {
gui_draw_text_box(cx, cy, w->w - 28, w->h - 68, shell_out_buf, 0x00CCCCCC);
int py = w->y + w->h - 24;
gui_draw_text(cx, py, "> ", 0x0055FF55); gui_draw_text(cx + 16, py, shell_cmd_buf, 0x00FFFFFF);
} else if (id == 4) settings_render(cfg, w);
else if (id == 5) games_update_and_render(w, lang);
else if (id == 6) paint_render(w, lang);
else if (id == 7) craft_render(w, lang);
else if (id == 8 && cfg->enable_packages) pkg_render(w, lang);
else if (id == 9 && cfg->enable_packages) pkg_app_render(w, &mouse, prev_left, ch);
}
if (ctx_open) {
gui_draw_rect(ctx_x, ctx_y, 140, 78, WIN95_GRAY); gui_draw_bevel(ctx_x, ctx_y, 140, 78, 0);
gui_draw_text(ctx_x + 10, ctx_y + 6,  "M95 Craft", WIN95_BLACK);
gui_draw_text(ctx_x + 10, ctx_y + 32, "M95 Paint", WIN95_BLACK);
gui_draw_text(ctx_x + 10, ctx_y + 58, lang ? "Настройки" : "Settings", WIN95_BLACK);
}
gui_draw_start_menu(start_menu_open, cfg);
gui_draw_taskbar(start_menu_open, cfg, keyboard_is_ru(), clock_buf);
const char* tab_names[] = { "PC", "3D", "Note", "Shell", "Prefs", "Games", "Paint", "Craft", "Pkg", "App" };
for (int t = 0; t < 10; t++) {
if ((wins + t)->visible) gui_draw_button(76 + t * 60, SCREEN_H - 24, 56, 20, *(tab_names + t), *(z_order + 9) == t);
}
gui_draw_cursor(mouse.x, mouse.y, cfg->cursor_theme);
gui_swap_buffers();
}
}
