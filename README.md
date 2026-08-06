<div align="center">

<h1 style="font-size: 3em; font-weight: bold; margin: 0;">MioOS</h1>

**A 64-bit hobby operating system for learning low-level systems programming**

*Built with C++ and x86_64 assembly*

[Features](#features) • [Installation](#installation-and-usage) • [Building](#building-and-running) • [Documentation](#documentation) • [Contributing](#contributing)

---

</div>

# Features

- **GRUB Bootloader** - Standard Multiboot2-compliant bootloader
- **64-bit Kernel** - Written in modern C++
- **Buddy Allocator** - A modern and efficient buddy allocator to manage physical memory
- **4-Level paging**
- **SLUB Allocator**

# Project Structure
```
MioOS/
├ docs/              # Additional documentation
├ scripts/           # Build and toolchain scripts
├ src/
│ ├── boot/            # Bootloader and GRUB configuration
│ ├── kernel/          # Core kernel implementation
│   ├── arch/          # x86_64 arch specific components (GDT, IDT, ...)
│   ├── drivers/       # Hardware drivers
│   ├── graphics/      # Graphics and visual output
│   ├── include/       # Include headers
│   │   ...
│   │   ├── lib/            # Library/Utilities
│   │   ├── registry/       # Kernel subsystem registries (object storage & management), acts like DI/state management
│   ├── mm/            # Memory management subsystems
```

# Installation and Usage

**1. Clone the repository**
```bash
git clone https://github.com/HoniT/MioOS_2.git
cd MioOS
```

**2. Install the toolchain**

For Debian-based distributions:
```bash
bash ./scripts/toolchain/debian_build.sh
```

For Arch Linux:
```bash
bash ./scripts/toolchain/arch_build.sh
```

For other distributions, Windows, and macOS, follow the [GCC Cross-Compiler Guide](https://wiki.osdev.org/GCC_Cross-Compiler) and install dependencies manually (I highly recommend using WSL for Windows and then following the Linux tutorial instead of MinGW or Cygwin).

# Building and Running

### Quick Start

Build and run with a single command:
```bash
bash ./scripts/build_run.sh
```
To only overwrite changed source code when building and run:
```bash
bash ./scripts/build_run.sh --quick
```

### Building options:
Regular build:
```bash
bash ./scripts/build.sh
```
To only overwrite changed source code when building:
```bash
bash ./scripts/build.sh --quick
```

### Running options:

Automatically uses KVM if available, falls back to standard QEMU otherwise:
```bash
bash ./scripts/run.sh
```

Run without KVM
```bash
bash ./scripts/run.sh --no-kvm
```

Run with GDB
```bash
bash ./scripts/run.sh --debug
```

#### You can change emulation settings in [config](scripts/config.sh)
#### You can manually clean the project with [clean.sh](scripts/clean.sh)
#### You can manually create disk images with [create_disks.sh](scripts/create_disks.sh)

# Contributing

Contributors are welcome to submit improvements and bug fixes! Please visit the [GitHub repository](https://github.com/HoniT/MioOS_2) to:

- Report issues
- Submit pull requests
- Suggest new features
- Improve documentation

# Documentation

Additional documentation and technical details can be found in the [`/docs/`](/docs/) directory.

# License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

# Credits

- **Project Author:** [Ioane Baidoshvili](https://github.com/HoniT)

---

<div align="center">


*Star this repository if you find it interesting!*

</div>