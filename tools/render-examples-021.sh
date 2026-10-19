#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export WINEPREFIX=/workspace/.atomic-wine-v5
export WINELOADER=/workspace/toolchains/wine/usr/lib/wine/wine64
export WINEDLLPATH=/workspace/toolchains/wine/usr/lib/x86_64-linux-gnu/wine
export LD_LIBRARY_PATH=/workspace/toolchains/wine/usr/lib/x86_64-linux-gnu/wine/x86_64-unix
export XDG_CACHE_HOME=/workspace/.atomic-cache
export WINEDEBUG=-all
server=/workspace/toolchains/wine/usr/lib/wine/wineserver64
"$server" -k || true
"$server" -w
display_file=$(mktemp /tmp/atomic-audio-display.XXXXXX)
/workspace/toolchains/wine/usr/bin/Xvfb -displayfd 3 -screen 0 1024x900x24 -nolisten tcp -ac 3>"$display_file" >/tmp/atomic-audio-xvfb.log 2>&1 &
xvfb_pid=$!
trap '"$server" -k || true; "$server" -w; kill "$xvfb_pid" 2>/dev/null || true; rm -f "$display_file"' EXIT
for attempt in {1..100}; do
  [ -s "$display_file" ] && break
  kill -0 "$xvfb_pid"
  sleep 0.1
done
[ -s "$display_file" ]
export DISPLAY=":$(cat "$display_file")"
python3 tools/render-examples-021.py
