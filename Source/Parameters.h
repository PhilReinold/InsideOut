#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace insideout
{

namespace ParamID
{
    inline constexpr const char* amount      = "amount";
    inline constexpr const char* attack      = "attack";
    inline constexpr const char* release     = "release";
    inline constexpr const char* scLowCut    = "scLowCut";
    inline constexpr const char* scHighCut   = "scHighCut";
    inline constexpr const char* lowProtect  = "lowProtect";
    inline constexpr const char* highProtect = "highProtect";
    inline constexpr const char* dryWet      = "dryWet";
    inline constexpr const char* output      = "output";
    inline constexpr const char* mode        = "mode";
    inline constexpr const char* stereo      = "stereo";
    inline constexpr const char* bands       = "bands";
    inline constexpr const char* listen      = "listen";
    inline constexpr const char* bypass      = "bypass";
}

inline const juce::StringArray& modeNames()
{
    static const juce::StringArray names { "Mix Bus", "Guitar", "Piano", "Synth", "Universal" };
    return names;
}

inline const juce::StringArray& stereoNames()
{
    static const juce::StringArray names { "Stereo", "Mid", "Side" };
    return names;
}

inline const juce::StringArray& bandNames()
{
    static const juce::StringArray names { "16", "32", "64" };
    return names;
}

inline int bandCountForIndex (int index) { return index == 0 ? 16 : (index == 2 ? 64 : 32); }

/** German number format: comma as decimal separator, real minus sign. */
inline juce::String formatNumber (float value, int decimals)
{
    auto s = juce::String (std::abs (value), decimals).replaceCharacter ('.', ',');
    return (value < 0.0f && std::abs (value) >= 0.5f * std::pow (10.0f, (float) -decimals))
               ? juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")) + s
               : s;
}

inline juce::String formatHz (float hz)
{
    if (hz < 1000.0f)
        return juce::String (juce::roundToInt (hz)) + " Hz";

    const float k = hz / 1000.0f;
    if (std::abs (k - std::round (k)) < 0.05f)
        return juce::String (juce::roundToInt (k)) + " kHz";
    return formatNumber (k, 1) + " kHz";
}

inline juce::String formatMs (float ms)
{
    if (ms < 1.0f)   return formatNumber (ms, 1) + " ms";
    if (ms < 1000.0f) return juce::String (juce::roundToInt (ms)) + " ms";
    return formatNumber (ms / 1000.0f, 2) + " s";
}

inline float parseNumber (const juce::String& text)
{
    auto t = text.replaceCharacter (',', '.')
                 .replace (juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")), "-")
                 .trim();
    float v = t.getFloatValue();
    if (t.containsIgnoreCase ("k"))
        v *= 1000.0f;
    return v;
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto floatParam = [&] (const char* id, const char* name, NormalisableRange<float> range, float def,
                           std::function<String (float, int)> toText, const char* label = "")
    {
        layout.add (std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, range, def,
            AudioParameterFloatAttributes()
                .withLabel (label)
                .withStringFromValueFunction (std::move (toText))
                .withValueFromStringFunction ([] (const String& s) { return parseNumber (s); })));
    };

    auto skewed = [] (float lo, float hi, float centre, float step = 0.0f)
    {
        NormalisableRange<float> r (lo, hi, step);
        r.setSkewForCentre (centre);
        return r;
    };

    floatParam (ParamID::amount, "Amount", { 0.0f, 100.0f, 0.1f }, 50.0f,
                [] (float v, int) { return String (roundToInt (v)) + " %"; });

    floatParam (ParamID::attack, "Attack", skewed (0.1f, 100.0f, 10.0f), 10.0f,
                [] (float v, int) { return formatMs (v); });

    floatParam (ParamID::release, "Release", skewed (10.0f, 1000.0f, 200.0f), 250.0f,
                [] (float v, int) { return formatMs (v); });

    floatParam (ParamID::scLowCut, "SC Low Cut", skewed (20.0f, 1000.0f, 150.0f), 120.0f,
                [] (float v, int) { return v <= 20.5f ? String ("Aus") : formatHz (v); });

    floatParam (ParamID::scHighCut, "SC High Cut", skewed (2000.0f, 20000.0f, 8000.0f), 12000.0f,
                [] (float v, int) { return v >= 19990.0f ? String ("Aus") : formatHz (v); });

    floatParam (ParamID::lowProtect, "Low Protect", skewed (20.0f, 500.0f, 120.0f), 150.0f,
                [] (float v, int) { return v <= 20.5f ? String ("Aus") : formatHz (v); });

    floatParam (ParamID::highProtect, "High Protect", skewed (5000.0f, 20000.0f, 10000.0f), 10000.0f,
                [] (float v, int) { return v >= 19990.0f ? String ("Aus") : formatHz (v); });

    floatParam (ParamID::dryWet, "Dry/Wet", { 0.0f, 100.0f, 0.1f }, 100.0f,
                [] (float v, int) { return String (roundToInt (v)) + " %"; });

    floatParam (ParamID::output, "Output", { -12.0f, 12.0f, 0.1f }, 0.0f,
                [] (float v, int) { return formatNumber (v, 1) + " dB"; });

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::mode, 1 }, "Modus", modeNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::stereo, 1 }, "Stereo-Modus", stereoNames(), 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::bands, 1 }, "Baender", bandNames(), 1));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::listen, 1 }, "Listen", false));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { ParamID::bypass, 1 }, "Bypass", false));

    return layout;
}

} // namespace insideout
