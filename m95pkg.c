#include "m95pkg.h"

extern int str_eq(const char* a, const char* b);
extern void str_append(char* dst, int* len, const char* src, int max_len);

typedef struct {
    char name[24];
    char author[24];
    int installed;
    int type; // 1 = xeyes, 2 = white rain, 3 = внешний бинарник .m95 с диска/флешки
    void (*ext_entry)(int cx, int cy, int cw, int ch, int mx, int my, int click, char key);
} M95Package;

static M95Package* repo = (M95Package*)0x00450000;
static int repo_count = 0;
static int running_idx = -1;
static uint32_t ext_code_offset = 0;

static void str_copy_n(char* dst, const char* src, int n) {
    int i = 0;
    while (src[i] && i < n - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int starts_with(const char* s, const char* p) {
    while (*p) { if (*s != *p) return 0; s++; p++; }
    return 1;
}

void pkg_init(void) {
    // Заполняем системную таблицу API по адресу 0x00407000 для пользовательских .m95 программ
    M95ApiTable* api = (M95ApiTable*)0x00407000;
    api->draw_rect     = gui_draw_rect;
    api->draw_text     = gui_draw_text;
    api->draw_button   = gui_draw_button;
    api->draw_line     = gui_draw_line;
    api->fill_triangle = gui_fill_triangle;

    repo_count = 2;
    running_idx = -1;
    ext_code_offset = 0;

    str_copy_n((repo + 0)->name, "xeyes", 24);
    str_copy_n((repo + 0)->author, "X11/M95", 24);
    (repo + 0)->installed = 1; (repo + 0)->type = 1; (repo + 0)->ext_entry = 0;

    str_copy_n((repo + 1)->name, "rain", 24);
    str_copy_n((repo + 1)->author, "M95Std", 24);
    (repo + 1)->installed = 1; (repo + 1)->type = 2; (repo + 1)->ext_entry = 0;
}

// Загрузка пользовательского пакета из папки /packages/*.m95 на диске/флешке
void pkg_load_disk_package(void* data, uint32_t size) {
    if (!data || size < 60 || repo_count >= 12) return;
    uint8_t* raw = (uint8_t*)data;
    if (raw[0] != 'M' || raw[1] != '9' || raw[2] != '5' || raw[3] != 'P') return;

    uint32_t load_addr = 0x00500000 + ext_code_offset;
    uint8_t* dst = (uint8_t*)load_addr;
    uint32_t code_len = size - 52;
    for (uint32_t i = 0; i < code_len; i++) *(dst + i) = *(raw + 52 + i);
    ext_code_offset += (code_len + 255) & ~255;

    M95Package* p = repo + repo_count++;
    str_copy_n(p->name,   (const char*)(raw + 4), 24);
    str_copy_n(p->author, (const char*)(raw + 28), 24);
    p->installed = 1;
    p->type = 3;
    p->ext_entry = (void (*)(int,int,int,int,int,int,int,char))load_addr;
}

int pkg_command(const char* cmd, char* out_buf, int* out_len, int max_len) {
    if (str_eq(cmd, "m95pkg") || str_eq(cmd, "pkg help")) {
        str_append(out_buf, out_len, "m95pkg list | install <name> | remove <name> | run <name>\n", max_len);
        return 1;
    }
    if (str_eq(cmd, "m95pkg list") || str_eq(cmd, "pkg list")) {
        for (int i = 0; i < repo_count; i++) {
            str_append(out_buf, out_len, (repo + i)->installed ? "[OK] " : "[  ] ", max_len);
            str_append(out_buf, out_len, (repo + i)->name, max_len);
            str_append(out_buf, out_len, " (by ", max_len);
            str_append(out_buf, out_len, (repo + i)->author, max_len);
            str_append(out_buf, out_len, ")\n", max_len);
        }
        return 1;
    }
    if (starts_with(cmd, "m95pkg install ")) {
        const char* arg = cmd + 15;
        for (int i = 0; i < repo_count; i++) if (str_eq(arg, (repo + i)->name)) {
            (repo + i)->installed = 1;
            str_append(out_buf, out_len, "Installed: ", max_len); str_append(out_buf, out_len, arg, max_len); str_append(out_buf, out_len, "\n", max_len);
            return 1;
        }
        str_append(out_buf, out_len, "Not found in /packages.\n", max_len); return 1;
    }
    if (starts_with(cmd, "m95pkg remove ")) {
        const char* arg = cmd + 14;
        for (int i = 0; i < repo_count; i++) if (str_eq(arg, (repo + i)->name)) {
            (repo + i)->installed = 0;
            if (running_idx == i) running_idx = -1;
            str_append(out_buf, out_len, "Removed: ", max_len); str_append(out_buf, out_len, arg, max_len); str_append(out_buf, out_len, "\n", max_len);
            return 1;
        }
        return 1;
    }
    if (starts_with(cmd, "m95pkg run ")) {
        const char* arg = cmd + 11;
        for (int i = 0; i < repo_count; i++) if (str_eq(arg, (repo + i)->name)) {
            if (!(repo + i)->installed) { str_append(out_buf, out_len, "Not installed!\n", max_len); return 1; }
            running_idx = i; return 2;
        }
        str_append(out_buf, out_len, "Unknown package.\n", max_len); return 1;
    }
    return 0;
}

void pkg_click(Window* win, MouseState* m, int prev_left) {
    if (!win->visible || !m->left_button || prev_left) return;
    int cx = win->x + 12, cy = win->y + 34;

    for (int i = 0; i < repo_count; i++) {
        int ry = cy + 22 + i * 28;
        if (m->y >= ry && m->y <= ry + 24) {
            if (m->x >= cx + 195 && m->x <= cx + 255) {
                (repo + i)->installed = !(repo + i)->installed;
                if (!(repo + i)->installed && running_idx == i) running_idx = -1;
            } else if (m->x >= cx + 260 && m->x <= cx + 315 && (repo + i)->installed) {
                running_idx = i;
            }
        }
    }
}

void pkg_render(Window* win, int lang) {
    int cx = win->x + 12, cy = win->y + 34;
    gui_draw_text(cx, cy, lang ? "Пакеты [EXP] (/packages/*.m95):" : "Packages [EXP] (/packages/*.m95):", WIN95_BLACK);

    for (int i = 0; i < repo_count; i++) {
        int ry = cy + 22 + i * 28;
        gui_draw_rect(cx, ry, 190, 24, WIN95_WHITE); gui_draw_bevel(cx, ry, 190, 24, 1);
        gui_draw_text(cx + 6, ry + 5, (repo + i)->name, 0x00000080);
        gui_draw_text(cx + 75, ry + 5, (repo + i)->author, WIN95_DARK_GRAY);

        gui_draw_button(cx + 195, ry, 60, 24, (repo + i)->installed ? (lang ? "Удал." : "Del") : (lang ? "Уст." : "Get"), (repo + i)->installed);
        if ((repo + i)->installed) gui_draw_button(cx + 260, ry, 55, 24, lang ? "Пуск" : "Run", 0);
    }
    gui_draw_text(cx, win->y + win->h - 22, lang ? "Кидайте .m95 в папку packages/ на диске" : "Drop .m95 files into packages/ on disk", WIN95_DARK_GRAY);
}

int pkg_app_is_running(void) { return running_idx != -1; }
const char* pkg_app_title(void) { return (running_idx >= 0) ? (repo + running_idx)->name : "M95 App"; }

static uint32_t rain_ticks = 0;

void pkg_app_render(Window* win, MouseState* m, int prev_left, char ch) {
    if (running_idx < 0) return;
    M95Package* p = repo + running_idx;
    int cx = win->x + 10, cy = win->y + 32, cw = win->w - 20, ch_h = win->h - 40;

    if (p->type == 1) { // xeyes: белые глаза со зрачками, следящими за курсором
        gui_draw_rect(cx, cy, cw, ch_h, WIN95_GRAY);
        for (int e = 0; e < 2; e++) {
            int ex = cx + 52 + e * 96;
            int ey = cy + 62;
            // Белок глаза
            gui_draw_rect(ex - 34, ey - 42, 68, 84, 0x00101010);
            gui_draw_rect(ex - 31, ey - 39, 62, 78, 0x00FFFFFF);

            int dx = m->x - ex;
            int dy = m->y - ey;
            int dist = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
            int px = ex, py = ey;
            if (dist > 0) {
                px += (dx * 16) / (dist > 25 ? dist : 25);
                py += (dy * 20) / (dist > 25 ? dist : 25);
            }
            gui_draw_rect(px - 8, py - 10, 16, 20, 0x00101010);
            gui_draw_rect(px - 4, py - 6, 4, 4, 0x00FFFFFF);
        }
    } else if (p->type == 2) { // rain: дождь из белых точек
        rain_ticks++;
        gui_draw_rect(cx, cy, cw, ch_h, 0x000A0C12);
        for (int i = 0; i < 45; i++) {
            int rx = cx + ((i * 37) % (cw - 6));
            int ry = cy + ((rain_ticks * (2 + (i % 3)) + i * 29) % (ch_h - 6));
            gui_draw_rect(rx, ry, 2, 4, 0x00FFFFFF);
        }
    } else if (p->type == 3 && p->ext_entry) { // Внешний код с диска/флешки!
        p->ext_entry(cx, cy, cw, ch_h, m->x, m->y, (m->left_button && !prev_left), ch);
    }
}
