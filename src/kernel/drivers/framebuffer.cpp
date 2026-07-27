// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Graphics framebuffer driver
// ========================================

#include <drivers/framebuffer.hpp>
#include <mm/mm_defs.hpp>
#include <graphics/kprint.hpp>
#include <kernel_panic.hpp>
#include <mm/paging.hpp>
#include <lib/mem_util.hpp>

bool FramebufferDriver::initialize() {
    if(!fb_tag) {
        kernel_panic("FramebufferDriver", "Framebuffer tag not found!\n");
        return false;
    }
    if(fb_tag->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
        kernel_panic("FramebufferDriver", "Invalid framebuffer type!\n");
        return false;
    }

    // Use pitch to calculate actual memory footprint
    screen_width = fb_tag->framebuffer_width;
    screen_height = fb_tag->framebuffer_height;
    screen_pitch = fb_tag->framebuffer_pitch;
    screen_bpp = fb_tag->framebuffer_bpp;
    fb_size = screen_pitch * screen_height; 

    uint64_t phys_addr = fb_tag->framebuffer_addr;
    uint64_t virt_addr = phys_addr + mem::HHDM_BASE;
    framebuffer = (uint32_t*)virt_addr;

    // Align starting addresses down to nearest page
    uint64_t map_phys = align_down(phys_addr, mem::PAGE_SIZE);
    uint64_t map_virt = align_down(virt_addr, mem::PAGE_SIZE);
    
    // Calculate how many pages we need to cover the fb_size
    uint64_t bytes_to_map = fb_size + (phys_addr - map_phys); 
    uint64_t pages = (bytes_to_map + mem::PAGE_SIZE - 1) / mem::PAGE_SIZE; 

    for (uint64_t i = 0; i < pages; i++) {
        mem::PagingBackend::map_page(
            map_virt + (i * mem::PAGE_SIZE), 
            map_phys + (i * mem::PAGE_SIZE), 
            mem::PageFlags::Read | mem::PageFlags::Write
        );
    }

    klogf("FramebufferDriver", "Initialized a framebuffer driver\n");
    initialized = true;
    return true;
}

void FramebufferDriver::put_pixel(const uint32_t x, const uint32_t y, const uint32_t color) {
    if (!framebuffer) return;
    if (x >= screen_width || y >= screen_height) return;

    uint8_t* pixel = (uint8_t*)framebuffer + y * screen_pitch + x * (screen_bpp / 8);

    switch (screen_bpp) {
        // RGBA
        case 32:
            pixel[0] = (color >> 0) & 0xFF;   // Blue
            pixel[1] = (color >> 8) & 0xFF;   // Green  
            pixel[2] = (color >> 16) & 0xFF;  // Red
            pixel[3] = (color >> 24) & 0xFF;  // Alpha (or 0xFF for opaque)
            break;
        // RGB
        case 24:
            pixel[0] = (color >> 0) & 0xFF;   // Blue
            pixel[1] = (color >> 8) & 0xFF;   // Green
            pixel[2] = (color >> 16) & 0xFF;  // Red
            break;
        default:
            return;
    }
}
    
uint32_t FramebufferDriver::get_pixel(const uint32_t x, const uint32_t y) {
    if (!framebuffer) return 0;
    if (x >= screen_width || y >= screen_height) return 0;

    // Calculate the byte address
    uint8_t* pixel_addr = (uint8_t*)framebuffer + y * screen_pitch + x * (screen_bpp / 8);

    if (screen_bpp == 32) {
        uint32_t color = *(uint32_t*)pixel_addr;

        // Mask out alpha
        return color & 0x00FFFFFF; 
    }
    
    return *(uint32_t*)pixel_addr;
}
