"""Render the released Windows DLL: dry, density, patterns, transitions, stereo."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import wave
import zipfile
import numpy as np
root=Path(__file__).resolve().parents[1]
folder=root/'audio-examples/0.2.0'; folder.mkdir(parents=True,exist_ok=True)
source=root/'audio-examples/source_plucked.wav'
if not source.exists(): subprocess.run(['python3',str(root/'tools/make-demo-source.py')],cwd=root,check=True)
with zipfile.ZipFile(root/'releases/0.2.0/JerzyAtomicClusterVST3-Win64.zip') as z:
    binary=z.read('JerzyAtomicCluster.vst3/Contents/x86_64-win/JerzyAtomicCluster.vst3')
plugin=root/'build-win/demo-plugin-020.vst3'; plugin.write_bytes(binary)
# name, blend, capture ms, atoms, smooth, flat, pattern, stereo
presets=[('00_dry',0,200,16,0,0,0,0),
         ('01_one_atom',1,200,1,0,0,0,0),
         ('02_sixteen_atoms',1,200,16,0,0,0,0),
         ('03_sixtyfour_atoms',1,200,64,0,0,0,0),
         ('04_arpeggio_stereo',1,180,24,0,0,.5,1),
         ('05_random_harmonic',1,140,48,0,1,1,1),
         ('06_smooth_stereo',1,500,48,1,0,1,1),
         ('07_orbiting_cluster',1,300,64,1,0,0,1)]
manifest=dict(version='0.2.0',dll_sha256=hashlib.sha256(binary).hexdigest(),
              source='original synthetic plucked phrase, no uploaded reference media',
              source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),normalization=False,renders=[])
for name,blend,ms,atoms,smooth,flat,pattern,stereo in presets:
    wav=folder/(name+'.wav'); mp3=folder/(name+'.mp3')
    subprocess.run([os.environ['WINELOADER'],str(root/'build-win/render_audio.exe'),str(plugin),str(source),str(wav),
                    '.5',str(blend),str(ms),str(atoms),str(smooth),str(flat),str(pattern),str(stereo)],check=True)
    subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-i',str(wav),'-c:a','libmp3lame','-b:a','192k',str(mp3)],check=True)
    with wave.open(str(wav)) as f:
        assert (f.getnchannels(),f.getsampwidth(),f.getframerate(),f.getnframes())==(2,2,48000,576000)
        data=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').reshape(-1,2).astype(float)/32768
    assert np.isfinite(data).all() and 0<np.abs(data).max()<.999
    if name=='00_dry':
        with wave.open(str(source)) as f: original=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').astype(float)/32768
        assert np.abs(data[:len(original),0]-original).max()<=1/32768
        assert np.abs(data[len(original):]).max()==0
    if stereo: assert np.sqrt(np.mean((data[:,0]-data[:,1])**2))>.005
    manifest['renders'].append(dict(file=mp3.name,sha256=hashlib.sha256(mp3.read_bytes()).hexdigest(),
        blend=blend,capture_ms=ms,atoms=atoms,smooth=bool(smooth),flat=bool(flat),pattern=pattern,stereo=stereo,
        peak=float(np.abs(data).max()),rms=float(np.sqrt(np.mean(data**2))),
        side_rms=float(np.sqrt(np.mean((data[:,0]-data[:,1])**2)))))
(folder/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
with wave.open(str(folder/'comparison.wav'),'wb') as dst:
    dst.setnchannels(2);dst.setsampwidth(2);dst.setframerate(48000)
    for i,(name,*_) in enumerate(presets):
        with wave.open(str(folder/(name+'.wav'))) as src: dst.writeframes(src.readframes(src.getnframes()))
        if i<len(presets)-1: dst.writeframes(bytes(36000*4))
subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-i',str(folder/'comparison.wav'),'-c:a','libmp3lame','-b:a','192k',str(folder/'comparison.mp3')],check=True)
with zipfile.ZipFile(folder/'JerzyAtomicCluster-0.2.0-examples.zip','w',zipfile.ZIP_DEFLATED) as z:
    for file in [folder/'README.md',folder/'manifest.json',*sorted(folder.glob('*.mp3'))]: z.write(file,file.name)
print('Verified released DLL, eight 12-second renders, exact dry, audible stereo; comparison and ZIP ready.')
