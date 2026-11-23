#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>

namespace insideout
{

enum class Mode { MixBus = 0, Guitar, Piano, Synth, Universal };
enum class StereoMode { Stereo = 0, Mid, Side };

constexpr int kMaxBands = 64;
constexpr float kMinFreq = 20.0f;
constexpr float kMaxFreq = 20000.0f;

struct DuckerParams
{
    float amount      = 0.5f;     // 0..1
    float attackMs    = 10.0f;
    float releaseMs   = 250.0f;
    float scLowCut    = 120.0f;   // Hz, 20 = off
    float scHighCut   = 12000.0f; // Hz, 20000 = off
    float lowProtect  = 150.0f;   // Hz, 20 = off
    float highProtect = 10000.0f; // Hz, 20000 = off
    float wet         = 1.0f;     // 0..1
    Mode mode         = Mode::MixBus;
    StereoMode stereo = StereoMode::Mid;
    int numBands      = 32;
    bool listen       = false;
    bool bypass       = false;
};

/** How strongly each mode may cut at a given frequency, and how deep it may cut. */
struct ModeShape
{
    static float weight (Mode mode, float hz) noexcept;
    static float maxDepthDb (Mode mode) noexcept;
};

/** Lock-free data the editor reads for the spectrum display. */
struct SpectrumMeter
{
    SpectrumMeter();

    std::array<std::atomic<float>, kMaxBands> mainDb;
    std::array<std::atomic<float>, kMaxBands> scDb;
    std::array<std::atomic<float>, kMaxBands> redDb;
    std::array<std::atomic<float>, kMaxBands> centreHz;
    std::atomic<int>   numBands { 32 };
    std::atomic<float> maxRedDb { 0.0f };
    std::atomic<float> depthDb { 6.0f };
};

/**
    STFT-based sidechain ducker (Trackspacer style).

    The main signal is processed in the mid/side domain, so "Stereo" (both),
    "Mid" and "Side" modes all use the same analysis and switch without clicks.
    Latency is exactly fftSize samples.
*/
class SpectralDucker
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder; // 2048
    static constexpr int hopSize  = fftSize / 4;   // 75 % overlap
    static constexpr int numBins  = fftSize / 2 + 1;

    SpectralDucker();

    void prepare (double sampleRate);
    void reset();

    /** Processes main channels in place. sidechain may be null / 0 channels. */
    void process (float* const* main, int numMainChannels,
                  const float* const* sidechain, int numScChannels,
                  int numSamples, const DuckerParams& params);

    static constexpr int getLatencySamples() noexcept { return fftSize; }

    SpectrumMeter& getMeter() noexcept { return meter; }

private:
    void configureBands (int numBands);
    void processFrame (const DuckerParams& p, bool mono);
    float bandLevelDb (const float* magnitudes, int band) const noexcept;

    juce::dsp::FFT fft { fftOrder };
    double sampleRate = 48000.0;

    std::vector<float> window;
    float magNorm = 1.0f;   // magnitude of a full-scale sine after windowing
    float olaScale = 0.5f;

    std::array<std::vector<float>, 2> inBuf, outAccum, outReady;
    std::vector<float> scBuf;
    std::vector<float> work, mags, binMask;
    int pos = 0;

    int configuredBands = -1;
    std::vector<int> bandLo, bandHi;
    std::vector<float> bandCentre;
    std::vector<int> binBand;     // lower band index for interpolation
    std::vector<float> binFrac;   // 0..1 between binBand and binBand+1

    std::array<float, kMaxBands> redState {};
    std::array<float, kMaxBands> target {};
    std::array<float, kMaxBands> mainLevel {};
    std::array<float, kMaxBands> scLevel {};

    SpectrumMeter meter;
};

} // namespace insideout
