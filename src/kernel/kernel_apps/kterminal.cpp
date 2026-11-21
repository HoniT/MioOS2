// ========================================
// Copyright Ioane Baidoshvili 2026.
// Distributed under the terms of the MIT License.
//
// A simple kernel terminal loop for basic I/O with the OS. Nothing fancy
// ========================================

#include <kernel_apps/kterminal.hpp>
#include <drivers/ps2/keyboard.hpp>
#include <kernel_ui.hpp>
#include <kernel_main.hpp>
#include <lib/string_util.hpp>
#include <timekeeping.hpp>
#include <lib/time_util.hpp>
#include <lib/mem_util.hpp>
#include <cpu.hpp>
#include <registry/system_topology_registry.hpp>
#include <mm/pmm.hpp>

util::String KTerminal::curr_input{};

cmnd_info_t KTerminal::cmnd_infos[] = {
    cmnd_info_t("sysfetch", sysfetch),
    cmnd_info_t("help", help)
};

static void new_terminal_line() {
    kprintf(RGB_COLOR_DARK_GRAY, "\nroot");
    kprintf(RGB_COLOR_LIGHT_GRAY, "@mio-os$ ");
}

void KTerminal::start_terminal() {
    kprintf("\n\n");

    new_terminal_line();

    while (true) {
        key_event_t ev = KeyboardDriver::get_instance()->get_curr_key_event();
        if (!ev.is_press) continue;

        if (ev.keycode == KeyCode::Enter) {
            kprintf("\n");
            if (!curr_input.empty()) {
                parse_and_run(curr_input);
                curr_input.clear();
            }
            new_terminal_line();
            continue;
        }

        if (ev.keycode == KeyCode::Backspace) {
            if (!curr_input.empty()) {
                curr_input.pop_back();
                kprintf("\b");
            }
            continue;
        }

        if (ev.ascii != '\0') {
            curr_input.push_back(ev.ascii);
            kprintf("%c", ev.ascii);
        }
    }
}

void KTerminal::parse_and_run(const util::String& input) {
    util::List<util::String> tokens;
    util::String current_token;

    for (size_t i = 0; i < input.size(); i++) {
        char c = input[i];
        if (c == ' ') {
            if (!current_token.empty()) {
                tokens.push_back(util::move(current_token));
            }
        } else {
            current_token.push_back(c);
        }
    }
    if (!current_token.empty()) {
        tokens.push_back(util::move(current_token));
    }

    if (tokens.empty()) return;

    if (!exec_cmnd(tokens[0], tokens)) {
        kprintf(gui::LOG_ERROR, "Unknown command: %s\n", tokens[0].c_str());
    }
}

bool KTerminal::exec_cmnd(const util::String& cmnd, const util::List<util::String>& args) {
    for (const cmnd_info_t& info : cmnd_infos) {
        if (strcmp(info.cmnd_name, cmnd.c_str()) == 0) {
            return info.cmnd_function(args);
        }
    }
    return false;
}   

bool KTerminal::sysfetch(const util::List<util::String>& args) {
    kprintf("OS: %s\n", OS_NAME);
    kprintf("Kernel: %s\n\n", KERNEL_VERSION);

    time_units units = get_time_units_from_ns(KernelTime::get_monotonic_ns());
    kprintf("Uptime: %uh %um %us\n", units.hours, units.minutes, units.seconds);
    kprintf("CPU: %s with %u cores\n", cpu::CPU::bsp_name, SystemTopology::ap_cpus.size() + 1);

    size_units total_mem = get_size_units_from_bytes(mem::PMM::get_total_memory());
    size_units used_mem = get_size_units_from_bytes(mem::PMM::get_used_memory());
    kprintf("RAM: Total- %uGiB %uMiB %uKiB   Used- %uGiB %uMiB %uKiB",
        static_cast<uint64_t>(total_mem.gib), static_cast<uint64_t>(total_mem.mib), static_cast<uint64_t>(total_mem.kib),
        static_cast<uint64_t>(used_mem.gib),  static_cast<uint64_t>(used_mem.mib),  static_cast<uint64_t>(used_mem.kib));

    return true;
}

bool KTerminal::help(const util::List<util::String>& args) {
    kprintf("Available commands:\n\n");
    
    for (cmnd_info_t cmd : cmnd_infos)
        kprintf("- %s\n", cmd.cmnd_name);
    
    return true;
}