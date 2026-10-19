"""Verify and package the 0.1.2 renders of the actual released Windows DLL."""
import hashlib
import json
import wave
import zipfile
from pathlib import Path
import numpy as np
root = Path(__file__).resolve().parents[1]
folder = root / 'audio-examples/0.1.2'
settings = [('00_dry',0,100,4,0,0), ('01_sharp_sparse',1,160,4,0,0),
            ('02_smooth_hold',1,900,6,1,0), ('03_smooth_chords',.85,450,18,1,0),
            ('04_flat_glitch',1,300,3,0,1), ('05_fast_cluster',.9,28,48,1,0)]
with zipfile.ZipFile(root/'releases/0.1.2/JerzyAtomicClusterVST3-Win64.zip') as z:
    binary=z.read('JerzyAtomicCluster.vst3/Contents/x86_64-win/JerzyAtomicCluster.vst3')
assert binary == (root/'build-win/demo-plugin.vst3').read_bytes()
manifest={'version':'0.1.2','dll_sha256':hashlib.sha256(binary).hexdigest(),
          'source':'original synthetic plucked phrase; no uploaded reference media', 'renders':[]}
for name,blend,period,atoms,smooth,flat in settings:
    with wave.open(str(folder/(name+'.wav'))) as f:
        assert (f.getframerate(),f.getnchannels(),f.getnframes())==(48000,2,576000)
        samples=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').reshape(-1,2)
    assert 0 < np.max(np.abs(samples.astype(float))) < 32760
    if name=='00_dry':
        with wave.open(str(root/'audio-examples/source_plucked.wav')) as f:
            source=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2')
        assert np.max(np.abs(samples[:len(source),0].astype(int)-source))<=1
        assert np.max(np.abs(samples[len(source):]))==0
    mp3=folder/(name+'.mp3')
    manifest['renders'].append(dict(file=mp3.name,sha256=hashlib.sha256(mp3.read_bytes()).hexdigest(),
        blend=blend,refresh_ms=period,atoms=atoms,smooth=bool(smooth),flat=bool(flat),
        peak=float(np.max(np.abs(samples.astype(float)))/32768),
        rms=float(np.sqrt(np.mean((samples.astype(float)/32768)**2)))))
(folder/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
with zipfile.ZipFile(folder/'JerzyAtomicCluster-0.1.2-examples.zip','w',zipfile.ZIP_DEFLATED) as z:
    for file in [folder/'README.md',folder/'manifest.json',*sorted(folder.glob('*.mp3'))]:
        z.write(file,file.name)
print('Verified six 12-second renders, released DLL identity, dry alignment, levels; MP3 ZIP ready.')
