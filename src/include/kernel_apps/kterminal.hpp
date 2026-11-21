// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
// ========================================

#pragma once
#ifndef KTERMINAL_HPP
#define KTERMINAL_HPP

#include <util/string.hpp>
#include <util/list.hpp>

typedef bool (*cmnd)(const util::List<util::String>& args);

struct cmnd_info_t {
    const char* cmnd_name;
    cmnd cmnd_function;

    cmnd_info_t(const char* cmnd_name, cmnd cmnd_function):
        cmnd_name(cmnd_name), cmnd_function(cmnd_function) { }
};

/// @brief A simple terminal for basic I/O with the OS
class KTerminal {
private:
    static util::String curr_input;
    static cmnd_info_t cmnd_infos[];

    static bool exec_cmnd(const util::String& cmnd, const util::List<util::String>& args);
    static void parse_and_run(const util::String& input);

    static bool sysfetch(const util::List<util::String>& args);
    static bool help(const util::List<util::String>& args);

public:
    static void start_terminal();
};

#endif // KTERMINAL_HPP
