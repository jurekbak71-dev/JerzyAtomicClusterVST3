#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
export WINEPREFIX=/workspace/.atomic-wine-v5
export WINELOADER=/workspace/toolchains/wine/usr/lib/wine/wine64
export WINEDLLPATH=/workspace/toolchains/wine/usr/lib/x86_64-linux-gnu/wine
export LD_LIBRARY_PATH=/workspace/toolchains/wine/usr/lib/x86_64-linux-gnu/wine/x86_64-unix
export XDG_CACHE_HOME=/workspace/.atomic-cache
export WINEDEBUG=-all
mkdir -p "$XDG_CACHE_HOME" build-win/validation
rm -f build-win/validation/{moduleinfo,dsp,processor,validator,editor}.log build-win/validation/editor.bmp
server=/workspace/toolchains/wine/usr/lib/wine/wineserver64
"$server" -k || true
"$server" -w
display_file=$(mktemp /tmp/atomic-display.XXXXXX)
/workspace/toolchains/wine/usr/bin/Xvfb -displayfd 3 -screen 0 1024x900x24 -nolisten tcp -ac 3>"$display_file" >build-win/validation/xvfb.log 2>&1 &
xvfb_pid=$!
trap '"$server" -k || true; "$server" -w; kill "$xvfb_pid" 2>/dev/null || true; rm -f "$display_file"' EXIT
for attempt in {1..100}; do
  [ -s "$display_file" ] && break
  kill -0 "$xvfb_pid"
  sleep 0.1
done
[ -s "$display_file" ] || { echo 'Xvfb did not become ready' >&2; exit 1; }
export DISPLAY=":$(cat "$display_file")"
wine_run() { "$WINELOADER" "$@"; }
python3 tools/package.py --build build-win
wine_run "$PWD/build-win/bin/moduleinfotool.exe" -create -version 0.1.0 -path "$PWD/dist/JerzyAtomicCluster.vst3" -output "$PWD/dist/JerzyAtomicCluster.vst3/Contents/Resources/moduleinfo.json" >build-win/validation/moduleinfo.log 2>&1
wine_run "$PWD/build-win/dsp_tests.exe" >build-win/validation/dsp.log 2>&1
wine_run "$PWD/build-win/processor_tests.exe" >build-win/validation/processor.log 2>&1
wine_run "$PWD/build-win/bin/validator.exe" "$PWD/dist/JerzyAtomicCluster.vst3" >build-win/validation/validator.log 2>&1
wine_run "$PWD/build-win/editor_smoke.exe" "$PWD/dist/JerzyAtomicCluster.vst3/Contents/x86_64-win/JerzyAtomicCluster.vst3" "$PWD/build-win/validation/editor.bmp" >build-win/validation/editor.log 2>&1
cat build-win/validation/dsp.log build-win/validation/processor.log build-win/validation/editor.log
tail -n 4 build-win/validation/validator.log
