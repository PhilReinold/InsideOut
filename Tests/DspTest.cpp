// Offline tests for the InsideOut DSP. Build with -DINSIDEOUT_BUILD_TESTS=ON.
#include "SpectralDucker.h"
#include <cstdio>
#include <cmath>
#include <random>

using namespace insideout;

static constexpr double sr = 48000.0;
static int failures = 0;

static void check (bool ok, const char* what, double value)
{
    std::printf ("[%s] %s (%.4f)\n", ok ? " OK " : "FAIL", what, value);
    if (! ok) ++failures;
}

static double goertzelDb (const std::vector<float>& x, size_t start, size_t len, double hz)
{
    const double w = 2.0 * juce::MathConstants<double>::pi * hz / sr;
    double re = 0, im = 0;
    for (size_t n = 0; n < len; ++n)
    {
        const double win = 0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * (double) n / (double) len);
        re += x[start + n] * win * std::cos (w * (double) n);
        im -= x[start + n] * win * std::sin (w * (double) n);
    }
    return 20.0 * std::log10 (std::sqrt (re * re + im * im) + 1e-12);
}

struct Run
{
    std::vector<float> inL, inR, outL, outR;
};

static Run run (const DuckerParams& p, int total, std::function<float(int)> mainL, std::function<float(int)> mainR,
                std::function<float(int)> sc, bool withSc, int block = 377)
{
    SpectralDucker d;
    d.prepare (sr);
    Run r;
    r.inL.resize ((size_t) total); r.inR.resize ((size_t) total);
    r.outL.resize ((size_t) total); r.outR.resize ((size_t) total);
    std::vector<float> scv ((size_t) total);
    for (int i = 0; i < total; ++i)
    {
        r.inL[(size_t) i] = mainL (i);
        r.inR[(size_t) i] = mainR (i);
        scv[(size_t) i] = sc (i);
    }
    r.outL = r.inL; r.outR = r.inR;

    for (int s = 0; s < total; s += block)
    {
        const int n = std::min (block, total - s);
        float* ch[2] = { r.outL.data() + s, r.outR.data() + s };
        const float* scc[1] = { scv.data() + s };
        d.process (ch, 2, scc, withSc ? 1 : 0, n, p);
    }
    return r;
}

int main()
{
    std::mt19937 rng (1);
    std::uniform_real_distribution<float> uni (-0.5f, 0.5f);
    std::vector<float> noiseL (200000), noiseR (200000);
    for (auto& v : noiseL) v = uni (rng);
    for (auto& v : noiseR) v = uni (rng);

    const int N = SpectralDucker::getLatencySamples();
    const int total = 96000;

    auto nl = [&] (int i) { return noiseL[(size_t) i]; };
    auto nr = [&] (int i) { return noiseR[(size_t) i]; };
    auto zero = [] (int) { return 0.0f; };

    // 1. Null test: no sidechain -> output equals input delayed by latency
    for (int mode = 0; mode < 3; ++mode)
    {
        DuckerParams p;
        p.amount = 1.0f;
        p.stereo = (StereoMode) mode;
        auto r = run (p, total, nl, nr, zero, false);
        double maxErr = 0;
        for (int i = N; i < total; ++i)
        {
            maxErr = std::max (maxErr, (double) std::abs (r.outL[(size_t) i] - r.inL[(size_t) (i - N)]));
            maxErr = std::max (maxErr, (double) std::abs (r.outR[(size_t) i] - r.inR[(size_t) (i - N)]));
        }
        check (maxErr < 1e-4, "Null-Test ohne Sidechain (Stereo/Mid/Side), max Fehler", maxErr);
    }

    // 2. Bypass with a loud sidechain must also be transparent
    {
        DuckerParams p; p.amount = 1.0f; p.bypass = true; p.mode = Mode::Universal;
        auto sine = [] (int i) { return 0.5f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * i / sr); };
        auto r = run (p, total, nl, nr, sine, true);
        double maxErr = 0;
        for (int i = N; i < total; ++i)
            maxErr = std::max (maxErr, (double) std::abs (r.outL[(size_t) i] - r.inL[(size_t) (i - N)]));
        check (maxErr < 1e-4, "Bypass transparent", maxErr);
    }

    // 3. Universal: vocal-like sine at 1 kHz ducks 1 kHz but leaves 100 Hz alone
    auto twoTones = [] (int i) {
        const double t = i / sr;
        return (float) (0.3 * std::sin (2 * juce::MathConstants<double>::pi * 100.0 * t)
                      + 0.3 * std::sin (2 * juce::MathConstants<double>::pi * 1000.0 * t));
    };
    auto sc1k = [] (int i) { return 0.25f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * i / sr); };
    {
        DuckerParams p; p.mode = Mode::Universal; p.amount = 1.0f; p.lowProtect = 20.0f; p.highProtect = 20000.0f;
        p.scLowCut = 20.0f; p.scHighCut = 20000.0f; p.stereo = StereoMode::Stereo;
        auto r = run (p, total, twoTones, twoTones, sc1k, true);
        const size_t st = (size_t) total - 16384;
        const double d1k = goertzelDb (r.outL, st, 16384, 1000.0) - goertzelDb (r.inL, st - (size_t) N, 16384, 1000.0);
        const double d100 = goertzelDb (r.outL, st, 16384, 100.0) - goertzelDb (r.inL, st - (size_t) N, 16384, 100.0);
        check (d1k < -15.0, "Universal: 1 kHz wird deutlich abgesenkt (dB)", d1k);
        check (std::abs (d100) < 0.5, "Universal: 100 Hz bleibt (dB)", d100);
    }

    // 4. Mix Bus: gentle, max 6 dB, and Low Protect keeps the bass untouched
    auto scBassVoc = [] (int i) {
        const double t = i / sr;
        return (float) (0.25 * std::sin (2 * juce::MathConstants<double>::pi * 100.0 * t)
                      + 0.25 * std::sin (2 * juce::MathConstants<double>::pi * 2000.0 * t));
    };
    auto mainBassVoc = [] (int i) {
        const double t = i / sr;
        return (float) (0.3 * std::sin (2 * juce::MathConstants<double>::pi * 100.0 * t)
                      + 0.3 * std::sin (2 * juce::MathConstants<double>::pi * 2000.0 * t));
    };
    {
        DuckerParams p; p.mode = Mode::MixBus; p.amount = 1.0f; p.lowProtect = 150.0f; p.stereo = StereoMode::Mid;
        auto r = run (p, total, mainBassVoc, mainBassVoc, scBassVoc, true);
        const size_t st = (size_t) total - 16384;
        const double d2k = goertzelDb (r.outL, st, 16384, 2000.0) - goertzelDb (r.inL, st - (size_t) N, 16384, 2000.0);
        const double d100 = goertzelDb (r.outL, st, 16384, 100.0) - goertzelDb (r.inL, st - (size_t) N, 16384, 100.0);
        check (d2k < -4.0 && d2k > -7.0, "Mix Bus: 2 kHz sanft abgesenkt, max ca. 6 dB", d2k);
        check (std::abs (d100) < 0.3, "Mix Bus: Low Protect schuetzt 100 Hz", d100);
    }

    // 5. Mid mode: side content stays untouched
    {
        DuckerParams p; p.mode = Mode::Universal; p.amount = 1.0f; p.stereo = StereoMode::Mid; p.lowProtect = 20.0f;
        auto l = [] (int i) { return 0.3f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * i / sr); };
        auto rr = [] (int i) { return -0.3f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 1000.0 * i / sr); };
        auto r = run (p, total, l, rr, sc1k, true); // pure side signal
        const size_t st = (size_t) total - 16384;
        const double d = goertzelDb (r.outL, st, 16384, 1000.0) - goertzelDb (r.inL, st - (size_t) N, 16384, 1000.0);
        check (std::abs (d) < 0.3, "Mid-Modus: reines Side-Signal unveraendert (dB)", d);
    }

    // 6. Dry/Wet 0 -> transparent, Listen -> only removed part
    {
        DuckerParams p; p.mode = Mode::Universal; p.amount = 1.0f; p.wet = 0.0f; p.stereo = StereoMode::Stereo; p.lowProtect = 20.0f;
        auto r = run (p, total, twoTones, twoTones, sc1k, true);
        double maxErr = 0;
        for (int i = N; i < total; ++i)
            maxErr = std::max (maxErr, (double) std::abs (r.outL[(size_t) i] - r.inL[(size_t) (i - N)]));
        check (maxErr < 1e-4, "Dry/Wet 0 % transparent", maxErr);

        p.wet = 1.0f; p.listen = true;
        auto r2 = run (p, total, twoTones, twoTones, sc1k, true);
        const size_t st = (size_t) total - 16384;
        const double d100 = goertzelDb (r2.outL, st, 16384, 100.0) - goertzelDb (r2.inL, st - (size_t) N, 16384, 100.0);
        const double d1k = goertzelDb (r2.outL, st, 16384, 1000.0) - goertzelDb (r2.inL, st - (size_t) N, 16384, 1000.0);
        check (d100 < -40.0, "Listen: 100 Hz (nicht abgesenkt) fast stumm (dB)", d100);
        check (d1k > -3.0, "Listen: 1 kHz (abgesenkt) hoerbar (dB)", d1k);
    }

    // 7. Mono and different band counts don't blow up
    for (int bands : { 16, 32, 64 })
    {
        SpectralDucker d; d.prepare (44100.0);
        DuckerParams p; p.numBands = bands; p.mode = Mode::Synth; p.amount = 1.0f;
        std::vector<float> m (44100), s (44100);
        for (size_t i = 0; i < m.size(); ++i) { m[i] = noiseL[i]; s[i] = noiseR[i]; }
        float* ch[1] = { m.data() };
        const float* sc[1] = { s.data() };
        d.process (ch, 1, sc, 1, (int) m.size(), p);
        bool finite = true;
        for (auto v : m) finite = finite && std::isfinite (v);
        check (finite, "Mono, Baender ok", (double) bands);
    }

    std::printf ("%s\n", failures == 0 ? "ALLE TESTS OK" : "TESTS FEHLGESCHLAGEN");
    return failures == 0 ? 0 : 1;
}
