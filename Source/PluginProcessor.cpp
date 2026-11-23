#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace insideout;

InsideOutProcessor::InsideOutProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "InsideOut", createParameterLayout())
{
    pAmount      = apvts.getRawParameterValue (ParamID::amount);
    pAttack      = apvts.getRawParameterValue (ParamID::attack);
    pRelease     = apvts.getRawParameterValue (ParamID::release);
    pScLowCut    = apvts.getRawParameterValue (ParamID::scLowCut);
    pScHighCut   = apvts.getRawParameterValue (ParamID::scHighCut);
    pLowProtect  = apvts.getRawParameterValue (ParamID::lowProtect);
    pHighProtect = apvts.getRawParameterValue (ParamID::highProtect);
    pDryWet      = apvts.getRawParameterValue (ParamID::dryWet);
    pOutput      = apvts.getRawParameterValue (ParamID::output);
    pMode        = apvts.getRawParameterValue (ParamID::mode);
    pStereo      = apvts.getRawParameterValue (ParamID::stereo);
    pBands       = apvts.getRawParameterValue (ParamID::bands);
    pListen      = apvts.getRawParameterValue (ParamID::listen);
    pBypass      = apvts.getRawParameterValue (ParamID::bypass);
}

void InsideOutProcessor::prepareToPlay (double sampleRate, int)
{
    ducker.prepare (sampleRate);
    setLatencySamples (SpectralDucker::getLatencySamples());

    outputGain.reset (sampleRate, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOutput->load()));
    scEnvelope = 0.0f;
}

bool InsideOutProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    const auto mainIn  = layouts.getMainInputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;
    if (mainIn != mainOut)
        return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! (sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo()))
            return false;
    }
    return true;
}

juce::AudioProcessorParameter* InsideOutProcessor::getBypassParameter() const
{
    return apvts.getParameter (ParamID::bypass);
}

void InsideOutProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    auto mainIO = getBusBuffer (buffer, true, 0);

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // ---- sidechain ---------------------------------------------------------
    const float* scPtrs[2] = { nullptr, nullptr };
    int numSc = 0;

    if (getBusCount (true) > 1)
        if (auto* bus = getBus (true, 1); bus != nullptr && bus->isEnabled())
        {
            auto scBuffer = getBusBuffer (buffer, true, 1);
            numSc = juce::jmin (2, scBuffer.getNumChannels());
            for (int c = 0; c < numSc; ++c)
                scPtrs[c] = scBuffer.getReadPointer (c);
        }

    sidechainConnected.store (numSc > 0);

    float peak = 0.0f;
    for (int c = 0; c < numSc; ++c)
        for (int i = 0; i < numSamples; ++i)
            peak = juce::jmax (peak, std::abs (scPtrs[c][i]));

    const float decay = std::exp (-(float) numSamples / (float) (getSampleRate() > 0 ? getSampleRate() : 48000.0) / 0.5f);
    scEnvelope = juce::jmax (peak, scEnvelope * decay);
    sidechainActive.store (scEnvelope > 1.0e-4f);

    // ---- parameters --------------------------------------------------------
    DuckerParams p;
    p.amount      = pAmount->load() * 0.01f;
    p.attackMs    = pAttack->load();
    p.releaseMs   = pRelease->load();
    p.scLowCut    = pScLowCut->load();
    p.scHighCut   = pScHighCut->load();
    p.lowProtect  = pLowProtect->load();
    p.highProtect = pHighProtect->load();
    p.wet         = pDryWet->load() * 0.01f;
    p.mode        = (Mode) juce::jlimit (0, 4, (int) pMode->load());
    p.stereo      = (StereoMode) juce::jlimit (0, 2, (int) pStereo->load());
    p.numBands    = bandCountForIndex ((int) pBands->load());
    p.listen      = pListen->load() > 0.5f;
    p.bypass      = pBypass->load() > 0.5f;

    // ---- DSP ---------------------------------------------------------------
    ducker.process (mainIO.getArrayOfWritePointers(), mainIO.getNumChannels(),
                    scPtrs, numSc, numSamples, p);

    // ---- output gain -------------------------------------------------------
    outputGain.setTargetValue (p.bypass ? 1.0f : juce::Decibels::decibelsToGain (pOutput->load()));

    if (outputGain.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = outputGain.getNextValue();
            for (int c = 0; c < mainIO.getNumChannels(); ++c)
                mainIO.getWritePointer (c)[i] *= g;
        }
    }
    else if (std::abs (outputGain.getTargetValue() - 1.0f) > 1.0e-6f)
    {
        mainIO.applyGain (outputGain.getTargetValue());
    }
}

//==============================================================================
int InsideOutProcessor::getNumPrograms()
{
    return (int) factoryPresets().size();
}

void InsideOutProcessor::setCurrentProgram (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms()))
        applyPreset (index);
}

const juce::String InsideOutProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms()))
        return factoryPresets()[(size_t) index].name;
    return {};
}

void InsideOutProcessor::setParam (const char* id, float plainValue)
{
    if (auto* param = apvts.getParameter (id))
    {
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 (plainValue));
        param->endChangeGesture();
    }
}

void InsideOutProcessor::applyPreset (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;

    const auto& pr = factoryPresets()[(size_t) index];
    setParam (ParamID::mode,        (float) pr.mode);
    setParam (ParamID::amount,      pr.amount);
    setParam (ParamID::attack,      pr.attack);
    setParam (ParamID::release,     pr.release);
    setParam (ParamID::scLowCut,    pr.scLowCut);
    setParam (ParamID::scHighCut,   pr.scHighCut);
    setParam (ParamID::lowProtect,  pr.lowProtect);
    setParam (ParamID::highProtect, pr.highProtect);
    setParam (ParamID::stereo,      (float) pr.stereo);
    setParam (ParamID::dryWet,      pr.dryWet);
    setParam (ParamID::bands,       (float) pr.bands);

    currentPreset = index;
    ++presetChangeCounter;
}

void InsideOutProcessor::applyModeDefaults (int modeIndex)
{
    const auto d = modeDefaults (modeIndex);
    setParam (ParamID::mode,        (float) modeIndex);
    setParam (ParamID::lowProtect,  d.lowProtect);
    setParam (ParamID::highProtect, d.highProtect);
    setParam (ParamID::stereo,      (float) d.stereo);
}

//==============================================================================
void InsideOutProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("preset", currentPreset, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void InsideOutProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            currentPreset = juce::jlimit (0, getNumPrograms() - 1, (int) state.getProperty ("preset", 0));
            apvts.replaceState (state);
            ++presetChangeCounter;
        }
}

juce::AudioProcessorEditor* InsideOutProcessor::createEditor()
{
    return new InsideOutEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new InsideOutProcessor();
}
