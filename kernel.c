#include <stdint.h>
#include "gui.h"
#include "mouse.h"
#include "keyboard.h"

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms1, syms2, syms3, syms4;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
} __attribute__((packed)) multiboot_info_t;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

int str_eq(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return (*a == '\0' && *b == '\0');
}

void str_append(char* dst, int* len, const char* src, int max_len) {
    while (*src && *len < max_len - 1) {
        *(dst + *len) = *src;
        (*len)++;
        src++;
    }
    *(dst + *len) = '\0';
}

void uint_to_str(uint32_t val, char* buf) {
    char* tmp = (char*)0x00400000;
    int i = 0, j = 0;
    if (val == 0) {
        *(buf + 0) = '0';
        *(buf + 1) = '\0';
        return;
    }
    while (val > 0) {
        *(tmp + i) = (val % 10) + '0';
        i++;
        val /= 10;
    }
    while (i > 0) {
        i--;
        *(buf + j) = *(tmp + i);
        j++;
    }
    *(buf + j) = '\0';
}

void get_cpu_vendor(char* vendor) {
    uint32_t ebx, ecx, edx;
    __asm__ volatile ("cpuid" : "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
    uint32_t* v32 = (uint32_t*)vendor;
    *(v32 + 0) = ebx;
    *(v32 + 1) = edx;
    *(v32 + 2) = ecx;
    *(vendor + 12) = '\0';
}

static const int16_t sin_table[] = {
    0, 4, 9, 13, 18, 22, 27, 31, 36, 40, 44, 49, 53, 58, 62, 66,
    71, 75, 79, 83, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 132,
    136, 139, 143, 147, 150, 154, 158, 161, 165, 168, 171, 175, 178, 181, 184, 187,
    190, 193, 196, 199, 202, 204, 207, 210, 212, 215, 217, 219, 222, 224, 226, 228,
    230, 232, 234, 236, 237, 239, 241, 242, 243, 245, 246, 247, 248, 249, 250, 251,
    252, 253, 254, 254, 255, 255, 255, 256, 256, 256, 256
};

int fast_sin(int deg) {
    deg %= 360;
    if (deg < 0) deg += 360;
    if (deg <= 90) return *(sin_table + deg);
    if (deg <= 180) return *(sin_table + (180 - deg));
    if (deg <= 270) return -(*(sin_table + (deg - 180)));
    return -(*(sin_table + (360 - deg)));
}

int fast_cos(int deg) { return fast_sin(deg + 90); }

void render_solid_3d_cube(int cx, int cy, int yaw, int pitch) {
    int sy = fast_sin(yaw), cy_cos = fast_cos(yaw);
    int sp = fast_sin(pitch), cp = fast_cos(pitch);
    int* px = (int*)0x00401000;
    int* py = (int*)0x00401100;
    int* pz = (int*)0x00401200;

    for (int i = 0; i < 8; i++) {
        int x = (i & 1) ? 58 : -58;
        int y = (i & 2) ? 58 : -58;
        int z = (i & 4) ? 58 : -58;
        int rx = (x * cy_cos - z * sy) >> 8;
        int rz = (x * sy + z * cy_cos) >> 8;
        int ry = (y * cp - rz * sp) >> 8;
        int fz = (y * sp + rz * cp) >> 8;
        int dist = 250 + fz;
        if (dist < 50) dist = 50;
        *(px + i) = cx + (rx * 190) / dist;
        *(py + i) = cy + (ry * 190) / dist;
        *(pz + i) = fz;
    }

    const uint8_t faces[] = { 0,1,3,2, 4,6,7,5, 0,2,6,4, 1,5,7,3, 2,3,7,6, 0,4,5,1 };
    const uint32_t colors[] = { 0x002E8B57, 0x001E90FF, 0x00DAA520, 0x00CD5C5C, 0x006A5ACD, 0x0020B2AA };
    int* face_order = (int*)0x00401300;
    int* face_depth = (int*)0x00401400;

    for (int f = 0; f < 6; f++) {
        *(face_order + f) = f;
        *(face_depth + f) = *(pz + *(faces + f*4)) + *(pz + *(faces + f*4 + 1)) +
                            *(pz + *(faces + f*4 + 2)) + *(pz + *(faces + f*4 + 3));
    }
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5 - i; j++) {
            int f1 = *(face_order + j), f2 = *(face_order + j + 1);
            if (*(face_depth + f1) < *(face_depth + f2)) {
                *(face_order + j) = f2;
                *(face_order + j + 1) = f1;
            }
        }
    }
    for (int i = 0; i < 6; i++) {
        int f = *(face_order + i);
        int v0 = *(faces + f*4), v1 = *(faces + f*4 + 1), v2 = *(faces + f*4 + 2), v3 = *(faces + f*4 + 3);
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
        int btn_x = win->x + win->w - 22;
        int btn_y = win->y + 6;
        if (m->x >= btn_x && m->x <= btn_x + 16 && m->y >= btn_y && m->y <= btn_y + 16) {
            win->visible = 0;
            return 2;
        }
        if (m->x >= win->x + 4 && m->x <= win->x + win->w - 4 &&
            m->y >= win->y + 4 && m->y <= win->y + 24) {
            win->dragging = 1;
            win->drag_offset_x = m->x - win->x;
            win->drag_offset_y = m->y - win->y;
            return 1;
        }
        if (m->x >= win->x && m->x <= win->x + win->w &&
            m->y >= win->y && m->y <= win->y + win->h) return 1;
    }
    if (!m->left_button) win->dragging = 0;
    if (win->dragging) {
        win->x = m->x - win->drag_offset_x;
        win->y = m->y - win->drag_offset_y;
    }
    return 0;
}

int run_language_selector(MouseState* mouse) {
    int selected = 1;
    uint8_t prev_left = 0;
    int box_w = 340, box_h = 200;
    int box_x = (SCREEN_W - box_w) / 2;
    int box_y = (SCREEN_H - box_h) / 2;

    while (1) {
        mouse_poll(mouse);
        char ch = keyboard_poll_char();
        if (ch == '1') selected = 0;
        else if (ch == '2') selected = 1;
        else if (ch == '\n') return selected;

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
        gui_draw_text(box_x + 10, box_y + 7, "M95OS Setup - Language / Язык", WIN95_WHITE);

        gui_draw_text(box_x + 24, box_y + 38, "Select system language:", WIN95_BLACK);
        gui_draw_text(box_x + 24, box_y + 55, "Выберите язык системы:", WIN95_BLACK);

        gui_draw_rect(btn_en_x, btn_y, btn_w, btn_h, selected == 0 ? WIN95_WHITE : WIN95_GRAY);
        gui_draw_bevel(btn_en_x, btn_y, btn_w, btn_h, selected == 0 ? 1 : 0);
        gui_draw_text(btn_en_x + 25, btn_y + 11, "[1] English", WIN95_BLACK);

        gui_draw_rect(btn_ru_x, btn_y, btn_w, btn_h, selected == 1 ? WIN95_WHITE : WIN95_GRAY);
        gui_draw_bevel(btn_ru_x, btn_y, btn_w, btn_h, selected == 1 ? 1 : 0);
        gui_draw_text(btn_ru_x + 25, btn_y + 11, "[2] Русский", WIN95_BLACK);

        gui_draw_rect(ok_x, ok_y, ok_w, ok_h, WIN95_GRAY);
        gui_draw_bevel(ok_x, ok_y, ok_w, ok_h, 0);
        gui_draw_text(ok_x + 32, ok_y + 7, "OK / Enter", WIN95_BLACK);

        gui_draw_cursor(mouse->x, mouse->y);
        gui_swap_buffers();
    }
}
typedef struct {
    const char* name;
    int used;
    int size;
    char* data_ptr;
} M95File;

void fs_init(M95File* files, int lang) {
    for (int i = 0; i < 6; i++) {
        (files + i)->used = 0;
        (files + i)->size = 0;
        (files + i)->data_ptr = (char*)(0x00405000 + i * 512);
        *((files + i)->data_ptr) = '\0';
    }
    // Файл 0: note.txt (связан с Блокнотом!)
    const char* n0 = "note.txt";
    (files + 0)->name = n0;
    (files + 0)->used = 1;
    const char* t0 = lang ? "Привет! Это файл note.txt\nПиши здесь и жми Enter!" : "Hello! This is note.txt\nType here and press Enter!";
    str_append((files + 0)->data_ptr, &((files + 0)->size), t0, 480);

    // Файл 1: readme.txt
    const char* n1 = "readme.txt";
    (files + 1)->name = n1;
    (files + 1)->used = 1;
    const char* t1 = lang ? "M95OS beta_1.1\nЛегковесная ОС с 3D и M95FS." : "M95OS beta_1.1\nLightweight OS with 3D & M95FS.";
    str_append((files + 1)->data_ptr, &((files + 1)->size), t1, 480);

    // Файл 2: system.ini
    const char* n2 = "system.ini";
    (files + 2)->name = n2;
    (files + 2)->used = 1;
    str_append((files + 2)->data_ptr, &((files + 2)->size), "[boot]\nkernel=m95os.bin\ngfx=800x600x32\n3d=tinygl_sw\n", 480);
}
void kernel_main(multiboot_info_t* mbi) {
    gui_init((uint32_t*)(uint32_t)mbi->framebuffer_addr, mbi->framebuffer_pitch);
    mouse_init();

    MouseState mouse = { SCREEN_W / 2, SCREEN_H / 2, 0, 0 };
    int lang = run_language_selector(&mouse);
    keyboard_set_ru(lang);

    char* cpu_vendor_buf = (char*)0x00400100;
    char* ram_str_buf    = (char*)0x00400200;
    char* clock_buf      = (char*)0x00400300;
    char* shell_out_buf  = (char*)0x00402000;
    char* shell_cmd_buf  = (char*)0x00402800;

    M95File* files = (M95File*)0x00404000;
    fs_init(files, lang);
    char* notepad_buf = (files + 0)->data_ptr;

    get_cpu_vendor(cpu_vendor_buf);
    uint_to_str((mbi->mem_lower + mbi->mem_upper) / 1024 + 1, ram_str_buf);

    int shell_out_len = 0;
    str_append(shell_out_buf, &shell_out_len, "M95OS Shell [beta_1.1]\nType 'help' for 12 commands.\n", 1400);
    int shell_cmd_len = 0;
    *(shell_cmd_buf + 0) = '\0';

    Window w0 = { 0, 110, 60,  340, 220, lang ? "Мой компьютер" : "My Computer", 0, 0, 0, 0, WIN95_GRAY };
    Window w1 = { 1, 380, 60,  370, 280, lang ? "TinyGL 3D" : "TinyGL 3D", 1, 0, 0, 0, 0x0010141C };
    Window w2 = { 2, 110, 80,  330, 230, lang ? "Блокнот - note.txt" : "Notepad - note.txt", 1, 0, 0, 0, WIN95_WHITE };
    Window w3 = { 3, 150, 290, 460, 260, lang ? "Терминал M95" : "M95 Shell", 1, 0, 0, 0, 0x00080808 };

    Window* wins = (Window*)0x00403000;
    *(wins + 0) = w0; *(wins + 1) = w1; *(wins + 2) = w2; *(wins + 3) = w3;

    int* z_order = (int*)0x00403200;
    *(z_order + 0) = 0; *(z_order + 1) = 1; *(z_order + 2) = 2; *(z_order + 3) = 3;

    uint8_t prev_left = 0;
    int selected_icon = -1, start_menu_open = 0;
    int yaw = 30, pitch = 25, auto_rotate = 1;
    uint32_t ticks = 0;
    uint32_t desktop_color = WIN95_DESKTOP_BG;
    while (1) {
        mouse_poll(&mouse);
        ticks++;
        if (auto_rotate) { yaw = (yaw + 2) % 360; pitch = (pitch + 1) % 360; }

        uint32_t secs = ticks / 60, mins = secs / 60;
        *(clock_buf + 0) = '0' + ((mins / 10) % 10);
        *(clock_buf + 1) = '0' + (mins % 10);
        *(clock_buf + 2) = ':';
        *(clock_buf + 3) = '0' + ((secs / 10) % 6);
        *(clock_buf + 4) = '0' + (secs % 10);
        *(clock_buf + 5) = '\0';

        int active_id = *(z_order + 3);
        char ch = keyboard_poll_char();

        if (ch != 0) {
            if (active_id == 1 && (wins + 1)->visible) {
                if (ch == ' ') auto_rotate = !auto_rotate;
                else if (ch == 'a' || (uint8_t)ch == 148) { auto_rotate = 0; yaw -= 10; }
                else if (ch == 'd' || (uint8_t)ch == 130) { auto_rotate = 0; yaw += 10; }
                else if (ch == 'w' || (uint8_t)ch == 150) { auto_rotate = 0; pitch -= 10; }
                else if (ch == 's' || (uint8_t)ch == 155) { auto_rotate = 0; pitch += 10; }
            }
            else if (active_id == 2 && (wins + 2)->visible) {
                int* nlen = &((files + 0)->size);
                if (ch == '\b' && *nlen > 0) {
                    (*nlen)--;
                    if (*nlen > 0 && ((uint8_t)*(notepad_buf + *nlen) & 0xC0) == 0x80) (*nlen)--;
                    *(notepad_buf + *nlen) = '\0';
                } else if (ch != '\b') {
                    str_append(notepad_buf, nlen, keyboard_char_to_utf8(ch), 490);
                }
            }
            else if (active_id == 3 && (wins + 3)->visible) {
                if (ch == '\b' && shell_cmd_len > 0) {
                    shell_cmd_len--;
                    if (shell_cmd_len > 0 && ((uint8_t)*(shell_cmd_buf + shell_cmd_len) & 0xC0) == 0x80) shell_cmd_len--;
                    *(shell_cmd_buf + shell_cmd_len) = '\0';
                } else if (ch == '\n') {
                    if (shell_out_len > 850) { shell_out_len = 0; *(shell_out_buf + 0) = '\0'; }
                    str_append(shell_out_buf, &shell_out_len, "C:\\> ", 1400);
                    str_append(shell_out_buf, &shell_out_len, shell_cmd_buf, 1400);
                    str_append(shell_out_buf, &shell_out_len, "\n", 1400);

                    if (str_eq(shell_cmd_buf, "help")) {
                        str_append(shell_out_buf, &shell_out_len, "help, ver, sysinfo, mem, cpu, ls, cat, clear,\ncube, note, color, time, reboot\n", 1400);
                    } else if (str_eq(shell_cmd_buf, "ver")) {
                        str_append(shell_out_buf, &shell_out_len, "M95OS Kernel beta_1.1 (i386 + TinyGL + M95FS)\n", 1400);
                    } else if (str_eq(shell_cmd_buf, "clear")) {
                        shell_out_len = 0; *(shell_out_buf + 0) = '\0';
                    } else if (str_eq(shell_cmd_buf, "sysinfo") || str_eq(shell_cmd_buf, "cpu")) {
                        str_append(shell_out_buf, &shell_out_len, "CPU: ", 1400);
                        str_append(shell_out_buf, &shell_out_len, cpu_vendor_buf, 1400);
                        str_append(shell_out_buf, &shell_out_len, " | RAM: ", 1400);
                        str_append(shell_out_buf, &shell_out_len, ram_str_buf, 1400);
                        str_append(shell_out_buf, &shell_out_len, " MB\n", 1400);
                    } else if (str_eq(shell_cmd_buf, "mem")) {
                        str_append(shell_out_buf, &shell_out_len, "Kernel: 0x00100000 | FB: 0x00200000 | FS: 0x00404000\n", 1400);
                    } else if (str_eq(shell_cmd_buf, "time")) {
                        str_append(shell_out_buf, &shell_out_len, "Uptime: ", 1400);
                        str_append(shell_out_buf, &shell_out_len, clock_buf, 1400);
                        str_append(shell_out_buf, &shell_out_len, "\n", 1400);
                    } else if (str_eq(shell_cmd_buf, "ls") || str_eq(shell_cmd_buf, "dir")) {
                        str_append(shell_out_buf, &shell_out_len, "Volume C: [M95FS] -> note.txt, readme.txt, system.ini\n", 1400);
                    } else if (str_eq(shell_cmd_buf, "cat") || str_eq(shell_cmd_buf, "cat note.txt")) {
                        str_append(shell_out_buf, &shell_out_len, (files + 0)->data_ptr, 1400);
                        str_append(shell_out_buf, &shell_out_len, "\n", 1400);
                    } else if (str_eq(shell_cmd_buf, "cat readme.txt")) {
                        str_append(shell_out_buf, &shell_out_len, (files + 1)->data_ptr, 1400);
                        str_append(shell_out_buf, &shell_out_len, "\n", 1400);
                    } else if (str_eq(shell_cmd_buf, "cat system.ini")) {
                        str_append(shell_out_buf, &shell_out_len, (files + 2)->data_ptr, 1400);
                    } else if (str_eq(shell_cmd_buf, "color")) {
                        desktop_color = (desktop_color == WIN95_DESKTOP_BG) ? 0x002B4865 : ((desktop_color == 0x002B4865) ? 0x003B3B55 : WIN95_DESKTOP_BG);
                        str_append(shell_out_buf, &shell_out_len, "Desktop theme updated.\n", 1400);
                    } else if (str_eq(shell_cmd_buf, "cube")) {
                        (wins + 1)->visible = 1;
                    } else if (str_eq(shell_cmd_buf, "note")) {
                        (wins + 2)->visible = 1;
                    } else if (str_eq(shell_cmd_buf, "reboot")) {
                        outb(0x64, 0xFE);
                    } else if (shell_cmd_len > 0) {
                        str_append(shell_out_buf, &shell_out_len, "Unknown cmd. Type 'help'.\n", 1400);
                    }
                    shell_cmd_len = 0; *(shell_cmd_buf + 0) = '\0';
                } else if (shell_cmd_len < 42 && ch != '\b') {
                    // В терминале разрешаем ввод латиницей и кириллицей!
                    str_append(shell_cmd_buf, &shell_cmd_len, keyboard_char_to_utf8(ch), 45);
                }
            }
        }
        if (mouse.left_button && !prev_left) {
            if (mouse.x >= SCREEN_W - 104 && mouse.x <= SCREEN_W - 76 && mouse.y >= SCREEN_H - 24)
                keyboard_set_ru(!keyboard_is_ru());
            else if (mouse.x >= 4 && mouse.x <= 72 && mouse.y >= SCREEN_H - 24)
                start_menu_open = !start_menu_open;
            else if (start_menu_open) {
                int mx = 4, my = SCREEN_H - 194;
                if (mouse.x >= mx && mouse.x <= mx + 185 && mouse.y >= my && mouse.y <= my + 166) {
                    int ry = mouse.y - my, oid = -1;
                    if (ry < 32) oid = 0; else if (ry < 58) oid = 1;
                    else if (ry < 84) oid = 2; else if (ry < 110) oid = 3;
                    else if (ry > 120) outb(0x64, 0xFE);
                    if (oid != -1) {
                        (wins + oid)->visible = 1;
                        for (int k = 0; k < 4; k++) if (*(z_order + k) == oid) {
                            for (int m = k; m < 3; m++) *(z_order + m) = *(z_order + m + 1);
                            *(z_order + 3) = oid; break;
                        }
                    }
                }
                start_menu_open = 0;
            }
        }
        int wh = 0;
        for (int i = 3; i >= 0; i--) {
            int id = *(z_order + i), res = handle_window_mouse(wins + id, &mouse, prev_left);
            if (res != 0) { wh = 1; if (res == 1) {
                for (int k = i; k < 3; k++) *(z_order + k) = *(z_order + k + 1);
                *(z_order + 3) = id;
            } break; }
        }
        if (mouse.left_button && !prev_left && !wh) {
            int ci = -1;
            if (mouse.x >= 10 && mouse.x <= 90) {
                if (mouse.y >= 15 && mouse.y <= 70) ci = 0;
                else if (mouse.y >= 85 && mouse.y <= 140) ci = 1;
                else if (mouse.y >= 155 && mouse.y <= 210) ci = 2;
                else if (mouse.y >= 225 && mouse.y <= 280) ci = 3;
            }
            if (ci != -1 && selected_icon == ci) {
                (wins + ci)->visible = 1;
                for (int k = 0; k < 4; k++) if (*(z_order + k) == ci) {
                    for (int m = k; m < 3; m++) *(z_order + m) = *(z_order + m + 1);
                    *(z_order + 3) = ci; break;
                }
            }
            selected_icon = ci;
        }
        prev_left = mouse.left_button;

        gui_draw_rect(0, 0, SCREEN_W, SCREEN_H, desktop_color);
        gui_draw_icon(12, 15,  0, lang ? "Мой комп." : "My Computer", selected_icon == 0);
        gui_draw_icon(12, 85,  1, "TinyGL 3D", selected_icon == 1);
        gui_draw_icon(12, 155, 2, lang ? "Блокнот" : "Notepad", selected_icon == 2);
        gui_draw_icon(12, 225, 3, lang ? "Терминал" : "M95 Shell", selected_icon == 3);

        for (int i = 0; i < 4; i++) {
            int id = *(z_order + i);
            Window* w = wins + id;
            if (!w->visible) continue;
            gui_draw_window(w, i == 3);
            int cx = w->x + 14, cy = w->y + 36;
            if (id == 0) {
                gui_draw_text(cx, cy, "M95OS beta_1.1 (M95FS)", WIN95_BLACK);
                gui_draw_text(cx, cy + 22, cpu_vendor_buf, 0x00000080);
                gui_draw_text(cx, cy + 42, ram_str_buf, 0x00000080);
                gui_draw_text(cx + 40, cy + 42, "MB RAM", WIN95_BLACK);
            } else if (id == 1) {
                render_solid_3d_cube(w->x + w->w / 2, w->y + w->h / 2 + 6, yaw, pitch);
            } else if (id == 2) {
                gui_draw_text(cx, cy, notepad_buf, WIN95_BLACK);
                if (i == 3 && (ticks / 20) % 2 == 0) {
                    gui_draw_text(cx, w->y + w->h - 22, "[Editing note.txt | Alt+Shift]", 0x00000080);
                }
            } else if (id == 3) {
                gui_draw_text(cx, cy, shell_out_buf, 0x00CCCCCC);
                int py = w->y + w->h - 24;
                gui_draw_text(cx, py, "> ", 0x0055FF55);
                gui_draw_text(cx + 16, py, shell_cmd_buf, 0x00FFFFFF);
                if (i == 3 && (ticks / 20) % 2 == 0) {
                    gui_draw_rect(cx + 16 + gui_text_width(shell_cmd_buf), py + 2, 7, 11, 0x0055FF55);
                }
            }
        }
        gui_draw_start_menu(start_menu_open, lang);
        gui_draw_taskbar(start_menu_open, lang, keyboard_is_ru(), clock_buf);
        gui_draw_cursor(mouse.x, mouse.y);
        gui_swap_buffers();
    }
}
