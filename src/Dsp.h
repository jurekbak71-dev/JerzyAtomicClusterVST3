#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdint>

namespace atomiccluster {
constexpr double pi = 3.14159265358979323846;
constexpr int fftSize = 2048, hopSize = 256, bins = fftSize / 2 + 1;
enum Parameter { Volume, Blend, Speed, Atoms, Smooth, Flat, Bypass, Sync, Division, TapEnabled, NumParams };
using Values = std::array<double, NumParams>;
inline Values defaults() { return {0.5, 0.8, 0.65, std::log(23.) / std::log(1023.), 0, 0, 0, 0, 0.5, 1}; }
inline double clean(double v) { return std::isfinite(v) ? std::clamp(v, 0., 1.) : 0.; }
inline double wrap(double x) { return std::remainder(x, 2 * pi); }
inline int atomCount(double v) { return std::clamp(int(std::round(std::pow(1023., clean(v)))), 1, 1023); }
inline double interval(double speed) { return 2. * std::pow(1. / 750., clean(speed)); }
inline void fft(std::array<std::complex<double>, fftSize>& a, bool inverse = false) {
    for (int i = 1, j = 0; i < fftSize; ++i) {
        int b = fftSize >> 1;
        for (; j & b; b >>= 1) j ^= b;
        j ^= b;
        if (i < j) std::swap(a[i], a[j]);
    }
    for (int len = 2; len <= fftSize; len <<= 1) {
        const auto step = std::polar(1., (inverse ? 2 : -2) * pi / len);
        for (int start = 0; start < fftSize; start += len) {
            std::complex<double> w(1., 0.);
            for (int j = 0; j < len / 2; ++j) {
                const auto u = a[start + j], v = a[start + j + len / 2] * w;
                a[start + j] = u + v;
                a[start + j + len / 2] = u - v;
                w *= step;
            }
        }
    }
    if (inverse) for (auto& v : a) v /= fftSize;
}

// Streaming phase-vocoder: sparse, held spectral frames with sample-continuous OLA.
// Fixed storage throughout: no allocation or locking in process().
class SpectralChannel {
    std::array<double, fftSize> input{}, window{};
    std::array<double, fftSize * 4> output{};
    std::array<double, bins> previousPhase{}, phase{}, delta{}, target{}, magnitude{}, heldDelta{};
    std::array<std::complex<double>, fftSize> spectrum{};
    std::array<int, bins - 2> rank{};
    uint64_t cursor = 0;
    double sampleRate = 48000, untilRefresh = 0;
    bool first = true;
    void frame(const Values& p, double bpm) {
        for (int i = 0; i < fftSize; ++i)
            spectrum[i] = input[(cursor + i) % fftSize] * window[i];
        fft(spectrum);
        for (int k = 0; k < bins; ++k) {
            const double angle = std::arg(spectrum[k]);
            const double expected = 2 * pi * k * hopSize / fftSize;
            delta[k] = first ? expected : expected + wrap(angle - previousPhase[k] - expected);
            previousPhase[k] = angle;
        }
        const bool refresh = first || untilRefresh <= 0;
        double seconds = interval(p[Speed]);
        if (p[Sync] >= .5) {
            constexpr double beats[]{.25, .5, 1, 2, 4};
            seconds = 60. / std::clamp(bpm, 20., 400.) * beats[int(std::round(clean(p[Division]) * 4))];
        }
        if (refresh) {
            target.fill(0);
            for (int i = 0; i < bins - 2; ++i) rank[i] = i + 1;
            const int count = atomCount(p[Atoms]);
            std::partial_sort(rank.begin(), rank.begin() + count, rank.end(), [this](int a, int b) {
                return std::norm(spectrum[a]) > std::norm(spectrum[b]);
            });
            double energy = 0;
            for (int i = 0; i < count; ++i) energy += std::norm(spectrum[rank[i]]);
            const double equal = std::sqrt(energy / count);
            for (int i = 0; i < count; ++i) {
                int k = rank[i];
                target[k] = p[Flat] >= .5 ? equal : std::abs(spectrum[k]);
            }
            heldDelta = delta;
            untilRefresh += std::max(double(hopSize), seconds * sampleRate);
        }
        const double fadeTime = std::min(.08, seconds * .5);
        const double alpha = p[Smooth] >= .5 ? 1 - std::exp(-hopSize / (sampleRate * fadeTime)) : 1.;
        for (int k = 0; k < bins; ++k) {
            magnitude[k] += alpha * (target[k] - magnitude[k]);
            if (magnitude[k] < 1e-15) magnitude[k] = 0;
            if (first || (refresh && p[Smooth] < .5)) phase[k] = previousPhase[k];
            else phase[k] = wrap(phase[k] + heldDelta[k]);
            spectrum[k] = std::polar(magnitude[k], phase[k]);
        }
        spectrum[0] = spectrum[fftSize / 2] = 0.;
        for (int k = 1; k < fftSize / 2; ++k) spectrum[fftSize - k] = std::conj(spectrum[k]);
        fft(spectrum, true);
        for (int i = 0; i < fftSize; ++i)
            output[(cursor + i) % output.size()] += spectrum[i].real() * window[i] / 3.;
        untilRefresh -= hopSize;
        first = false;
    }
public:
    SpectralChannel() { for (int i = 0; i < fftSize; ++i) window[i] = .5 - .5 * std::cos(2 * pi * i / fftSize); }
    void reset(double sr) {
        input.fill(0); output.fill(0); previousPhase.fill(0); phase.fill(0);
        target.fill(0); magnitude.fill(0); heldDelta.fill(0); delta.fill(0);
        cursor = 0; untilRefresh = 0; first = true; sampleRate = sr;
    }
    // Returns aligned dry/wet samples, both delayed by fftSize samples.
    std::array<double, 2> process(double x, const Values& p, double bpm) {
        if (!std::isfinite(x)) x = 0;
        x = std::clamp(x, -16., 16.);
        const auto pos = cursor % fftSize;
        const double dry = input[pos];
        input[pos] = x;
        const double wet = output[cursor % output.size()];
        output[cursor % output.size()] = 0;
        ++cursor;
        if (cursor >= fftSize && cursor % hopSize == 0) frame(p, bpm);
        return {dry, std::isfinite(wet) ? wet : 0};
    }
};

class Engine {
    std::array<SpectralChannel, 2> channels;
    double mix = .8, gain = 1., engaged = 1., smoothing = .004;
public:
    void reset(double sr, const Values& p = defaults()) {
        for (auto& c : channels) c.reset(sr);
        mix = p[Blend]; gain = p[Volume] * 2.; engaged = p[Bypass] >= .5 ? 0 : 1;
        smoothing = 1 - std::exp(-1. / (sr * .005));
    }
    std::array<double, 2> process(double l, double r, const Values& p, double bpm = 120) {
        mix += smoothing * (p[Blend] - mix);
        gain += smoothing * (p[Volume] * 2. - gain);
        engaged += smoothing * ((p[Bypass] >= .5 ? 0 : 1) - engaged);
        const std::array<double, 2> in{l, r};
        std::array<double, 2> result{};
        for (int c = 0; c < 2; ++c) {
            const auto v = channels[c].process(in[c], p, bpm);
            const double effected = gain * ((1 - mix) * v[0] + mix * v[1]);
            result[c] = v[0] + engaged * (effected - v[0]);
        }
        return result;
    }
};
}
