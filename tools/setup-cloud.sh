#!/usr/bin/env bash
# Reproduce the tested toolchain without root or modifications to SDK sources.
set -euo pipefail
cd /workspace/JerzyAtomicClusterVST3
python3 -m venv /workspace/.atomic-cluster-tools
/workspace/.atomic-cluster-tools/bin/python -m pip install --no-cache-dir --disable-pip-version-check cmake==3.31.6 ninja==1.11.1.4
export PATH=/workspace/.atomic-cluster-tools/bin:$PATH
sdk=/workspace/dependencies/vst3sdk
if [ ! -e "$sdk" ]; then
  mkdir -p /workspace/dependencies
  git clone --no-checkout https://github.com/steinbergmedia/vst3sdk.git "$sdk"
  git -C "$sdk" checkout --detach 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
fi
[ "$(git -C "$sdk" rev-parse HEAD)" = 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96 ] || { echo 'Preserve and investigate the different SDK revision.' >&2; exit 1; }
git -C "$sdk" submodule update --init --recursive
mingw=/workspace/toolchains/mingw
wine=/workspace/toolchains/wine
if [ ! -x "$mingw/usr/bin/x86_64-w64-mingw32-g++-posix" ] || [ ! -x "$wine/usr/lib/wine/wine64" ]; then
  apt_root=/workspace/.atomic-apt
  mkdir -p "$apt_root"/{parts,lists/partial,archives/partial,mingw-downloads,wine-downloads}
  cat >"$apt_root/apt.conf" <<'CONF'
Dir::Etc::parts "/workspace/.atomic-apt/parts";
Dir::Etc::main "-";
Dir::Etc::sourcelist "/workspace/.atomic-apt/sources.list";
Dir::Etc::sourceparts "-";
Dir::State::lists "/workspace/.atomic-apt/lists";
Dir::Cache::archives "/workspace/.atomic-apt/archives";
APT::Sandbox::User "agent";
CONF
  echo 'deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main' >"$apt_root/sources.list"
  export APT_CONFIG="$apt_root/apt.conf"
  /usr/bin/apt-get update
  (
    cd "$apt_root/mingw-downloads"
    /usr/bin/apt-get download \
      g++-mingw-w64-x86-64-posix=14.2.0-19+27+b1 \
      gcc-mingw-w64-x86-64-posix=14.2.0-19+27+b1 \
      gcc-mingw-w64-x86-64-posix-runtime=14.2.0-19+27+b1 \
      gcc-mingw-w64-base=14.2.0-19+27+b1 \
      binutils-mingw-w64-x86-64=2.44-3+12+b1 \
      mingw-w64-x86-64-dev=12.0.0-5 mingw-w64-common=12.0.0-5
    for package in ./*.deb; do dpkg-deb -x "$package" "$mingw"; done
  )
  (
    cd "$apt_root/wine-downloads"
    /usr/bin/apt-get download wine64=10.0~repack-6 libwine=10.0~repack-6 \
      libz-mingw-w64=1.3.1+dfsg-2 xvfb=2:21.1.16-1.3+deb13u3 \
      libxfont2=1:2.0.6-1+deb13u1 x11-xkb-utils=7.7+9 xkb-data=2.42-1
    for package in ./*.deb; do dpkg-deb -x "$package" "$wine"; done
  )
fi
# Relocate Debian's wineserver launcher and NLS lookup without patching binaries.
if [ ! -L "$wine/usr/lib/wine/wineserver" ]; then
  mv "$wine/usr/lib/wine/wineserver" "$wine/usr/lib/wine/wineserver.debian-wrapper"
  ln -s wineserver64 "$wine/usr/lib/wine/wineserver"
fi
[ -e "$wine/usr/share/wine/wine" ] || ln -s . "$wine/usr/share/wine/wine"
# Wine requires this native dependency while bootstrapping a fresh 64-bit prefix.
mkdir -p /workspace/.atomic-wine-v5/drive_c/windows/system32
zlib=/workspace/.atomic-wine-v5/drive_c/windows/system32/zlib1.dll
[ -e "$zlib" ] || ln -s "$wine/usr/x86_64-w64-mingw32/lib/zlib1.dll" "$zlib"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DVST3_SDK_ROOT="$sdk"
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
cmake -S . -B build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/mingw.cmake -DVST3_SDK_ROOT="$sdk"
cmake --build build-win --parallel 4
bash tools/test-windows.sh
python3 tools/package.py --build build-win --sdk "$sdk"
