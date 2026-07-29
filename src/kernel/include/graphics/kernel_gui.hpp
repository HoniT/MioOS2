// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef KERNEL_GUI_HPP
#define KERNEL_GUI_HPP

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <graphics/draw_util.hpp>

#define RGB_COLOR_BLACK        0x000000
#define RGB_COLOR_BLUE         0x0000AA
#define RGB_COLOR_GREEN        0x00AA00
#define RGB_COLOR_CYAN         0x00AAAA
#define RGB_COLOR_RED          0xAA0000
#define RGB_COLOR_MAGENTA      0xAA00AA
#define RGB_COLOR_BROWN        0xAA5500
#define RGB_COLOR_LIGHT_GRAY   0xAAAAAA
#define RGB_COLOR_DARK_GRAY    0x555555
#define RGB_COLOR_LIGHT_BLUE   0x5555FF
#define RGB_COLOR_LIGHT_GREEN  0x55FF55
#define RGB_COLOR_LIGHT_CYAN   0x55FFFF
#define RGB_COLOR_LIGHT_RED    0xFF5555
#define RGB_COLOR_PINK         0xFF55FF
#define RGB_COLOR_YELLOW       0xFFFF55
#define RGB_COLOR_WHITE        0xFFFFFF

#define FONT_HEIGHT 8
#define FONT_WIDTH 8

namespace gui
{
    // Structs
    
    /// @brief Printing types/modes
    enum PrintTypes {
        STD_PRINT,
        LOG_INFO,
        LOG_WARNING,
        LOG_ERROR
    };
    
    /// @brief Text column and row in a specific section
    struct txt_coords {
        size_t col;
        size_t row;
    };
    
    /// @brief Represents a box section that text can be written to
    struct txt_section {
        // Pixel boundaries
        uint32_t startX;
        uint32_t startY;
        uint32_t endX;
        uint32_t endY;
    
        // Text grid configuration
        size_t max_cols;
        size_t max_rows;
        
        // State
        txt_coords curr_cords;
        RGBAColor bg_color;
        RGBAColor fg_color;

        txt_section() {};
    
        txt_section(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, uint8_t font_width, uint8_t font_height) :
            startX(startX), startY(startY), endX(endX), endY(endY), curr_cords{0, 0} 
        {
            max_cols = (endX - startX) / font_width;
            max_rows = (endY - startY) / font_height;
            bg_color = RGB_COLOR_BLACK;
            fg_color = RGB_COLOR_WHITE;
        }
    };
    
    // Active section management for variadic functions
    void set_active_section(txt_section sect);
    txt_section get_active_section();

    void init_kernel_gui();

    class KernelGUI {
    private:
        static txt_section active_gui_section;
        static txt_section default_gui_section;
        
    public:
        static inline bool initialized = false;
        static void initialize();

        static void set_active_gui_section(txt_section* sect);
        static void set_default_gui_section() { active_gui_section = default_gui_section; };
        static txt_section* get_active_gui_section();

        static void kputchar(RGBAColor color, const char c, gui::txt_section& sect = active_gui_section);
        static void kputs(RGBAColor color, const char* str, gui::txt_section& sect = active_gui_section);
    };
} // namespace gui


gui::txt_coords kprintf(const char* fmt, ...);
gui::txt_coords kprintf(const RGBAColor color, const char* fmt, ...);
gui::txt_coords kprintf(const gui::PrintTypes print_type, const char* fmt, ...);
gui::txt_coords kprintf(const gui::PrintTypes print_type, const RGBAColor color, const char* fmt, ...);

// Core engine
void kvprintf(gui::txt_section* sect, gui::PrintTypes print_type, RGBAColor color, const char* fmt, va_list args);

#endif // KERNEL_GUI_HPP
