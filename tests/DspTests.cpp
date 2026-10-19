#include "Dsp.h"
#include <iostream>
#include <random>
#include <vector>
#include <stdexcept>
using namespace atomiccluster;
static void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
static std::vector<double> render(Values p, int n, double sr, bool noise = false) {
    Engine engine; engine.reset(sr, p);
    std::mt19937 rng(42); std::uniform_real_distribution<double> random(-.2, .2);
    std::vector<double> out(n);
    for (int i = 0; i < n; ++i) {
        double x = noise ? random(rng) : .15 * std::sin(2 * pi * 440 * i / sr) + .1 * std::sin(2 * pi * 1320 * i / sr);
        out[i] = engine.process(x, x, p)[0];
        check(std::isfinite(out[i]) && std::abs(out[i]) < 8, "non-finite or unstable audio");
    }
    return out;
}
static double tone(const std::vector<double>& x, double f, double sr) {
    std::complex<double> sum{};
    for (size_t i = x.size() / 2; i < x.size(); ++i) sum += x[i] * std::polar(1., -2 * pi * f * i / sr);
    return std::abs(sum) * 4 / x.size();
}
int main() try {
    int passed = 0;
    auto p = defaults();
    // An actual FFT roundtrip catches corruption of complex phase and normalization.
    std::array<std::complex<double>, fftSize> a{}, original{};
    for (int i = 0; i < fftSize; ++i) a[i] = original[i] = {std::sin(i * .1), std::cos(i * .23)};
    fft(a); fft(a, true);
    for (int i = 0; i < fftSize; ++i) check(std::abs(a[i] - original[i]) < 1e-10, "FFT roundtrip");
    ++passed;
    // A second atom must select the second independent tone, not another
    // leakage bin of the stronger 440 Hz partial (regression from 0.1.0).
    for (int mode : {0, 1}) {
        Values two = defaults(); two[Atoms] = std::log(2.)/std::log(1023.);
        two[Speed] = two[Blend] = 1; two[Smooth] = mode;
        Engine e; e.reset(48000, two); std::vector<double> y(96000);
        for (int i=0; i<int(y.size()); ++i) {
            double in=.2*std::sin(2*pi*440*i/48000)+.08*std::sin(2*pi*880*i/48000);
            y[i]=e.process(in,in,two)[0];
        }
        check(tone(y,440,48000)>.17 && tone(y,880,48000)>.065, "two atoms must retain two distinct partials");
        ++passed;
    }
    {
        Values flat = defaults(); flat[Atoms] = std::log(2.)/std::log(1023.);
        flat[Speed] = flat[Blend] = flat[Flat] = 1;
        Engine e; e.reset(48000,flat); std::vector<double> y(96000);
        for (int i=0;i<int(y.size());++i) {
            double in=.2*std::sin(2*pi*440*i/48000)+.08*std::sin(2*pi*880*i/48000);
            y[i]=e.process(in,in,flat)[0];
        }
        const double ratio=tone(y,880,48000)/tone(y,440,48000);
        check(ratio>.85 && ratio<1.18, "flat mode should equalize distinct partials"); ++passed;
    }
    // Dry mix and bypass must preserve the delayed input exactly at unity gain.
    for (int bypass : {0, 1}) {
        p = defaults(); p[Blend] = 0; p[Bypass] = bypass;
        Engine e; e.reset(48000, p);
        for (int i = 0; i < 12000; ++i) {
            double in = .25 * std::sin(.017 * i);
            auto y = e.process(in, -in, p);
            double expected = i >= fftSize ? .25 * std::sin(.017 * (i - fftSize)) : 0;
            check(std::abs(y[0] - expected) < 1e-12 && std::abs(y[1] + expected) < 1e-12, "dry latency/stereo/bypass");
        }
        ++passed;
    }
    p = defaults(); p[Atoms] = p[Speed] = p[Blend] = 1;
    auto full = render(p, 96000, 48000);
    check(std::abs(tone(full, 440, 48000) - .15) < .003, "full spectrum fundamental reconstruction");
    check(std::abs(tone(full, 1320, 48000) - .10) < .003, "full spectrum harmonic reconstruction");
    ++passed;
    p[Atoms] = 0;
    auto sparse = render(p, 96000, 48000);
    check(tone(sparse, 440, 48000) > .025, "one atom should retain dominant input tone");
    check(tone(sparse, 1320, 48000) < tone(full, 1320, 48000) * .15, "one atom should reject weaker tone");
    ++passed;
    for (double sr : {44100., 48000., 96000., 192000.}) {
        for (int smooth : {0, 1}) {
            p = defaults(); p[Smooth] = smooth; p[Flat] = 1; p[Speed] = 0;
            render(p, int(sr * .4), sr, true); ++passed;
        }
    }
    // No self-oscillation from silence, even with flattened dynamics.
    Engine silence; silence.reset(48000); p = defaults(); p[Flat] = 1;
    for (int i = 0; i < 12000; ++i) check(silence.process(0, 0, p)[0] == 0, "silence must stay silent");
    ++passed;
    // Hostile inputs must not poison persistent DSP state.
    silence.process(NAN, INFINITY, p);
    for (int i = 0; i < 5000; ++i) {
        auto v = silence.process(.1, -.1, p);
        check(std::isfinite(v[0]) && std::isfinite(v[1]), "recovery from invalid input");
    }
    ++passed;
    std::cout << passed << " DSP checks passed\n";
    return 0;
} catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
