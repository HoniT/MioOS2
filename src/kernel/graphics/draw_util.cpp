// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel printing utility
// ========================================

#include <graphics/draw_util.hpp>
#include <registry/output_registry.hpp>
#include <lib/string_util.hpp>
#include <lib/math.hpp>

void gui::draw_char(uint32_t x, uint32_t y, char c, uint32_t color, uint8_t fb_index) {
    if (c < 32 || c > 126) return;

    int font_height = 8;
    int font_width = 8;
    FramebufferDriver& fb = *OutputRegistry::get_framebuffer();

    for (int row = 0; row < font_height; row++) {
        auto bits = font8x8_basic[(uint8_t)c - 32][row];

        for (int col = 0; col < font_width; col++) {
            if (bits & (1 << (font_width - 1 - col))) {
                // Later we'll get the fb_indexth framebuffer, but now we only save one fb in the registry
                fb.put_pixel(x + col, y + row, color);
            }
            else fb.put_pixel(x + col, y + row, 0);
        }
    }
}

void gui::draw_line(const uint32_t x0, const uint32_t y0, const uint32_t x1, const uint32_t y1, const uint32_t color) {
    FramebufferDriver& fb = *OutputRegistry::get_framebuffer();
    
    int dx = abs((int)x1 - (int)x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs((int)y1 - (int)y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    int x = x0;
    int y = y0;

    while (true) {
        fb.put_pixel(x, y, color);
        if (x == (int)x1 && y == (int)y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}