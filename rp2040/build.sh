#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$project_dir/bootstrap.sh"
cmake -S "$project_dir" -B "$project_dir/build" -DPICO_BOARD=pico
cmake --build "$project_dir/build" --parallel
echo "UF2: $project_dir/build/babel_usb.uf2"
