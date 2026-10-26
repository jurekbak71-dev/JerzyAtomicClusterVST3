#!/usr/bin/env bash
# Non-root, signature-verified setup and cross-build for both Raspberry Pi ABIs.
set -euo pipefail
cd /workspace/JerzyAtomicClusterVST3
export PATH=/workspace/.atomic-cluster-tools/bin:$PATH
command -v cmake >/dev/null
root=/workspace/toolchains/lv2
if [ ! -x "$root/usr/bin/aarch64-linux-gnu-g++-10" ] || [ ! -x "$root/usr/bin/arm-linux-gnueabihf-g++-10" ] || [ ! -x "$root/usr/bin/qemu-aarch64" ] || [ ! -f "$root/usr/include/lv2/core/lv2.h" ]; then
  apt_root=/workspace/.atomic-lv2-apt
  mkdir -p "$apt_root"/{lists/partial,archives/partial,downloads} "$root"
  cat > "$apt_root/apt.conf" <<'CONF'
Dir::Etc::parts "-";
Dir::Etc::main "-";
Dir::Etc::sourcelist "/workspace/.atomic-lv2-apt/sources.list";
Dir::Etc::sourceparts "-";
Dir::State::lists "/workspace/.atomic-lv2-apt/lists";
Dir::Cache::archives "/workspace/.atomic-lv2-apt/archives";
APT::Sandbox::User "agent";
CONF
  cat > "$apt_root/sources.list" <<'SOURCES'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian bullseye main
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main
SOURCES
  export APT_CONFIG="$apt_root/apt.conf"
  apt-get update
  (
    cd "$apt_root/downloads"
    mapfile -t packages < /workspace/JerzyAtomicClusterVST3/tools/lv2-packages.txt
    apt-get download "${packages[@]}"
    for package in ./*.deb; do dpkg-deb -x "$package" "$root"; done
  )
fi
export LD_LIBRARY_PATH="$root/usr/lib/x86_64-linux-gnu${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cmake -S . -B build-lv2-native -G Ninja -DCMAKE_BUILD_TYPE=Release -DATOMIC_BUILD_PLUGIN=OFF -DATOMIC_BUILD_LV2=ON
cmake --build build-lv2-native --parallel 4
ctest --test-dir build-lv2-native --output-on-failure --no-tests=error
python3 tools/validate-lv2.py build-lv2-native/lv2/JerzyAtomicCluster.lv2
for arch in aarch64 armhf; do
  cmake -S . -B "build-lv2-$arch" -G Ninja -DCMAKE_BUILD_TYPE=Release -DATOMIC_BUILD_PLUGIN=OFF -DATOMIC_BUILD_LV2=ON -DCMAKE_TOOLCHAIN_FILE="cmake/rpi-$arch.cmake"
  cmake --build "build-lv2-$arch" --parallel 4
  ctest --test-dir "build-lv2-$arch" --output-on-failure --no-tests=error
  python3 tools/validate-lv2.py "build-lv2-$arch/lv2/JerzyAtomicCluster.lv2"
done
python3 tools/package-lv2.py
