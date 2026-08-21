#!/bin/bash

source "$(dirname "$0")/config.sh"

# Remove generated disk images
rm -f "$ISO_FILE"

# Clean the source code binaries
make clean

# Remove log files or other temporary build artifacts if they exist
rm -rf iso/boot/*.elf
rm -rf *.log

read -p "Do you also want to delete the contents of env/? (y/n) " -n 1 -r
echo

if [[ $REPLY =~ ^[Yy]$ ]]; then
    rm -rf env/*
    echo "env/ contents deleted."
else
    echo "Skipped env/ cleanup."
fi

echo "Project cleaned."