"""Verify actual renders and package the five examples with their dry reference."""
from array import array
import hashlib
import json
import math
from pathlib import Path
import wave
import zipfile

root = Path('audio-examples')
settings = [
    ('00_dry', 0, 27.1, 23, False, False),
    ('01_single_atom', 1, 125, 1, False, False),
    ('02_rhythmic_glitch', .85, 250, 6, False, False),
    ('03_smooth_pad', 1, 600, 32, True, False),
    ('04_flat_robot', 1, 70, 4, False, True),
    ('05_dense_spectrum', 1, 12, 256, True, False),
]
manifest = {'plugin_version': '0.1.0', 'host': 'Actual Windows VST3 loaded under Wine',
            'sample_rate': 48000, 'duration_seconds': 12, 'latency_compensated_samples': 2048,
            'source': 'Original synthetic plucked-string phrase, seed 71; not a real guitar recording',
            'post_processing': 'PCM16 quantization and MP3 encoding only; no per-file normalization', 'files': []}
with zipfile.ZipFile('releases/0.1.0/JerzyAtomicClusterVST3-Win64.zip') as z:
    dll = z.read('JerzyAtomicCluster.vst3/Contents/x86_64-win/JerzyAtomicCluster.vst3')
manifest['plugin_sha256'] = hashlib.sha256(dll).hexdigest()
assert dll == Path('build-win/demo-plugin.vst3').read_bytes()
unique = set()
for name, blend, interval, atoms, smooth, flat in settings:
    wav = root / (name + '.wav')
    with wave.open(str(wav)) as w:
        assert (w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()) == (2, 2, 48000, 576000)
        samples = array('h', w.readframes(w.getnframes()))
    peak = max(abs(s) for s in samples)
    assert 0 < peak < 32760, f'Clipped or silent: {name}'
    unique.add(hashlib.sha256(wav.read_bytes()).hexdigest())
    assert (root / (name + '.mp3')).stat().st_size > 10000
    manifest['files'].append({'name': name, 'volume_linear': 1, 'blend': blend,
                              'refresh_ms': interval, 'atoms': atoms, 'smooth': smooth, 'flat': flat,
                              'peak_dbfs': round(20 * math.log10(peak / 32768), 2),
                              'wav_sha256': hashlib.sha256(wav.read_bytes()).hexdigest()})
assert len(unique) == 6, 'Renders must be distinct'
with wave.open(str(root / 'source_plucked.wav')) as w:
    source = array('h', w.readframes(w.getnframes()))
with wave.open(str(root / '00_dry.wav')) as w:
    dry = array('h', w.readframes(w.getnframes()))
assert max(abs(dry[2*i] - s) for i, s in enumerate(source)) <= 1, 'Dry reference misaligned'
assert not any(dry[len(source)*2:]), 'Dry reference has unexpected tail'
(root / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
archive = root / 'JerzyAtomicCluster-audio-examples.zip'
with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
    for p in sorted(root.iterdir()):
        if p.suffix in ('.wav', '.mp3', '.json', '.md'):
            z.write(p, p.name)
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
print('Verified six distinct, aligned, unclipped renders from the released Windows DLL.')
print(archive, archive.stat().st_size, 'bytes')
