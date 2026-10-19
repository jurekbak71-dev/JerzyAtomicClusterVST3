"""Concatenate the verified 0.1.2 renders without level changes."""
from pathlib import Path
import wave
import subprocess
folder=Path(__file__).resolve().parents[1]/'audio-examples/0.1.2'
names=['00_dry','01_sharp_sparse','02_smooth_hold','03_smooth_chords','04_flat_glitch','05_fast_cluster']
target=folder/'comparison.wav'
with wave.open(str(target),'wb') as dst:
    dst.setnchannels(2);dst.setsampwidth(2);dst.setframerate(48000)
    for i,name in enumerate(names):
        with wave.open(str(folder/(name+'.wav')),'rb') as src:
            assert (src.getnchannels(),src.getsampwidth(),src.getframerate(),src.getnframes())==(2,2,48000,576000)
            dst.writeframes(src.readframes(src.getnframes()))
        if i<len(names)-1: dst.writeframes(bytes(36000*4))
subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-y','-i',str(target),'-c:a','libmp3lame','-b:a','192k',str(folder/'comparison.mp3')],check=True)
print('Comparison: 75.75 seconds; six 12-second sections, 0.75-second gaps; no normalization.')
