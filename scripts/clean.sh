#!/bin/bash

source "$(dirname "$0")/config.sh"

# Remove generated disk images
rm -f "$ISO_FILE"

# Clean the source code binaries
make clean

# Remove log files or other temporary build artifacts if they exist
rm -rf iso/boot/*.elf
rm -rf *.log

echo "Project cleaned."