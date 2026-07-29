// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Kernel GUI util
// ========================================

#include <graphics/kernel_gui.hpp>
#include <graphics/draw_util.hpp>
#include <registry/output_registry.hpp>
#include <kernel_panic.hpp>
#include <lib/string_util.hpp>
#include <lib/mem_util.hpp>

using namespace gui;

txt_section KernelGUI::active_gui_section = txt_section();
txt_section KernelGUI::default_gui_section = txt_section();

void KernelGUI::set_active_gui_section(txt_section* sect) { active_gui_section = *sect; }
txt_section* KernelGUI::get_active_gui_section() { return &active_gui_section; }



void KernelGUI::initialize() {
    FramebufferDriver* fb = OutputRegistry::get_framebuffer();
    if(!fb) kprintf(PrintTypes::LOG_ERROR, "Couldn't get a framebuffer driver object to init kernel GUI\n");
    // Nothing specific with font_size * 3, just I add some padding for beauty and to make sure it's not invalid
    if(fb->get_screen_width() <= FONT_WIDTH * 3 || fb->get_screen_height() <= FONT_WIDTH * 3)
        kernel_panic("Invalid framebuffer resolution to set up kernel GUI\n");

    // A little beauty :)
    draw_line(FONT_WIDTH, FONT_HEIGHT, FONT_WIDTH, fb->get_screen_height() - FONT_HEIGHT, RGB_COLOR_WHITE);
    draw_line(fb->get_screen_width() - FONT_WIDTH, FONT_HEIGHT, fb->get_screen_width() - FONT_WIDTH, fb->get_screen_height() - FONT_HEIGHT, RGB_COLOR_WHITE);
    draw_line(FONT_WIDTH, FONT_HEIGHT, fb->get_screen_width() - FONT_WIDTH, FONT_HEIGHT, RGB_COLOR_WHITE);
    draw_line(FONT_WIDTH, fb->get_screen_height() - FONT_HEIGHT, fb->get_screen_width() - FONT_WIDTH, fb->get_screen_height() - FONT_HEIGHT, RGB_COLOR_WHITE);

    default_gui_section = txt_section(FONT_WIDTH * 2, FONT_HEIGHT * 2, 
        fb->get_screen_width() - FONT_WIDTH * 2, fb->get_screen_height() - FONT_HEIGHT * 2, FONT_WIDTH, FONT_HEIGHT);
    set_default_gui_section();

    initialized = true;
}



void KernelGUI::kputchar(RGBAColor color, const char c, txt_section& sect) {
    if(c == '\n') {
        sect.curr_cords.col = 0;
        sect.curr_cords.row++;
    }
    else {
        uint32_t px = sect.startX + (sect.curr_cords.col * FONT_WIDTH);
        uint32_t py = sect.startY + (sect.curr_cords.row * FONT_HEIGHT);
        
        draw_char(px, py, c, color);
        
        sect.curr_cords.col++;
        
        // Line wrap
        if (sect.curr_cords.col >= sect.max_cols) {
            sect.curr_cords.col = 0;
            sect.curr_cords.row++;
        }
    }

    if (sect.curr_cords.row >= sect.max_rows) {
        FramebufferDriver* fb = OutputRegistry::get_framebuffer();
        if (fb) {
            // Adjust these getters to match your FramebufferDriver methods
            uint8_t* fb_ptr = (uint8_t*)fb->get_buffer(); 
            uint32_t pitch = fb->get_pitch();
            uint32_t bpp = fb->get_bpp() / 8;
            
            uint32_t row_bytes = (sect.endX - sect.startX) * bpp;
            
            for (uint32_t y = sect.startY; y < sect.endY - FONT_HEIGHT; y++) {
                uint8_t* dest = fb_ptr + (y * pitch) + (sect.startX * bpp);
                uint8_t* src  = fb_ptr + ((y + FONT_HEIGHT) * pitch) + (sect.startX * bpp);
                
                memcpy(dest, src, row_bytes);
            }
            
            // Clear the bottom text row so new text isn't drawn over old text
            if (sect.bg_color == RGB_COLOR_BLACK) {
                for (uint32_t y = sect.endY - FONT_HEIGHT; y < sect.endY; y++) {
                    uint8_t* dest = fb_ptr + (y * pitch) + (sect.startX * bpp);
                    memset(dest, 0, row_bytes);
                }
            }
        }
        
        sect.curr_cords.row = sect.max_rows - 1;
    }
}

void KernelGUI::kputs(RGBAColor color, const char* str, txt_section& sect) {
    for (int i = 0; str[i] != '\0'; i++) {
        kputchar(color, str[i], sect);
    }
}


txt_coords kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    txt_section* active_section = KernelGUI::get_active_gui_section();

    kvprintf(active_section, STD_PRINT, active_section->fg_color, fmt, args);
    va_end(args);
    return active_section->curr_cords;
}

txt_coords kprintf(const RGBAColor color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    txt_section* active_section = KernelGUI::get_active_gui_section();

    kvprintf(active_section, STD_PRINT, color, fmt, args);
    va_end(args);
    return active_section->curr_cords;
}

txt_coords kprintf(const PrintTypes print_type, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    txt_section* active_section = KernelGUI::get_active_gui_section();

    kvprintf(active_section, print_type, active_section->fg_color, fmt, args);
    va_end(args);
    return active_section->curr_cords;
}

txt_coords kprintf(const PrintTypes print_type, const RGBAColor color, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    txt_section* active_section = KernelGUI::get_active_gui_section();

    kvprintf(active_section, print_type, color, fmt, args);
    va_end(args);
    return active_section->curr_cords;
}


// Core engine
void kvprintf(txt_section* sect, PrintTypes print_type, RGBAColor color, const char* fmt, va_list args) {
    if(!gui::KernelGUI::initialized) {
        char buf[1024];
        sprintf(buf, fmt, args);
        OutputRegistry::get_serial_logger()->write(buf);
        return;
    }

    // Print prefix based on print type
    switch (print_type) {
        case LOG_INFO:    gui::KernelGUI::kputs(RGB_COLOR_LIGHT_GRAY, "[INFO] ", *sect); break;
        case LOG_WARNING: gui::KernelGUI::kputs(RGB_COLOR_YELLOW, "[WARN] ", *sect); break;
        case LOG_ERROR:   gui::KernelGUI::kputs(RGB_COLOR_RED, "[FAIL] ", *sect); break;
        case STD_PRINT:   break;
    }

    // Parse format string
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%' && fmt[i+1] != '\0') {
            i++;
            char tmp_buf[65];
            
            switch (fmt[i]) {
                case 'd':
                case 'i': {
                    int64_t val = va_arg(args, int64_t);
                    itoa(val, tmp_buf, 10);
                    gui::KernelGUI::kputs(color, tmp_buf, *sect);
                    break;
                }
                case 'u': {
                    uint64_t val = va_arg(args, uint64_t);
                    utoa(val, tmp_buf, 10);
                    gui::KernelGUI::kputs(color, tmp_buf, *sect);
                    break;
                }
                case 'x':
                case 'X': {
                    uint64_t val = va_arg(args, uint64_t);
                    utoa(val, tmp_buf, 16);
                    gui::KernelGUI::kputs(color, tmp_buf, *sect);
                    break;
                }
                case 's': {
                    const char* s = va_arg(args, const char*);
                    if (!s) s = "(null)";
                    gui::KernelGUI::kputs(color, s, *sect);
                    break;
                }
                case 'c': {
                    char c = (char)va_arg(args, int);
                    gui::KernelGUI::kputchar(color, c, *sect);
                    break;
                }
                case '%': {
                    gui::KernelGUI::kputchar(color, '%', *sect);
                    break;
                }
                default: {
                    // Unknown specifier, print literally
                    gui::KernelGUI::kputchar(color, '%', *sect);
                    gui::KernelGUI::kputchar(color, fmt[i], *sect);
                    break;
                }
            }
        } else {
            // Standard character
            gui::KernelGUI::kputchar(color, fmt[i], *sect);
        }
    }
}
