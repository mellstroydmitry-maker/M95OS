#include "mouse.h"
#include "gui.h"
#include "keyboard.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void mouse_wait(uint8_t type) {
    uint32_t timeout = 100000;
    if (type == 0) {
        while (timeout--) { if ((inb(0x64) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((inb(0x64) & 2) == 0) return; }
    }
}

static void mouse_write(uint8_t data) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, data);
}

static uint8_t mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init(void) {
    uint8_t status;
    mouse_wait(1);
    outb(0x64, 0xA8);

    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();
}

static uint8_t packet_b0 = 0, packet_b1 = 0, packet_b2 = 0;
static uint8_t cycle = 0;

void mouse_poll(MouseState* state) {
    while (inb(0x64) & 1) {
        uint8_t status = inb(0x64);
        uint8_t data = inb(0x60);

        // Если бит 5 == 0, это байт от клавиатуры — отдаем его драйверу клавиатуры!
        if (!(status & 0x20)) {
            keyboard_handle_scancode(data);
            continue;
        }

        if (cycle == 0 && !(data & 0x08)) continue;

        if (cycle == 0) packet_b0 = data;
        else if (cycle == 1) packet_b1 = data;
        else if (cycle == 2) packet_b2 = data;
        cycle++;

        if (cycle == 3) {
            cycle = 0;
            state->left_button = packet_b0 & 0x01;
            state->right_button = (packet_b0 >> 1) & 0x01;

            int dx = packet_b1;
            int dy = packet_b2;

            if (packet_b0 & 0x10) dx |= 0xFFFFFF00;
            if (packet_b0 & 0x20) dy |= 0xFFFFFF00;

            state->x += dx;
            state->y -= dy;

            if (state->x < 0) state->x = 0;
            if (state->x >= SCREEN_W - 12) state->x = SCREEN_W - 12;
            if (state->y < 0) state->y = 0;
            if (state->y >= SCREEN_H - 19) state->y = SCREEN_H - 19;
        }
    }
}
