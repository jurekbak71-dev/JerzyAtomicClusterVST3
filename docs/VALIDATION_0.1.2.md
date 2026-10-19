# Validation — 0.1.2

## Verified in the cloud instance

- Linux x86_64: GNU C++ 14.2, CMake 3.31.6, Ninja 1.11.1.4.
- Windows x64: MinGW GCC 14.2 cross-build, static GCC/C++/pthread runtime.
- Steinberg VST3 SDK 3.8.1, commit `3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96`.
- Linux native VST3 validator: **47 passed, 0 failed**.
- Linux CTest: **2 suites passed, 0 failed** (`dsp_regression`, `processor_state`).
- Windows binaries executed under Wine 10.0 with an Xvfb display:
  - **19 DSP checks passed**: FFT reconstruction, aligned dry/bypass,
    stereo polarity, fundamental/harmonic retention at full resolution,
    weaker-tone rejection with one atom, finite output across several sample rates,
    both transition modes, silence and recovery from invalid input; two independent
    partials retained with ATOMS=2 in Sharp and Smooth, and Flat equalization
    of those partials; captured-pitch retention during release.
  - **9 processor/state checks passed**: parameter IDs, all-control state restore,
    invalid/truncated state rejection without partial modification, mono/stereo,
    delayed bypass, automation persistence, concurrent restore while processing,
    zero-sample flush.
  - Official Windows VST3 validator on the **packaged bundle**:
    **47 passed, 0 failed**. Bundle structure validated.
  - **6 GUI checks passed**, loading the actual Windows DLL: attachment,
    mode click, knob drag, long-press dynamics, bypass, tap tempo; detach/unload
    also completed successfully. Screenshot inspected.
- Windows DLL imports only system libraries (GDI32, KERNEL32, MSVCRT, OLE32, USER32),
  without external GCC runtime DLL requirements.

## 0.1.2 sound changes

SMOOTH crossfade time now scales with capture interval (3–450 ms rather than
an 80 ms maximum). Newly assigned spectral lobes approach their phase lock
progressively. Disappearing partials retain captured phase increments during
release instead of using estimates from the new, unrelated or silent input.
A 440 Hz held-note/release regression passes in this release and fails when
compiled against the previous 0.1.1 DSP. These are controlled DSP checks,
not a perceptual similarity score against the uploaded demonstration.

Five new examples render the packaged Windows DLL on the same original
synthetic plucked phrase, without additional reverb, delay or normalization.
The reference film includes other equipment; no isolated paired dry/wet
recording is available. The uploaded reference media is not redistributed.

## Diagnosed and corrected before packaging

0.1.1 fixes selection of adjacent FFT bins as multiple atoms. A controlled
440/880 Hz probe in the actual Linux VST3 changes the 880/440 output ratio
from -120.32 dB in 0.1.0 to -7.9615 dB in 0.1.1, versus -7.9588 dB at input.
This validates this regression, not equivalence to the EHX hardware.
The uploaded reference recording was analyzed numerically; knob settings
and an isolated paired dry input were not available. See
`docs/reference-analysis/REPORT.md` for evidence and limits.

The first flat-file Windows build loaded and passed its processing tests, but
the current SDK validator rejected its bundle structure. Distribution now uses
`JerzyAtomicCluster.vst3/Contents/x86_64-win/JerzyAtomicCluster.vst3` and generated
`Contents/Resources/moduleinfo.json`. The packaged bundle passed validation.
Root-unit metadata and parsing of human-readable parameter values were corrected.

The validator probes unusual sample rates, including 1234.568 and 1234567.8 Hz.
The plugin explicitly rejects rates outside 8–384 kHz; those probes report
setup rejection inside a test that the validator marks successful.

## Not verified

- Native execution in Windows or FL Studio on a real Windows machine.
- Audible equivalence to a physical EHX Pico Atomic Cluster. The manual provides
  behavior descriptions, not source code, measured transfer functions or exact
  numeric control ranges. This release is an independent experimental implementation.
- Real-time monitoring at low latency: reported latency is 2048 samples.
- GitHub Actions result: the workflow is supplied, but its remote result has not
  been read or inferred from the local tests.

## Reproduce

`cmake --build build --parallel 4` runs Linux validation automatically.
`ctest --test-dir build --output-on-failure` runs both regression suites.
`cmake --build build-win --parallel 4` builds Windows binaries.
`bash tools/test-windows.sh` generates bundle metadata, runs Windows checks,
and captures the GUI. Its logs are in `build-win/validation`.
`python3 tools/package.py --build build-win` regenerates the distributable ZIP
and SHA256 file, including this report and SDK license notices.
