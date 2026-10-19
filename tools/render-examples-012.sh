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
python3 tools/make-demo-source.py
# The DLL is extracted from the exact user-facing release ZIP.
python3 - <<'PY'
import zipfile
from pathlib import Path
with zipfile.ZipFile('releases/0.1.2/JerzyAtomicClusterVST3-Win64.zip') as z:
    binary=z.read('JerzyAtomicCluster.vst3/Contents/x86_64-win/JerzyAtomicCluster.vst3')
Path('build-win/demo-plugin.vst3').write_bytes(binary)
PY
render() {
  "$WINELOADER" "$PWD/build-win/render_audio.exe" "$PWD/build-win/demo-plugin.vst3" "$PWD/audio-examples/source_plucked.wav" "$PWD/audio-examples/0.1.2/$1.wav" .5 "$2" "$3" "$4" "$5" "$6"
}
mkdir -p audio-examples/0.1.2
render 00_dry 0 100 4 0 0
render 01_sharp_sparse 1 160 4 0 0
render 02_smooth_hold 1 900 6 1 0
render 03_smooth_chords .85 450 18 1 0
render 04_flat_glitch 1 300 3 0 1
render 05_fast_cluster .9 28 48 1 0
for audio in audio-examples/0.1.2/0*.wav; do
  ffmpeg -hide_banner -loglevel error -y -i "$audio" -c:a libmp3lame -b:a 192k "${audio%.wav}.mp3"
done
