# Historical validation — 0.1.0

The original release remains in `releases/0.1.0` with its original validation
report inside the ZIP. It passed Linux VST3 validation (47/47), 15 DSP checks,
9 processor/state checks, and Windows-under-Wine VST3 validation (47/47)
and GUI checks (6/6). Its ATOMS control selected individual FFT bins; the
distinct-partial selection regression was discovered afterward and corrected
in 0.1.1. Existing audio examples document 0.1.0 and remain unchanged.
