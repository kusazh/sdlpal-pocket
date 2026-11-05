#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
make desktop
mkdir -p ../../build
smoke_dir=$(mktemp -d "$(cd ../../build && pwd)/smoke.XXXXXX")
ln -s "$(pwd)/pal.pak" "$smoke_dir/pal.pak"
ln -s "$(pwd)/pal-fm.pcm" "$smoke_dir/pal-fm.pcm"
app="$(pwd)/pal-desktop"
cd "$smoke_dir"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$app" --smoke-test
echo "Smoke artifacts: $smoke_dir"
