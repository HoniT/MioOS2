#!/bin/bash

source "$(dirname "$0")/config.sh"

USE_KVM=true
DEBUG_MODE=false

for arg in "$@"; do
    if [[ "$arg" == "--no-kvm" ]]; then
        USE_KVM=false
    elif [[ "$arg" == "--debug" ]]; then
        DEBUG_MODE=true
        USE_KVM=false
    fi
done

QEMU_ARGS=(
  "-m" "$RAM_AMOUNT"
  "-serial" "stdio"
  "-drive" "file=$ISO_FILE,format=raw,if=ide,index=0"
  "-boot" "d"
)

if [[ -f "$MAIN_IMG" ]]; then
    QEMU_ARGS+=(
        "-drive" "file=$MAIN_IMG,format=raw,if=ide,index=1"
    )
fi

if [[ -f "$EXTRA_IMG" ]]; then
    QEMU_ARGS+=(
        "-device" "ahci,id=ahci0"
        "-drive" "file=$EXTRA_IMG,format=raw,if=none,id=drive0"
        "-device" "ide-hd,drive=drive0,bus=ahci0.0"
    )
fi

HOST_ARCH=$(uname -m)

echo "--- KVM CHECK ---"
echo "USE_KVM: $USE_KVM"
echo "HOST_ARCH: $HOST_ARCH"
if [ -e "/dev/kvm" ]; then
    ls -l /dev/kvm
    [ -r "/dev/kvm" ] && echo "/dev/kvm is READABLE" || echo "/dev/kvm is NOT READABLE"
    [ -w "/dev/kvm" ] && echo "/dev/kvm is WRITABLE" || echo "/dev/kvm is NOT WRITABLE"
else
    echo "/dev/kvm DOES NOT EXIST"
fi
if [[ "$USE_KVM" == true ]] && [[ "$HOST_ARCH" == "x86_64" ]] && [[ -c "/dev/kvm" ]] && [[ -w "/dev/kvm" ]]; then
    echo "KVM checks passed! Enabling hardware acceleration."
    QEMU_ARGS+=("-enable-kvm" "-cpu" "host") 
else
    echo "KVM checks failed. Falling back to software emulation."
fi
echo "-----------------------"

if [[ "$DEBUG_MODE" == true ]]; then
    QEMU_ARGS+=("-s" "-S")
    "$QEMU_CMD" "${QEMU_ARGS[@]}" &

    while ! nc -z localhost 1234 2>/dev/null; do
        sleep 0.1
    done
else
    "$QEMU_CMD" "${QEMU_ARGS[@]}"
fi