#!/bin/bash

if [[ "$1" == "--quick" ]]; then
    bash "$(dirname "$0")/build.sh" --quick
else
    bash "$(dirname "$0")/build.sh"
fi
bash "$(dirname "$0")/run.sh"