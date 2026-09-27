#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
build_dir="$project_dir/build-host"
mkdir -p "$build_dir"
cc -std=c11 -Wall -Wextra -Werror -pedantic \
  -I"$project_dir/src" \
  "$project_dir/src/babel.c" "$project_dir/tests/test_babel.c" \
  -o "$build_dir/test_babel"
"$build_dir/test_babel"

tmp_dir=$(mktemp -d)
trap 'rm -rf "$tmp_dir"' EXIT INT TERM
printf '\000\377hello' > "$tmp_dir/input.bin"
path=$(python3 "$project_dir/tools/file_to_path.py" "$tmp_dir/input.bin")
python3 "$project_dir/tools/path_to_file.py" "$path" "$tmp_dir/output.bin"
cmp "$tmp_dir/input.bin" "$tmp_dir/output.bin"
echo "tool round-trip passed"
