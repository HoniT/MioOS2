// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include <drivers/driver.hpp>
#include <multiboot.hpp>
#include <stdint.h>

class FramebufferDriver : public Driver {
private:
    uint32_t* framebuffer = nullptr;
    uint32_t fb_size = 0;
    
    uint32_t screen_width = 0;
    uint32_t screen_height = 0;
    uint32_t screen_pitch = 0;
    uint8_t screen_bpp = 0; // Bits per pixel
    
    const multiboot_tag_framebuffer* fb_tag;
    
public:
    constexpr explicit FramebufferDriver(const multiboot_tag_framebuffer* fb_tag) : fb_tag(fb_tag) { }
    
    /// @brief Initializes the fb driver
    bool initialize() override;
    void start() override { }
    void stop() override { }
    
    /// @brief Sets a color to a given pixel
    /// @param x Pixel's X coordinate
    /// @param y Pixel's Y coordinate
    /// @param color Color to assign this pixel
    void put_pixel(const uint32_t x, const uint32_t y, const uint32_t color);
    
    /// @brief Returns a pixels color
    /// @param x Pixel's X coordinate
    /// @param y Pixel's Y coordinate
    uint32_t get_pixel(const uint32_t x, const uint32_t y);
};

#endif // FRAMEBUFFER_HPP
