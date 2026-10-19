#!/usr/bin/env python3
"""Package a real x64 Windows VST3 binary, without modifying the build output."""
import argparse
import hashlib
import shutil
import struct
import zipfile
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--build', default='build-win')
p.add_argument('--sdk', default='/workspace/dependencies/vst3sdk')
p.add_argument('--output', default='dist')
args = p.parse_args()
root = Path(__file__).resolve().parents[1]
build = Path(args.build).resolve()
candidates = [x for x in (build / 'VST3').rglob('JerzyAtomicCluster.vst3') if x.is_file()]
if len(candidates) != 1:
    raise SystemExit(f'Expected one compiled Windows VST3, found {len(candidates)}')
binary = candidates[0]
data = binary.read_bytes()
if data[:2] != b'MZ':
    raise SystemExit('Not a Windows binary')
pe = struct.unpack_from('<I', data, 0x3C)[0]
if data[pe:pe+4] != b'PE\0\0' or struct.unpack_from('<H', data, pe+4)[0] != 0x8664:
    raise SystemExit('Expected Windows x64 PE binary')
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)
bundle = out / 'JerzyAtomicCluster.vst3'
# Older local packaging placed a flat DLL here. It is a generated output.
if bundle.is_file():
    bundle.unlink()
destination = bundle / 'Contents/x86_64-win/JerzyAtomicCluster.vst3'
destination.parent.mkdir(parents=True, exist_ok=True)
(bundle / 'Contents/Resources').mkdir(parents=True, exist_ok=True)
shutil.copy2(binary, destination)
native_metadata = binary.parent.parent / 'Resources/moduleinfo.json'
if native_metadata.is_file():
    shutil.copy2(native_metadata, bundle / 'Contents/Resources/moduleinfo.json')
sdk = Path(args.sdk).resolve()
archive = out / 'JerzyAtomicClusterVST3-Win64.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for file in sorted(bundle.rglob('*')):
        if file.is_file():
            z.write(file, file.relative_to(out))
    z.write(root / 'README.md', 'README.md')
    if (root / 'docs/VALIDATION.md').exists():
        z.write(root / 'docs/VALIDATION.md', 'VALIDATION.md')
    for part in ('', 'base', 'pluginterfaces', 'public.sdk'):
        license_file = sdk / part / 'LICENSE.txt'
        if not license_file.is_file():
            raise SystemExit(f'Missing SDK license: {license_file}')
        z.write(license_file, f'licenses/Steinberg-{part or "SDK"}.txt')
    mingw = Path('/workspace/toolchains/mingw/usr/share/doc')
    gcc_notice = mingw / 'gcc-mingw-w64-base/copyright'
    if gcc_notice.exists():
        z.write(gcc_notice, 'licenses/GCC-runtime-notices.txt')
        z.write(mingw / 'mingw-w64-common/copyright', 'licenses/MinGW-notices.txt')
        z.write('/usr/share/common-licenses/GPL-3', 'licenses/GPL-3.txt')
checksums = '\n'.join(f'{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.relative_to(out)}'
                      for f in (destination, archive)) + '\n'
(out / 'SHA256SUMS.txt').write_text(checksums)
print(archive)
print(checksums, end='')
