// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef FPU_HPP
#define FPU_HPP

#include <stdint.h>

namespace arch
{
    class FPU_X87 {
    private:
        uint32_t xsave_area_size;
        uint32_t xsave_area_align;

        bool initialized;
    public:
        void initialize();
    };
} // namespace arch


#endif // FPU_HPP
