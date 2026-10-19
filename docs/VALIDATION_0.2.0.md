# Validation — 0.2.0

Independent harmonic resynthesis update, implementing the user's listening notes:
100% wet end stop, audible density, harmonic patterns, separate moving stereo
voices, rhythmic SHARP and continuous SMOOTH. This changes the sound engine;
it is not a calibrated replica of the physical EHX pedal.

## Verified

- SDK 3.8.1 pinned to `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`.
- Linux GNU C++ 14.2 / CMake 3.31.6: native validator **47/47**;
  CTest **2/2 suites** (`dsp_regression`, `processor_state`).
- Windows x64 MinGW cross-build, actual binaries executed under Wine 10 / Xvfb:
  - **25 DSP checks**: FFT accuracy, exact delayed stereo dry/bypass,
    100% wet exclusion of an unselected 17 kHz input tone after automation,
    harmonic density and level, moving stereo from mono input, mono endpoint,
    deterministic random reset, distinct ARP/RANDOM patterns, SHARP pulse vs
    SMOOTH sustain, noise/flat/modes at 8/44.1/96/384 kHz, silence, invalid input,
    prompt capture of a note played after activation, held-pitch retention,
    flat-dynamics equalization, stereo position varying over time, anti-phase input.
  - **11 processor/state checks**, including 12 stable parameter IDs, format 2
    roundtrip, invalid/truncated atomic rejection, format 1 migration with ten
    values and original atom-count conversion, text roundtrip for new controls,
    mono/stereo layouts, bypass, automation, concurrent restore and empty flush.
  - Official validator on packaged Windows bundle: **47 passed, 0 failed**.
  - **10 GUI checks** on the actual DLL: attach, mode, knob, dynamics hold,
    bypass, tap, visible CLUSTER/ARP/RANDOM buttons and stereo control; detach
    and unload. Captured editor inspected.
- Fixed storage in DSP; no allocations or locks in `Engine::process`.

## Controlled density probe

Input: steady 220 Hz sine, amplitude 0.15, 48 kHz; full wet, SMOOTH,
fast capture, mono. Harmonics counted above 0.003 sinusoidal amplitude.

| ATOMS | Measured harmonic components | Output RMS |
|---:|---:|---:|
| 1 | 1 | 0.11172 |
| 16 | 16 | 0.10621 |
| 64 | 64 | 0.09251 |

The density now increases even for a single-tone source, without the old
full-spectrum endpoint. These are signal measurements, not a listening score
or proof that all 64 voices remain perceptually distinct in every guitar mix.

## Release previews

`tools/render-examples-020.sh` extracts the exact DLL from the release ZIP and
renders the same original synthetic plucked source for eight files (dry + seven
settings). It checks PCM format/duration, dry alignment, finite unclipped levels
and nonzero side energy for stereo examples. Per-file SHA256, source/DLL identity,
RMS and stereo-side RMS are in `audio-examples/0.2.0/manifest.json`.
The comparison concatenates those files without level normalization. No uploaded
reference audio/video is redistributed. No external reverb/delay is added.

## Compatibility and limits

- State IDs 0–9 preserved; pattern and stereo appended as IDs 10 and 11.
- Format 1 projects load with their former ATOMS count (capped at 64), CLUSTER,
  and stereo motion disabled. New projects use 16 atoms, 100% wet, 85% stereo.
- Range 1–64 replaces 1–1023. The sound engine has deliberately changed.
  Existing host automation uses the new normalized ATOMS mapping; old automation
  curves can therefore require retuning, even though saved static state is migrated.
- Reported latency remains 2048 samples; tail allowance 5 seconds plus latency.
- Real Windows / FL Studio execution and GitHub Actions result have not been
  observed. Windows checks here use Wine. Physical pedal equivalence is unverified.

## Reproduce

`cmake --build build --parallel 4` then `ctest --test-dir build --output-on-failure`.
`cmake --build build-win --parallel 4` then `bash tools/test-windows.sh`.
`python3 tools/package.py --build build-win` creates the distributable bundle ZIP.
`bash tools/render-examples-020.sh` renders and verifies the listening previews.
