"""Deterministic, original synthetic plucked-string phrase; no external recordings."""
import math
import random
import struct
import wave
from pathlib import Path

rate = 48000
audio = [0.0] * (rate * 9)
rng = random.Random(71)
notes = [(i * .375, n, .8) for i, n in enumerate([52, 55, 57, 59, 62, 59, 55, 52])]
notes += [(3.2 + i * .035, n, .48) for i, n in enumerate([40, 47, 52, 55, 59, 64])]
notes += [(6 + i * .25, n, .7) for i, n in enumerate([64, 67, 71, 69, 67, 64, 62, 59])]
for onset, midi, velocity in notes:
    frequency = 440 * 2 ** ((midi - 69) / 12)
    start = round(onset * rate)
    length = min(round(2.5 * rate), len(audio) - start)
    for harmonic in range(1, 17):
        amplitude = velocity / harmonic ** 1.1
        angular = 2 * math.pi * frequency * harmonic * math.sqrt(1 + .00004 * harmonic ** 2) / rate
        decay = (1.9 + harmonic * .22) / rate
        for i in range(length):
            audio[start + i] += amplitude * math.sin(angular * i) * math.exp(-decay * i) * min(1, i / 60)
    for i in range(min(length, 720)):
        audio[start + i] += .07 * velocity * rng.uniform(-1, 1) * math.exp(-i / 110)
scale = .45 / max(abs(x) for x in audio)
for i in range(len(audio) - 7200, len(audio)):
    audio[i] *= (len(audio) - 1 - i) / 7200
target = Path('audio-examples/source_plucked.wav')
target.parent.mkdir(parents=True, exist_ok=True)
with wave.open(str(target), 'wb') as f:
    f.setnchannels(1)
    f.setsampwidth(2)
    f.setframerate(rate)
    f.writeframes(b''.join(struct.pack('<h', round(x * scale * 32767)) for x in audio))
print(target)
