# LV2 Raspberry Pi release validation — 0.2.1

## Built and executed

- Native Linux x86-64: GCC 14.2; two CTest suites, 29 DSP checks and 10 real
  LV2 host checks. Native bundle discovered and verified with Lilv.
- ARMHF: GCC 10.2.1 / glibc 2.31 cross sysroot, Cortex-A72 hard-float/NEON.
  DSP suite and real LV2 `.so` host suite executed under qemu-arm (Cortex-A15
  compatible ARMv7 execution). Two suites passed, no skipped/disabled checks.
- AArch64: GCC 10.2.1 / glibc 2.31 cross sysroot, Cortex-A72.
  Same suites executed under qemu-aarch64 / Cortex-A72. Two suites passed.
- After selecting old glibc math ABI symbols, both actual ARM shared modules
  again passed the 10-check LV2 host suite. The common DSP source is unchanged.
- Lilv discovers the URI, 19 ports, the latency designation and three presets.
  Metadata validation checks the full port order, MOD GUI audio jacks, every
  visible control and all referenced resources. MODEP browser graphics are
  original illustrations, not captures of a physical MODEP session.

The LV2 host tests use `dlopen` on the built plugin and real URID/Atom APIs:
required features, exact delayed dry/bypass, moving stereo from an optional
mono right input, no dry-tone leak at full wet, PPQ pulse grid with latency,
sample-offset time events, 6/8 beat-unit conversion, malformed event/control recovery and in-place audio.

## Binary compatibility

Each package is checked for ELF class/machine, ARMHF hard-float ABI, exported
`lv2_descriptor`, allowed dynamic dependencies and maximum GLIBC version.
AArch64 imports GLIBC_2.17; ARMHF imports GLIBC_2.4. libstdc++/libgcc runtime
is linked statically with GCC runtime notices and exception text included.
No VST3 SDK or GUI toolkit is linked into LV2.

The longstanding glibc compatibility symbols for exp/log/pow avoid a new
GLIBC_2.29 requirement in older MODEP images. The target's own libc and libm
remain in use; no system library is bundled or replaced.

## Installer

The combined ZIP contains both ABIs. The installer checks userland bitness,
not just `uname -m`, supports explicit prefixes, and preserves prior bundles
outside the LV2 scan path. MODEP defaults to `/usr/modep/lv2`; Zynthian to
`/usr/local/lib/lv2`, present in upstream Zynthian's LV2_PATH. Install, update,
dry-run and binary identity are tested in a temporary directory.

## Practical limits

No physical RPi 4B, audio interface, full MODEP UI or Zynthian engine session
has been exercised here. QEMU validates code execution and plugin behavior,
not physical CPU usage, audio-interface routing, xruns or end-to-end latency.
Start with 16 atoms, 48 kHz, 256 frames and check actual load on the device.
Reported algorithm latency is 2048 samples (42.7 ms at 48 kHz), before host and
interface buffering. Host control-port state is stored by the host; internal
oscillator phases are reset on activation, not serialized as custom LV2 state.

## Repeatable workflow

`bash tools/setup-lv2-cloud.sh` installs signature-verified, pinned Debian
cross tools into `/workspace/toolchains/lv2`, builds native and both ARM
bundles, runs actual suites under QEMU, scans the metadata and packages ZIPs.
No system root installation or long-running service is required in the cloud.
`tools/generate-lv2-metadata.py` regenerates metadata/HTML/CSS.
`tools/generate-lv2-artwork.py` regenerates original PNG assets (Pillow needed
only for that maintainer step; committed artwork is used by normal builds).
GitHub native LV2 workflow is supplied; its remote outcome is not inferred.
