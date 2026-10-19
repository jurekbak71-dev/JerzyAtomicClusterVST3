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
    std::array<double, bins> relativePhase{};
    std::array<int, bins> owner{};
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
            owner.fill(-1);
            const int count = atomCount(p[Atoms]);
            int candidates = 0;
            double strongest = 0;
            for (int k = 1; k < bins - 1; ++k) strongest = std::max(strongest, std::norm(spectrum[k]));
            // Count independent local maxima, not neighboring bins of one
            // Hann-windowed partial. Ignore peaks >60 dB below the strongest.
            for (int k = 1; k < bins - 1; ++k) {
                const double m = std::norm(spectrum[k]);
                if (m > 1e-24 && m >= strongest * 1e-6 && m > std::norm(spectrum[k-1]) && m >= std::norm(spectrum[k+1]))
                    rank[candidates++] = k;
            }
            const int selected = std::min(count, candidates);
            std::partial_sort(rank.begin(), rank.begin() + selected, rank.begin() + candidates, [this](int a, int b) {
                return std::norm(spectrum[a]) > std::norm(spectrum[b]);
            });
            double energy = 0;
            for (int i = 0; i < selected; ++i) {
                energy += std::norm(spectrum[rank[i]]);
                owner[rank[i]] = rank[i];
            }
            const double equal = selected ? std::sqrt(energy / selected) : 0;
            for (int i = 0; i < selected; ++i) {
                const int center = rank[i];
                for (int k = std::max(1, center-2); k <= std::min(bins-2, center+2); ++k)
                    if (owner[k] < 0) owner[k] = center;
                if (first) {
                    const double l = std::log(std::max(1e-20, std::abs(spectrum[center-1])));
                    const double m = std::log(std::max(1e-20, std::abs(spectrum[center])));
                    const double r = std::log(std::max(1e-20, std::abs(spectrum[center+1])));
                    const double fraction = std::clamp(.5*(l-r)/(l-2*m+r), -.5, .5);
                    delta[center] = 2*pi*(center+fraction)*hopSize/fftSize;
                }
            }
            for (int k = 1; k < bins-1; ++k) {
                if (count == bins-2 && p[Flat] < .5) {
                    // Keep the documented full-resolution, full-dynamics end stop.
                    owner[k] = k; relativePhase[k] = 0; target[k] = std::abs(spectrum[k]);
                } else if (owner[k] >= 0) {
                    const int center = owner[k];
                    const double scale = p[Flat] >= .5 ? equal/std::max(1e-20,std::abs(spectrum[center])) : 1.;
                    target[k] = std::abs(spectrum[k])*scale;
                    relativePhase[k] = wrap(previousPhase[k]-previousPhase[center]);
                }
            }
            // A disappearing partial must finish fading at its old pitch.
            // Updating every bin here made the released note follow unrelated
            // input phase estimates, producing chirps during SMOOTH transitions.
            for (int k = 1; k < bins-1; ++k)
                if (target[k] > 0) heldDelta[k] = delta[k];
            untilRefresh += std::max(double(hopSize), seconds * sampleRate);
        }
        // Scale the crossfade with capture time: slow captures should become
        // sustained pads rather than short, almost SHARP amplitude steps.
        const double fadeTime = std::clamp(seconds * .32, .003, .45);
        const double alpha = p[Smooth] >= .5 ? 1 - std::exp(-hopSize / (sampleRate * fadeTime)) : 1.;
        for (int k = 0; k < bins; ++k) {
            magnitude[k] += alpha * (target[k] - magnitude[k]);
            if (magnitude[k] < 1e-15) magnitude[k] = 0;
            if (first || (refresh && p[Smooth] < .5)) phase[k] = previousPhase[k];
            else phase[k] = wrap(phase[k] + heldDelta[k]);
        }
        // Phase-lock each selected partial's window lobes to its center.
        for (int k = 1; k < bins-1; ++k)
            if (owner[k] >= 0 && owner[k] != k) {
                const double locked = wrap(phase[owner[k]]+relativePhase[k]);
                // Reassigning a lobe at a capture must not jump its phase in
                // SMOOTH mode. SHARP retains the immediate phase lock.
                phase[k] = wrap(phase[k] + alpha * wrap(locked-phase[k]));
            }
        for (int k = 0; k < bins; ++k) spectrum[k] = std::polar(magnitude[k], phase[k]);
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
        owner.fill(-1); relativePhase.fill(0);
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
