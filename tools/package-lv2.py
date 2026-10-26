"""Package and inspect actual RPi LV2 binaries, with a combined installer."""
from pathlib import Path
import hashlib
import json
import re
import shutil
import struct
import subprocess
import zipfile
root=Path(__file__).resolve().parents[1]
release=root/'releases/lv2/0.2.1';release.mkdir(parents=True,exist_ok=True)
work=root/'dist-lv2/JerzyAtomicCluster-LV2-RPi4B';work.mkdir(parents=True,exist_ok=True)
report={'version':'0.2.1','plugin_uri':'https://jurekbak71-dev.github.io/JerzyAtomicCluster#effect','architectures':{}}
for arch,elfclass,machine,glibc in [('aarch64',2,183,(2,17)),('armhf',1,40,(2,4))]:
    source=root/f'build-lv2-{arch}/lv2/JerzyAtomicCluster.lv2'
    binary=source/'atomic_cluster.so';data=binary.read_bytes()
    assert data[:4]==b'\x7fELF' and data[4]==elfclass and data[5]==1
    assert struct.unpack_from('<H',data,18)[0]==machine
    if arch=='armhf': assert struct.unpack_from('<I',data,36)[0]&0x400,'hard-float ELF ABI required'
    versions=subprocess.check_output(['readelf','--version-info',str(binary)],text=True)
    required=sorted({tuple(map(int,v.split('.'))) for v in re.findall(r'GLIBC_([0-9.]+)',versions)})
    assert required and max(required)<=glibc,required
    dyn=subprocess.check_output(['readelf','-d',str(binary)],text=True)
    needed=re.findall(r'\(NEEDED\).*\[(.*?)\]',dyn)
    assert set(needed)<={'libc.so.6','libm.so.6','ld-linux-armhf.so.3'},needed
    symbols=subprocess.check_output(['readelf','-Ws',str(binary)],text=True)
    assert re.search(r'GLOBAL\s+DEFAULT\s+\d+\s+lv2_descriptor',symbols)
    target=work/arch/'JerzyAtomicCluster.lv2';shutil.copytree(source,target,dirs_exist_ok=True)
    for p in [target,*target.rglob('*')]: p.chmod(0o755 if p.is_dir() or p.name=='atomic_cluster.so' else 0o644)
    report['architectures'][arch]=dict(sha256=hashlib.sha256(data).hexdigest(),size=len(data),needed=needed,max_glibc='.'.join(map(str,max(required))))
    with zipfile.ZipFile(release/f'JerzyAtomicCluster-LV2-RPi4B-{arch}.zip','w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(target.rglob('*')):
            if p.is_file(): z.write(p,p.relative_to(target.parent))
        z.write(root/'docs/LV2_RPI4_PL.md','INSTALL_LV2_PL.md')
        z.write(root/'docs/LV2_VALIDATION.md','VALIDATION.md')
        z.write('/workspace/toolchains/lv2/usr/share/doc/gcc-10-aarch64-linux-gnu-base/copyright','licenses/GCC-runtime-notices.txt')
        z.write('/usr/share/common-licenses/GPL-3','licenses/GPL-3.txt')
shutil.copy2(root/'tools/install-lv2.sh',work/'install.sh')
shutil.copy2(root/'docs/LV2_RPI4_PL.md',work/'INSTALL_LV2_PL.md')
shutil.copy2(root/'docs/LV2_VALIDATION.md',work/'VALIDATION.md')
licenses=work/'licenses';licenses.mkdir(exist_ok=True)
shutil.copy2('/workspace/toolchains/lv2/usr/share/doc/gcc-10-aarch64-linux-gnu-base/copyright',licenses/'GCC-runtime-notices.txt')
shutil.copy2('/usr/share/common-licenses/GPL-3',licenses/'GPL-3.txt')
(work/'build-info.json').write_text(json.dumps(report,indent=2)+'\n')
for p in [work,*work.rglob('*')]: p.chmod(0o755 if p.is_dir() or p.name in ('atomic_cluster.so','install.sh') else 0o644)
archive=release/'JerzyAtomicCluster-LV2-RPi4B.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(work.rglob('*')):
        if p.is_file(): z.write(p,p.relative_to(work.parent))
(release/'build-info.json').write_text(json.dumps(report,indent=2)+'\n')
files=sorted(release.glob('*.zip'))
(release/'SHA256SUMS.txt').write_text(''.join(f'{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.name}\n' for p in files))
print(archive)
print(json.dumps(report,indent=2))
