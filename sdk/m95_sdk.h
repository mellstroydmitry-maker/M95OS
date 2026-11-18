#ifndef M95_SDK_H
#define M95_SDK_H
#include <stdint.h>
typedef struct {
    void (*draw_rect)(int x, int y, int w, int h, uint32_t color);
    void (*draw_text)(int x, int y, const char* str, uint32_t color);
    void (*draw_button)(int x, int y, int w, int h, const char* label, int pressed);
    void (*draw_line)(int x0, int y0, int x1, int y1, uint32_t color);
    void (*fill_triangle)(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color);
} M95ApiTable;
static inline M95ApiTable* m95_get_api(void) { return (M95ApiTable*)0x00407000; }
#endif
