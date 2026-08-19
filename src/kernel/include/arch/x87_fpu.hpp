// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef X87_FPU_HPP
#define X87_FPU_HPP

#include <stdint.h>

namespace arch
{
    class X87_FPU {
    private:
        static uint32_t xsave_area_size;
        static uint32_t xsave_area_align;

        static bool initialized;
    public:
        static void initialize();
    };
} // namespace arch


#endif // X87_FPU_HPP
