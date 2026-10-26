#!/usr/bin/env bash
# Install only the plugin bundle. Never changes audio routing or host services.
set -euo pipefail
base=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
target=${1:-}
if [[ "$target" != modep && "$target" != zynthian && "$target" != user ]]; then
  echo "Usage: bash install.sh modep|zynthian|user [--prefix DIR] [--arch aarch64|armhf] [--dry-run]" >&2
  exit 64
fi
shift
case "$target" in
  modep) prefix=/usr/modep/lv2 ;;
  zynthian) prefix=/usr/local/lib/lv2 ;;
  user) prefix="$HOME/.lv2" ;;
esac
arch=auto; dry_run=0
while (( $# )); do
  case "$1" in
    --prefix) [[ $# -ge 2 ]] || exit 64; prefix=$2; shift 2 ;;
    --arch) [[ $# -ge 2 ]] || exit 64; arch=$2; shift 2 ;;
    --dry-run) dry_run=1; shift ;;
    *) echo "Unknown option: $1" >&2; exit 64 ;;
  esac
done
if [[ "$arch" == auto ]]; then
  bits=$(getconf LONG_BIT)
  case "$(uname -m):$bits" in
    aarch64:64|arm64:64) arch=aarch64 ;;
    aarch64:32|arm*:32) arch=armhf ;;
    *) echo "This package targets Raspberry Pi ARM Linux (32 or 64 bit)." >&2; exit 65 ;;
  esac
fi
[[ "$arch" == aarch64 || "$arch" == armhf ]] || { echo 'Unsupported architecture' >&2; exit 65; }
source_bundle="$base/$arch/JerzyAtomicCluster.lv2"
[[ -f "$source_bundle/atomic_cluster.so" ]] || { echo "Missing bundle: $source_bundle" >&2; exit 66; }
printf 'Target: %s\nArchitecture: %s\nDestination: %s/JerzyAtomicCluster.lv2\n' "$target" "$arch" "$prefix"
(( dry_run )) && exit 0
mkdir -p -- "$prefix"
[[ -w "$prefix" ]] || { echo 'Directory not writable; use sudo for MODEP/Zynthian, or select --prefix.' >&2; exit 73; }
staging=$(mktemp -d "$prefix/.atomic-install.XXXXXX")
backup=
cleanup() {
  if [[ -n "$backup" && -d "$backup" && ! -e "$prefix/JerzyAtomicCluster.lv2" ]]; then
    mv -- "$backup" "$prefix/JerzyAtomicCluster.lv2" || true
  fi
  rm -rf -- "$staging"
}
trap cleanup EXIT
cp -a -- "$source_bundle" "$staging/JerzyAtomicCluster.lv2"
chmod -R a+rX -- "$staging/JerzyAtomicCluster.lv2"
if [[ -e "$prefix/JerzyAtomicCluster.lv2" ]]; then
  backup_dir="$prefix/../.atomic-cluster-backups"
  mkdir -p -- "$backup_dir"
  backup="$backup_dir/JerzyAtomicCluster.lv2.$(date -u +%Y%m%dT%H%M%SZ).$$"
  mv -- "$prefix/JerzyAtomicCluster.lv2" "$backup"
  echo "Previous bundle preserved at: $backup"
fi
mv -- "$staging/JerzyAtomicCluster.lv2" "$prefix/JerzyAtomicCluster.lv2"
echo 'Installed. Rescan LV2 plugins or restart the plugin host.'
if [[ "$target" == zynthian ]]; then echo 'Zynthian: rescan/enable Jerzy Atomic Cluster in Webconf, then add it as an audio effect.'; fi
if [[ "$target" == modep ]]; then echo 'MODEP: rescan or restart modep-mod-ui, then refresh the pedalboard browser.'; fi
