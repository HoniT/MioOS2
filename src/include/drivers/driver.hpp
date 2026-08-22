// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// Base interface for all MioOS kernel drivers
// ========================================

#pragma once
#ifndef DRIVER_HPP
#define DRIVER_HPP

/// @brief Base interface for all kernel level drivers
class Driver {
public:
    bool initialized = false;
    virtual bool initialize() = 0;

    virtual void start() = 0;
    virtual void stop() = 0;
};

#endif // DRIVER_HPP
