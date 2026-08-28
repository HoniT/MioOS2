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
        RGBAColor fg_color;

        txt_section() {};
    
        txt_section(uint32_t startX, uint32_t startY, uint32_t endX, uint32_t endY, uint8_t font_width, uint8_t font_height) :
            startX(startX), startY(startY), endX(endX), endY(endY), curr_cords{0, 0} 
        {
            max_cols = (endX - startX) / font_width;
            max_rows = (endY - startY) / font_height;
            fg_color = RGB_COLOR_WHITE;
        }
    };

    class KernelGUI {
    private:
        static txt_section active_gui_section;
        static txt_section default_gui_section;
        
    public:
        static inline bool initialized = false;
        static void initialize();

        static void set_active_gui_section(txt_section* sect);
        static txt_section* get_active_gui_section();
        static void set_default_gui_section_as_active() { active_gui_section = default_gui_section; };
        static txt_section* get_default_gui_section() { return &default_gui_section; }

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
