#!/bin/bash

set -e
source "$(dirname "$0")/config.sh"

if [[ "$1" == "--quick" ]]; then
    rm -f "$ISO_FILE"
    rm -f "$KERNEL_ELF"
else
    bash ./scripts/clean.sh
    bash ./scripts/create_disks.sh
fi

make all

grub-mkrescue -o "$ISO_FILE" iso/
