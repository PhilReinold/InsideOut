#include "SpectralDucker.h"

#include <cmath>

namespace insideout
{

namespace
{
    inline float clamp01 (float x) noexcept { return juce::jlimit (0.0f, 1.0f, x); }
    inline float octaves (float a, float b) noexcept { return std::log2 (a / b); }
    inline float bumpLog (float hz, float centreHz, float widthDecades) noexcept
    {
        const float d = (std::log10 (hz) - std::log10 (centreHz)) / widthDecades;
        return std::exp (-d * d);
    }

    /** 1 = may be cut, 0 = protected. Soft transition over one octave. */
    inline float protectWeight (float hz, float lowProtect, float highProtect) noexcept
    {
        float w = 1.0f;
        if (lowProtect > kMinFreq + 0.5f)
            w *= clamp01 (octaves (hz, lowProtect));
        if (highProtect < kMaxFreq - 1.0f)
            w *= clamp01 (octaves (highProtect, hz));
        return w;
    }

    /** Sidechain filters: 12 dB/oct below low cut and above high cut. */
    inline float sidechainFilterDb (float hz, float lowCut, float highCut) noexcept
    {
        float db = 0.0f;
        if (lowCut > kMinFreq + 0.5f && hz < lowCut)
            db -= 12.0f * octaves (lowCut, hz);
        if (highCut < kMaxFreq - 1.0f && hz > highCut)
            db -= 12.0f * octaves (hz, highCut);
        return db;
    }

    constexpr float kScFloorDb = -66.0f; // sidechain level where ducking starts
    constexpr float kScRangeDb = 48.0f;  // range until full ducking
}

//==============================================================================
float ModeShape::weight (Mode mode, float hz) noexcept
{
    switch (mode)
    {
        case Mode::MixBus:
            // Focus on the vocal presence region (about 1-5 kHz)
            return 0.12f + 0.88f * bumpLog (hz, 2200.0f, 0.38f);

        case Mode::Guitar:
        {
            const float w = clamp01 (octaves (hz, 100.0f)) * clamp01 (octaves (10000.0f, hz));
            return 0.1f + 0.9f * w;
        }

        case Mode::Piano:
        {
            const float w = clamp01 (octaves (hz, 75.0f)) * clamp01 (octaves (12000.0f, hz));
            const float lowMidCare = 1.0f - 0.3f * bumpLog (hz, 300.0f, 0.2f);
            return (0.08f + 0.92f * w) * lowMidCare;
        }

        case Mode::Synth:
        {
            const float w = clamp01 (octaves (hz, 50.0f)) * clamp01 (octaves (16000.0f, hz) / 0.7f);
            return 0.1f + 0.9f * w;
        }

        case Mode::Universal:
        default:
            return 1.0f;
    }
}

float ModeShape::maxDepthDb (Mode mode) noexcept
{
    switch (mode)
    {
        case Mode::MixBus:    return 6.0f;
        case Mode::Guitar:    return 15.0f;
        case Mode::Piano:     return 10.0f;
        case Mode::Synth:     return 20.0f;
        case Mode::Universal:
        default:              return 24.0f;
    }
}

//==============================================================================
SpectrumMeter::SpectrumMeter()
{
    for (int i = 0; i < kMaxBands; ++i)
    {
        mainDb[(size_t) i].store (-120.0f);
        scDb[(size_t) i].store (-120.0f);
        redDb[(size_t) i].store (0.0f);
        centreHz[(size_t) i].store (1000.0f);
    }
}

//==============================================================================
SpectralDucker::SpectralDucker()
{
    window.resize ((size_t) fftSize);
    float sum = 0.0f, sumSq = 0.0f;

    for (int n = 0; n < fftSize; ++n)
    {
        // periodic sqrt-Hann: analysis * synthesis = Hann, which overlap-adds flat at 75 %
        const float hann = 0.5f * (1.0f - std::cos (juce::MathConstants<float>::twoPi * (float) n / (float) fftSize));
        window[(size_t) n] = std::sqrt (hann);
        sum += window[(size_t) n];
        sumSq += window[(size_t) n] * window[(size_t) n];
    }

    magNorm = sum * 0.5f;
    olaScale = (float) hopSize / sumSq;

    for (auto* v : { &inBuf[0], &inBuf[1], &outAccum[0], &outAccum[1] })
        v->assign ((size_t) fftSize, 0.0f);
    for (auto& v : outReady)
        v.assign ((size_t) hopSize, 0.0f);

    scBuf.assign ((size_t) fftSize, 0.0f);
    work.assign ((size_t) fftSize * 2, 0.0f);
    mags.assign ((size_t) fftSize * 2, 0.0f);
    binMask.assign ((size_t) numBins, 1.0f);
}

void SpectralDucker::prepare (double newSampleRate)
{
    sampleRate = newSampleRate > 0.0 ? newSampleRate : 48000.0;
    configuredBands = -1;
    reset();
}

void SpectralDucker::reset()
{
    for (auto* v : { &inBuf[0], &inBuf[1], &outAccum[0], &outAccum[1], &outReady[0], &outReady[1], &scBuf })
        std::fill (v->begin(), v->end(), 0.0f);

    redState.fill (0.0f);
    target.fill (0.0f);
    mainLevel.fill (-120.0f);
    scLevel.fill (-120.0f);
    pos = 0;
}

void SpectralDucker::configureBands (int numBands)
{
    numBands = juce::jlimit (4, kMaxBands, numBands);
    configuredBands = numBands;

    const float binHz = (float) (sampleRate / fftSize);
    const float ratio = std::pow (kMaxFreq / kMinFreq, 1.0f / (float) numBands);
    const int maxBin = fftSize / 2;

    bandLo.assign ((size_t) numBands, 1);
    bandHi.assign ((size_t) numBands, 1);
    bandCentre.assign ((size_t) numBands, 1000.0f);

    for (int b = 0; b < numBands; ++b)
    {
        const float e0 = kMinFreq * std::pow (ratio, (float) b);
        const float e1 = e0 * ratio;
        const float centre = std::sqrt (e0 * e1);

        int lo = (int) std::ceil (e0 / binHz);
        int hi = (int) std::floor (e1 / binHz);
        if (hi < lo)
            lo = hi = (int) std::round (centre / binHz);

        bandLo[(size_t) b] = juce::jlimit (1, maxBin, lo);
        bandHi[(size_t) b] = juce::jlimit (1, maxBin, hi);
        bandCentre[(size_t) b] = centre;

        meter.centreHz[(size_t) b].store (centre);
    }

    binBand.assign ((size_t) numBins, 0);
    binFrac.assign ((size_t) numBins, 0.0f);

    const float logRatio = std::log (ratio);
    for (int k = 0; k < numBins; ++k)
    {
        const float hz = juce::jmax (binHz * 0.5f, (float) k * binHz);
        const float x = std::log (hz / bandCentre[0]) / logRatio;
        const int i0 = juce::jlimit (0, numBands - 2, (int) std::floor (x));
        binBand[(size_t) k] = i0;
        binFrac[(size_t) k] = clamp01 (x - (float) i0);
    }

    redState.fill (0.0f);
    meter.numBands.store (numBands);
}

float SpectralDucker::bandLevelDb (const float* m, int band) const noexcept
{
    const int lo = bandLo[(size_t) band], hi = bandHi[(size_t) band];
    float power = 0.0f;
    for (int k = lo; k <= hi; ++k)
        power += m[k] * m[k];
    // energy of the whole band (a sine reads its true level regardless of band width)
    const float norm = magNorm * magNorm;
    return 10.0f * std::log10 (power / norm + 1.0e-12f);
}

void SpectralDucker::process (float* const* main, int numMainChannels,
                              const float* const* sidechain, int numScChannels,
                              int numSamples, const DuckerParams& p)
{
    if (numMainChannels <= 0)
        return;

    if (p.numBands != configuredBands)
        configureBands (p.numBands);

    const bool mono = numMainChannels == 1;
    const float scGain = numScChannels > 0 ? 1.0f / (float) numScChannels : 0.0f;
    const int writeBase = fftSize - hopSize;

    for (int i = 0; i < numSamples; ++i)
    {
        const float l = main[0][i];
        const float r = mono ? 0.0f : main[1][i];

        float sc = 0.0f;
        for (int c = 0; c < numScChannels; ++c)
            sc += sidechain[c][i];
        sc *= scGain;

        inBuf[0][(size_t) (writeBase + pos)] = mono ? l : 0.5f * (l + r); // mid
        inBuf[1][(size_t) (writeBase + pos)] = mono ? 0.0f : 0.5f * (l - r); // side
        scBuf[(size_t) (writeBase + pos)] = sc;

        const float m = outReady[0][(size_t) pos];
        const float s = outReady[1][(size_t) pos];

        if (mono)
        {
            main[0][i] = m;
        }
        else
        {
            main[0][i] = m + s;
            main[1][i] = m - s;
        }

        if (++pos == hopSize)
        {
            pos = 0;
            processFrame (p, mono);
        }
    }
}

void SpectralDucker::processFrame (const DuckerParams& p, bool mono)
{
    const int nb = configuredBands;

    // ---- 1. Sidechain analysis --------------------------------------------
    std::fill (mags.begin(), mags.end(), 0.0f);
    for (int n = 0; n < fftSize; ++n)
        mags[(size_t) n] = scBuf[(size_t) n] * window[(size_t) n];
    fft.performFrequencyOnlyForwardTransform (mags.data(), true);

    for (int b = 0; b < nb; ++b)
        scLevel[(size_t) b] = bandLevelDb (mags.data(), b);

    // ---- 2. Target reduction per band -------------------------------------
    const float depth = ModeShape::maxDepthDb (p.mode);
    const float amount = juce::jlimit (0.0f, 1.0f, p.amount);

    for (int b = 0; b < nb; ++b)
    {
        const float hz = bandCentre[(size_t) b];
        const float sc = scLevel[(size_t) b] + sidechainFilterDb (hz, p.scLowCut, p.scHighCut);
        const float rel = clamp01 ((sc - kScFloorDb) / kScRangeDb);
        target[(size_t) b] = depth * amount * rel
                             * ModeShape::weight (p.mode, hz)
                             * protectWeight (hz, p.lowProtect, p.highProtect);
    }

    // spread a little into neighbouring bands so the curve is not jagged (peaks stay intact)
    {
        std::array<float, kMaxBands> tmp {};
        for (int b = 0; b < nb; ++b)
        {
            const float prev = target[(size_t) juce::jmax (0, b - 1)];
            const float next = target[(size_t) juce::jmin (nb - 1, b + 1)];
            tmp[(size_t) b] = juce::jmax (target[(size_t) b], 0.35f * juce::jmax (prev, next));
        }
        // keep protected / zero-weighted bands exactly untouched
        for (int b = 0; b < nb; ++b)
        {
            const float pw = protectWeight (bandCentre[(size_t) b], p.lowProtect, p.highProtect);
            target[(size_t) b] = tmp[(size_t) b] * pw;
        }
    }

    // ---- 3. Attack / release in time ---------------------------------------
    const float hopSeconds = (float) hopSize / (float) sampleRate;
    const float attCoef = std::exp (-hopSeconds / juce::jmax (0.0001f, p.attackMs * 0.001f));
    const float relCoef = std::exp (-hopSeconds / juce::jmax (0.001f, p.releaseMs * 0.001f));

    float maxRed = 0.0f;
    for (int b = 0; b < nb; ++b)
    {
        auto& st = redState[(size_t) b];
        const float t = target[(size_t) b];
        const float coef = t > st ? attCoef : relCoef;
        st = t + coef * (st - t);
        maxRed = juce::jmax (maxRed, st);
    }

    // ---- 4. Per-bin mask ---------------------------------------------------
    const float wet = juce::jlimit (0.0f, 1.0f, p.wet);
    for (int k = 0; k < numBins; ++k)
    {
        const int i0 = binBand[(size_t) k];
        const float f = binFrac[(size_t) k];
        const float red = redState[(size_t) i0] + f * (redState[(size_t) i0 + 1] - redState[(size_t) i0]);
        const float g = juce::Decibels::decibelsToGain (-red);
        float mask = 1.0f - wet * (1.0f - g);

        if (p.bypass)
            mask = 1.0f;
        else if (p.listen)
            mask = 1.0f - mask; // only what is removed

        binMask[(size_t) k] = mask;
    }

    const float unmasked = (p.listen && ! p.bypass) ? 0.0f : 1.0f;
    StereoMode stereo = p.stereo;
    if (mono)
        stereo = StereoMode::Stereo;

    // ---- 5. Main channels: FFT, apply mask, IFFT, overlap-add --------------
    const int numCh = mono ? 1 : 2;
    for (int c = 0; c < numCh; ++c)
    {
        const bool applyMask = stereo == StereoMode::Stereo
                            || (stereo == StereoMode::Mid && c == 0)
                            || (stereo == StereoMode::Side && c == 1);

        std::fill (work.begin(), work.end(), 0.0f);
        for (int n = 0; n < fftSize; ++n)
            work[(size_t) n] = inBuf[(size_t) c][(size_t) n] * window[(size_t) n];

        fft.performRealOnlyForwardTransform (work.data());

        if (c == 0)
        {
            // meter: main level per band (mid / mono)
            for (int k = 0; k < numBins; ++k)
                mags[(size_t) k] = std::hypot (work[(size_t) (2 * k)], work[(size_t) (2 * k + 1)]);
            for (int b = 0; b < nb; ++b)
                mainLevel[(size_t) b] = bandLevelDb (mags.data(), b);
        }

        for (int k = 0; k < fftSize; ++k)
        {
            const int mk = k <= fftSize / 2 ? k : fftSize - k;
            const float m = applyMask ? binMask[(size_t) mk] : unmasked;
            work[(size_t) (2 * k)] *= m;
            work[(size_t) (2 * k + 1)] *= m;
        }

        fft.performRealOnlyInverseTransform (work.data());

        auto& acc = outAccum[(size_t) c];
        for (int n = 0; n < fftSize; ++n)
            acc[(size_t) n] += work[(size_t) n] * window[(size_t) n] * olaScale;
    }

    // ---- 6. Hand out finished samples, shift buffers -----------------------
    for (int c = 0; c < 2; ++c)
    {
        auto& acc = outAccum[(size_t) c];
        if (c < numCh)
            std::copy (acc.begin(), acc.begin() + hopSize, outReady[(size_t) c].begin());
        else
            std::fill (outReady[(size_t) c].begin(), outReady[(size_t) c].end(), 0.0f);

        std::move (acc.begin() + hopSize, acc.end(), acc.begin());
        std::fill (acc.end() - hopSize, acc.end(), 0.0f);

        auto& in = inBuf[(size_t) c];
        std::move (in.begin() + hopSize, in.end(), in.begin());
        std::fill (in.end() - hopSize, in.end(), 0.0f);
    }

    std::move (scBuf.begin() + hopSize, scBuf.end(), scBuf.begin());
    std::fill (scBuf.end() - hopSize, scBuf.end(), 0.0f);

    // ---- 7. Meter ----------------------------------------------------------
    for (int b = 0; b < nb; ++b)
    {
        meter.mainDb[(size_t) b].store (mainLevel[(size_t) b], std::memory_order_relaxed);
        meter.scDb[(size_t) b].store (scLevel[(size_t) b], std::memory_order_relaxed);
        meter.redDb[(size_t) b].store (p.bypass ? 0.0f : redState[(size_t) b] * wet, std::memory_order_relaxed);
    }
    meter.maxRedDb.store (p.bypass ? 0.0f : maxRed * wet, std::memory_order_relaxed);
    meter.depthDb.store (depth, std::memory_order_relaxed);
}

} // namespace insideout
