#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "Presets.h"
#include "SpectralDucker.h"

class InsideOutProcessor : public juce::AudioProcessor
{
public:
    InsideOutProcessor();
    ~InsideOutProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    /** Host bypass maps to our own bypass, so latency stays constant. */
    juce::AudioProcessorParameter* getBypassParameter() const override;

    /** Applies a factory preset (notifies host). */
    void applyPreset (int index);
    /** Sets protect / stereo defaults for a mode (used when the mode is picked in the UI). */
    void applyModeDefaults (int modeIndex);

    juce::AudioProcessorValueTreeState apvts;
    insideout::SpectrumMeter& getMeter() noexcept { return ducker.getMeter(); }

    std::atomic<bool> sidechainConnected { false };
    std::atomic<bool> sidechainActive { false };
    std::atomic<int>  presetChangeCounter { 0 };

    int currentPreset = 0;

private:
    void setParam (const char* id, float plainValue);

    insideout::SpectralDucker ducker;
    juce::SmoothedValue<float> outputGain;
    float scEnvelope = 0.0f;

    std::atomic<float>* pAmount = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pScLowCut = nullptr;
    std::atomic<float>* pScHighCut = nullptr;
    std::atomic<float>* pLowProtect = nullptr;
    std::atomic<float>* pHighProtect = nullptr;
    std::atomic<float>* pDryWet = nullptr;
    std::atomic<float>* pOutput = nullptr;
    std::atomic<float>* pMode = nullptr;
    std::atomic<float>* pStereo = nullptr;
    std::atomic<float>* pBands = nullptr;
    std::atomic<float>* pListen = nullptr;
    std::atomic<float>* pBypass = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InsideOutProcessor)
};
