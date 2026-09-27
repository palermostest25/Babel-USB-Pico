#!/bin/sh
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p "$project_dir/deps"

clone_tag() {
  url=$1
  tag=$2
  destination=$3
  if [ -d "$destination/.git" ]; then
    return
  fi
  if [ -e "$destination" ]; then
    echo "Refusing to replace existing $destination" >&2
    exit 1
  fi
  git clone --depth 1 --branch "$tag" "$url" "$destination"
}

clone_tag https://github.com/raspberrypi/pico-sdk.git 2.3.1 "$project_dir/deps/pico-sdk"
clone_tag https://github.com/hathach/tinyusb.git 0.21.0 "$project_dir/deps/tinyusb"

echo "Dependencies are ready."
